// Copyright (C) 2026 Paul McKinney
#include "SetupWorker.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QUuid>
#include <QDebug>

SetupWorker::SetupWorker(QObject *parent)
    : QObject(parent)
{
}

void SetupWorker::cancel()
{
    m_cancelled = true;
}

bool SetupWorker::execStep(int index, const QString &name, const QString &sql)
{
    if (m_cancelled) {
        emit stepCompleted(index, false, name, QStringLiteral("Cancelled"));
        return false;
    }

    QSqlQuery q(m_db);
    if (!q.exec(sql)) {
        const QString err = q.lastError().text();
        emit stepCompleted(index, false, name, err);
        return false;
    }

    emit stepCompleted(index, true, name, QString());
    return true;
}

bool SetupWorker::execSoftStep(int index, const QString &name, const QString &sql)
{
    if (m_cancelled) {
        emit stepCompleted(index, false, name, QStringLiteral("Cancelled"));
        return false;
    }

    // Use a savepoint so that if the statement fails, PostgreSQL's transaction
    // error state is cleared via ROLLBACK TO SAVEPOINT before continuing.
    // Without this, a failure inside an open transaction causes every subsequent
    // statement to fail immediately with 25P02 (current transaction is aborted).
    const QString sp = QStringLiteral("sp_%1").arg(index);
    QSqlQuery q(m_db);
    q.exec(QStringLiteral("SAVEPOINT %1").arg(sp));

    if (!q.exec(sql)) {
        const QString err = q.lastError().text();
        q.exec(QStringLiteral("ROLLBACK TO SAVEPOINT %1").arg(sp));
        q.exec(QStringLiteral("RELEASE SAVEPOINT %1").arg(sp));
        emit stepCompleted(index, true,
                           name + QStringLiteral(" (skipped – hosted mode)"),
                           err);
        return true;
    }

    q.exec(QStringLiteral("RELEASE SAVEPOINT %1").arg(sp));
    emit stepCompleted(index, true, name, QString());
    return true;
}

void SetupWorker::runSetup(const QString &host,
                            int            port,
                            const QString &dbName,
                            const QString &superuser,
                            const QString &superPass,
                            const QString &authenticatorPassword,
                            const QString &jwtSecret,
                            bool           supabaseMode)
{
    m_cancelled  = false;
    m_supabaseMode = supabaseMode;

    // supabaseMode implies hosted-mode behavior for role/grant soft steps
    const bool hostedMode = supabaseMode;

    m_connName = QStringLiteral("SqlSyncAdmin_setup_%1")
                     .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));

    m_db = QSqlDatabase::addDatabase(QStringLiteral("QPSQL"), m_connName);
    m_db.setHostName(host);
    m_db.setPort(port);
    m_db.setDatabaseName(dbName);
    m_db.setUserName(superuser);
    m_db.setPassword(superPass);

    auto cleanup = [this]() {
        m_db.close();
        m_db = QSqlDatabase();
        QSqlDatabase::removeDatabase(m_connName);
    };

    if (!m_db.open()) {
        emit finished(false, QStringLiteral("Cannot connect to PostgreSQL: %1")
                                 .arg(m_db.lastError().text()));
        cleanup();
        return;
    }

    // Dispatch helper: uses soft steps for role/grant DDL when in hosted mode.
    auto roleStep = [this, hostedMode](int i, const QString &n, const QString &s) -> bool {
        return hostedMode ? execSoftStep(i, n, s) : execStep(i, n, s);
    };

    // -----------------------------------------------------------------------
    // STEP 0 (outside transaction) – pgcrypto cannot run inside a transaction
    // -----------------------------------------------------------------------
    if (!execStep(0, QStringLiteral("Enable pgcrypto extension"),
                  QStringLiteral("CREATE EXTENSION IF NOT EXISTS pgcrypto"))) {
        cleanup();
        emit finished(false, QStringLiteral("Failed at step 0"));
        return;
    }

    // -----------------------------------------------------------------------
    // Open transaction for steps 1-21
    // -----------------------------------------------------------------------
    {
        QSqlQuery q(m_db);
        q.exec(QStringLiteral("BEGIN"));
        m_inTransaction = true;
    }

    // authenticatorPassword and jwtSecret are URL-safe base64 [A-Za-z0-9-_]
    // – no SQL metacharacters, safe to interpolate directly.
    int s = 1;
    bool ok = false;

    // -----------------------------------------------------------------------
    // Roles
    // In hosted-service mode (Supabase, etc.) these roles may already
    // exist and the service may block role/grant DDL.  execSoftStep() logs
    // the failure as a warning and continues rather than aborting setup.
    // -----------------------------------------------------------------------
    ok = roleStep(s++,
        QStringLiteral("Create role: pnauthenticator"),
        QStringLiteral(R"(
DO $$
BEGIN
    CREATE ROLE pnauthenticator NOINHERIT LOGIN PASSWORD '%1';
EXCEPTION WHEN duplicate_object THEN
    ALTER ROLE pnauthenticator WITH LOGIN PASSWORD '%1';
END $$)").arg(authenticatorPassword));
    if (!ok) goto rollback;

    ok = roleStep(s++,
        QStringLiteral("Create role: pnanon"),
        QStringLiteral(R"(
DO $$ BEGIN
    CREATE ROLE pnanon NOLOGIN;
EXCEPTION WHEN duplicate_object THEN NULL;
END $$)"));
    if (!ok) goto rollback;

    ok = roleStep(s++,
        QStringLiteral("Create role: pnapp_user"),
        QStringLiteral(R"(
DO $$ BEGIN
    CREATE ROLE pnapp_user NOLOGIN;
EXCEPTION WHEN duplicate_object THEN NULL;
END $$)"));
    if (!ok) goto rollback;

    ok = roleStep(s++,
        QStringLiteral("Grant roles to pnauthenticator"),
        QStringLiteral(R"(
DO $$ BEGIN
    GRANT pnanon     TO pnauthenticator;
    GRANT pnapp_user TO pnauthenticator;
END $$)"));
    if (!ok) goto rollback;

    ok = roleStep(s++,
        QStringLiteral("Grant schema usage to PostgREST roles"),
        QStringLiteral(
            "GRANT USAGE ON SCHEMA public TO pnanon, pnapp_user"));
    if (!ok) goto rollback;

    // -----------------------------------------------------------------------
    // JWT helper functions
    // In Supabase mode PostgREST uses JWT from Supabase Auth, so these functions are
    // not needed.  Use soft steps so the setup doesn't fail on Neon.
    // -----------------------------------------------------------------------
    {
        auto jwtStep = [this, supabaseMode](int i, const QString &n, const QString &sql) -> bool {
            return supabaseMode ? execSoftStep(i, n, sql) : execStep(i, n, sql);
        };

        ok = jwtStep(s++,
            QStringLiteral("Create function: _base64url_encode"),
            QStringLiteral(R"(
CREATE OR REPLACE FUNCTION _base64url_encode(data BYTEA)
RETURNS TEXT AS $$
    SELECT rtrim(
               translate(encode(data, 'base64'), E'+/\n', '-_'),
               '='
           );
$$ LANGUAGE SQL IMMUTABLE STRICT)"));
        if (!ok) goto rollback;

        ok = jwtStep(s++,
            QStringLiteral("Restrict access to _base64url_encode"),
            QStringLiteral("REVOKE ALL ON FUNCTION _base64url_encode(BYTEA) FROM PUBLIC"));
        if (!ok) goto rollback;

        ok = jwtStep(s++,
            QStringLiteral("Create function: _sign_jwt"),
            QStringLiteral(R"(
CREATE OR REPLACE FUNCTION _sign_jwt(payload JSON, secret TEXT)
RETURNS TEXT AS $$
DECLARE
    header_b64    TEXT;
    payload_b64   TEXT;
    signing_input TEXT;
    sig           TEXT;
BEGIN
    header_b64    := _base64url_encode('{"alg":"HS256","typ":"JWT"}'::BYTEA);
    payload_b64   := _base64url_encode(payload::TEXT::BYTEA);
    signing_input := header_b64 || '.' || payload_b64;
    sig           := _base64url_encode(hmac(signing_input, secret, 'sha256'));
    RETURN signing_input || '.' || sig;
END;
$$ LANGUAGE plpgsql SECURITY DEFINER)"));
        if (!ok) goto rollback;

        ok = jwtStep(s++,
            QStringLiteral("Restrict access to _sign_jwt"),
            QStringLiteral("REVOKE ALL ON FUNCTION _sign_jwt(JSON, TEXT) FROM PUBLIC"));
        if (!ok) goto rollback;
    }

    // -----------------------------------------------------------------------
    // auth_users – username IS the PostgreSQL role name
    // In Supabase mode Supabase Auth manages users; auth_users is not needed.
    // -----------------------------------------------------------------------
    {
        auto authStep = [this, supabaseMode](int i, const QString &n, const QString &sql) -> bool {
            return supabaseMode ? execSoftStep(i, n, sql) : execStep(i, n, sql);
        };

        ok = authStep(s++,
            QStringLiteral("Create table: auth_users"),
            QStringLiteral(R"(
CREATE TABLE IF NOT EXISTS auth_users (
    username      TEXT   NOT NULL PRIMARY KEY
                         CHECK (username = lower(trim(username))),
    password_hash TEXT   NOT NULL,
    created_at    BIGINT NOT NULL
                         DEFAULT (EXTRACT(EPOCH FROM now()) * 1000)::BIGINT
))"));
        if (!ok) goto rollback;

        ok = authStep(s++,
            QStringLiteral("Lock down auth_users table"),
            QStringLiteral("REVOKE ALL ON auth_users FROM PUBLIC, pnanon, pnapp_user"));
        if (!ok) goto rollback;
    }

    // -----------------------------------------------------------------------
    // sync_data
    //
    // server_modified_at is populated by a BEFORE INSERT/UPDATE trigger on
    // every write so the pull cursor is monotonic regardless of clients'
    // updateddate values (which may be backdated or affected by clock skew).
    // The pull index is on server_modified_at so pulls scan in true write order.
    // -----------------------------------------------------------------------
    ok = execStep(s++,
        QStringLiteral("Create table: sync_data"),
        QStringLiteral(R"(
CREATE TABLE IF NOT EXISTS sync_data (
    userid             TEXT   NOT NULL,
    tablename          TEXT   NOT NULL,
    id                 TEXT   NOT NULL,
    updateddate        BIGINT NOT NULL,
    server_modified_at BIGINT NOT NULL,
    jsonrowdata        JSONB  NOT NULL,
    PRIMARY KEY (userid, tablename, id)
))"));
    if (!ok) goto rollback;

    ok = execStep(s++,
        QStringLiteral("Create function: sync_data_stamp_server_time"),
        QStringLiteral(R"(
CREATE OR REPLACE FUNCTION sync_data_stamp_server_time()
RETURNS TRIGGER AS $$
BEGIN
    NEW.server_modified_at := (EXTRACT(EPOCH FROM clock_timestamp()) * 1000)::BIGINT;
    RETURN NEW;
END;
$$ LANGUAGE plpgsql)"));
    if (!ok) goto rollback;

    ok = execStep(s++,
        QStringLiteral("Create trigger: sync_data_stamp_server_time"),
        QStringLiteral(R"(
DO $$ BEGIN
    CREATE TRIGGER sync_data_stamp_server_time_trg
        BEFORE INSERT OR UPDATE ON sync_data
        FOR EACH ROW EXECUTE FUNCTION sync_data_stamp_server_time();
EXCEPTION WHEN duplicate_object THEN NULL;
END $$)"));
    if (!ok) goto rollback;

    ok = execStep(s++,
        QStringLiteral("Create index on sync_data"),
        QStringLiteral(R"(
CREATE INDEX IF NOT EXISTS idx_sync_data_pull
    ON sync_data (userid, tablename, server_modified_at))"));
    if (!ok) goto rollback;

    // -----------------------------------------------------------------------
    // Row-Level Security
    // Self-hosted:  current_user (PostgREST sets role = username from JWT)
    // Supabase mode: request.jwt.claims GUC (PostgREST sets this for each request;
    //               all requests run as pnapp_user role, RLS checks sub claim)
    // -----------------------------------------------------------------------
    ok = execStep(s++,
        QStringLiteral("Enable row-level security on sync_data"),
        QStringLiteral("ALTER TABLE sync_data ENABLE ROW LEVEL SECURITY"));
    if (!ok) goto rollback;

    if (supabaseMode) {
        ok = execStep(s++,
            QStringLiteral("Create RLS policy: SELECT"),
            QStringLiteral(R"(
DO $$ BEGIN
    CREATE POLICY sync_data_select ON sync_data FOR SELECT
        USING (userid = current_setting('request.jwt.claims', true)::json->>'sub');
EXCEPTION WHEN duplicate_object THEN NULL;
END $$)"));
        if (!ok) goto rollback;

        ok = execStep(s++,
            QStringLiteral("Create RLS policy: INSERT"),
            QStringLiteral(R"(
DO $$ BEGIN
    CREATE POLICY sync_data_insert ON sync_data FOR INSERT
        WITH CHECK (userid = current_setting('request.jwt.claims', true)::json->>'sub');
EXCEPTION WHEN duplicate_object THEN NULL;
END $$)"));
        if (!ok) goto rollback;

        ok = execStep(s++,
            QStringLiteral("Create RLS policy: UPDATE"),
            QStringLiteral(R"(
DO $$ BEGIN
    CREATE POLICY sync_data_update ON sync_data FOR UPDATE
        USING (userid = current_setting('request.jwt.claims', true)::json->>'sub');
EXCEPTION WHEN duplicate_object THEN NULL;
END $$)"));
        if (!ok) goto rollback;
    } else {
        ok = execStep(s++,
            QStringLiteral("Create RLS policy: SELECT"),
            QStringLiteral(R"(
DO $$ BEGIN
    CREATE POLICY sync_data_select ON sync_data FOR SELECT
        USING (userid = current_user);
EXCEPTION WHEN duplicate_object THEN NULL;
END $$)"));
        if (!ok) goto rollback;

        ok = execStep(s++,
            QStringLiteral("Create RLS policy: INSERT"),
            QStringLiteral(R"(
DO $$ BEGIN
    CREATE POLICY sync_data_insert ON sync_data FOR INSERT
        WITH CHECK (userid = current_user);
EXCEPTION WHEN duplicate_object THEN NULL;
END $$)"));
        if (!ok) goto rollback;

        ok = execStep(s++,
            QStringLiteral("Create RLS policy: UPDATE"),
            QStringLiteral(R"(
DO $$ BEGIN
    CREATE POLICY sync_data_update ON sync_data FOR UPDATE
        USING (userid = current_user);
EXCEPTION WHEN duplicate_object THEN NULL;
END $$)"));
        if (!ok) goto rollback;
    }

    ok = execStep(s++,
        QStringLiteral("Grant sync_data access to pnapp_user"),
        QStringLiteral("GRANT SELECT, INSERT, UPDATE ON sync_data TO pnapp_user"));
    if (!ok) goto rollback;

    // -----------------------------------------------------------------------
    // rpc_login – validates username/password, returns JWT with role=username
    // PostgREST will SET LOCAL ROLE <username> so current_user matches USERID
    // In Supabase mode, Supabase Auth handles login; rpc_login is not needed.
    // -----------------------------------------------------------------------
    {
        auto loginStep = [this, supabaseMode, &jwtSecret](int i, const QString &n,
                                                       const QString &sql) -> bool {
            Q_UNUSED(jwtSecret)
            return supabaseMode ? execSoftStep(i, n, sql) : execStep(i, n, sql);
        };

        ok = loginStep(s++,
            QStringLiteral("Create function: rpc_login"),
            QStringLiteral(R"(
CREATE OR REPLACE FUNCTION rpc_login(username TEXT, password TEXT)
RETURNS JSON AS $$
DECLARE
    v_username   TEXT;
    v_hash       TEXT;
    v_now        BIGINT;
    v_ttl        INT := COALESCE(
                     NULLIF(current_setting('app.jwt_ttl_seconds', true), '')::INT,
                     86400);
    v_payload    JSON;
BEGIN
    v_username := lower(trim(username));

    SELECT password_hash INTO v_hash
    FROM   auth_users
    WHERE  auth_users.username = v_username;

    IF NOT FOUND OR v_hash IS NULL OR v_hash <> crypt(password, v_hash) THEN
        PERFORM pg_sleep(0.1);
        RAISE EXCEPTION 'Invalid username or password'
            USING ERRCODE = 'invalid_password';
    END IF;

    v_now     := EXTRACT(EPOCH FROM clock_timestamp())::BIGINT;
    v_payload := json_build_object(
        'sub',  v_username,
        'role', v_username,
        'iat',  v_now,
        'exp',  v_now + v_ttl
    );

    RETURN json_build_object('token', _sign_jwt(v_payload, '%1'));
END;
$$ LANGUAGE plpgsql SECURITY DEFINER)").arg(jwtSecret));
        if (!ok) goto rollback;

        ok = loginStep(s++,
            QStringLiteral("Set rpc_login permissions"),
            QStringLiteral(R"(
DO $$ BEGIN
    REVOKE ALL     ON FUNCTION rpc_login(TEXT, TEXT) FROM PUBLIC;
    GRANT  EXECUTE ON FUNCTION rpc_login(TEXT, TEXT) TO pnanon;  -- username, password
END $$)"));
        if (!ok) goto rollback;
    }

    // -----------------------------------------------------------------------
    // Commit
    // -----------------------------------------------------------------------
    {
        QSqlQuery q(m_db);
        q.exec(QStringLiteral("COMMIT"));
        m_inTransaction = false;
    }

    cleanup();
    emit finished(true, QString());
    return;

rollback:
    if (m_inTransaction) {
        QSqlQuery q(m_db);
        q.exec(QStringLiteral("ROLLBACK"));
        m_inTransaction = false;
    }
    cleanup();
    emit finished(false, QStringLiteral("Setup aborted and rolled back."));
}
