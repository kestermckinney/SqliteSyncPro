-- =============================================================================
-- SQLiteSyncPro – PostgreSQL Setup Script
--
-- Run as a database superuser before starting PostgREST.
-- The SQLSync Administrator application runs this automatically.
--
-- Changes from the email-based design:
--   • auth_users now uses 'username' (= PostgreSQL role name) as the key
--   • Each sync user IS a PostgreSQL NOLOGIN ROLE
--   • RLS uses current_user (set by PostgREST from the JWT 'role' claim)
--   • rpc_login returns role=username so PostgREST switches to the user's role
-- =============================================================================

CREATE EXTENSION IF NOT EXISTS pgcrypto;

-- =============================================================================
-- Roles
-- PostgREST connects as 'authenticator', switches to 'anon' or a user role.
-- Individual sync users are NOLOGIN roles granting app_user and listed in
-- auth_users.  rpc_login validates them and issues JWTs with role=username.
-- =============================================================================

DO $$ BEGIN
    CREATE ROLE authenticator NOINHERIT LOGIN PASSWORD 'change_me_in_production';
EXCEPTION WHEN duplicate_object THEN
    RAISE NOTICE 'Role authenticator already exists, skipping.';
END $$;

DO $$ BEGIN
    CREATE ROLE anon NOLOGIN;
EXCEPTION WHEN duplicate_object THEN
    RAISE NOTICE 'Role anon already exists, skipping.';
END $$;

DO $$ BEGIN
    CREATE ROLE app_user NOLOGIN;
EXCEPTION WHEN duplicate_object THEN
    RAISE NOTICE 'Role app_user already exists, skipping.';
END $$;

GRANT anon     TO authenticator;
GRANT app_user TO authenticator;
GRANT USAGE ON SCHEMA public TO anon, app_user;


-- =============================================================================
-- JWT secret
--
-- The secret is embedded directly in rpc_login below rather than stored as a
-- database or role-level GUC.  This avoids ALTER DATABASE / ALTER ROLE SET
-- for custom parameters, which hosted services (Neon, etc.) restrict.
-- Replace CHANGE_ME_... with your actual secret in the rpc_login function.
-- =============================================================================


-- =============================================================================
-- JWT signing helpers (pgcrypto only – no pgjwt extension needed)
-- =============================================================================

CREATE OR REPLACE FUNCTION _base64url_encode(data BYTEA)
RETURNS TEXT AS $$
    SELECT rtrim(translate(encode(data, 'base64'), E'+/\n', '-_'), '=');
$$ LANGUAGE SQL IMMUTABLE STRICT;

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
$$ LANGUAGE plpgsql SECURITY DEFINER;

REVOKE ALL ON FUNCTION _base64url_encode(BYTEA) FROM PUBLIC;
REVOKE ALL ON FUNCTION _sign_jwt(JSON, TEXT)    FROM PUBLIC;


-- =============================================================================
-- auth_users – one row per sync user; username = PostgreSQL role name
-- =============================================================================

CREATE TABLE IF NOT EXISTS auth_users (
    username      TEXT   NOT NULL PRIMARY KEY
                         CHECK (username = lower(trim(username))),
    password_hash TEXT   NOT NULL,
    created_at    BIGINT NOT NULL
                         DEFAULT (EXTRACT(EPOCH FROM now()) * 1000)::BIGINT
);

REVOKE ALL ON auth_users FROM PUBLIC, anon, app_user;


-- =============================================================================
-- sync_data – single generic table for all synced SQLite rows
-- =============================================================================

CREATE TABLE IF NOT EXISTS sync_data (
    userid      TEXT   NOT NULL,
    tablename   TEXT   NOT NULL,
    id          TEXT   NOT NULL,
    updateddate BIGINT NOT NULL,
    jsonrowdata JSONB  NOT NULL,
    PRIMARY KEY (userid, tablename, id)
);

CREATE INDEX IF NOT EXISTS idx_sync_data_pull
    ON sync_data (userid, tablename, updateddate);

ALTER TABLE sync_data ENABLE ROW LEVEL SECURITY;

-- RLS uses current_user, which PostgREST sets from the JWT 'role' claim.
-- This means each user can only see/write their own rows without any JWT parsing.
CREATE POLICY sync_data_select ON sync_data
    FOR SELECT USING (userid = current_user);

CREATE POLICY sync_data_insert ON sync_data
    FOR INSERT WITH CHECK (userid = current_user);

CREATE POLICY sync_data_update ON sync_data
    FOR UPDATE USING (userid = current_user);

GRANT SELECT, INSERT, UPDATE ON sync_data TO app_user;


-- =============================================================================
-- rpc_login – validates username/password, issues JWT with role=username
--
-- PostgREST exposes this as: POST /rpc/rpc_login
--   Request:  { "p_username": "alice", "p_password": "secret" }
--   Response: { "token": "<hs256-jwt>" }
--
-- JWT payload:
--   sub  – username (stored as userid in sync_data)
--   role – username (PostgREST uses this to SET LOCAL ROLE, enabling RLS)
--   iat  – issued-at epoch seconds
--   exp  – expiry epoch seconds (default 24 h)
-- =============================================================================

CREATE OR REPLACE FUNCTION rpc_login(p_username TEXT, p_password TEXT)
RETURNS JSON AS $$
DECLARE
    v_username TEXT;
    v_hash     TEXT;
    v_now      BIGINT;
    v_ttl      INT := COALESCE(
                    NULLIF(current_setting('app.jwt_ttl_seconds', true), '')::INT,
                    86400);
    v_payload  JSON;
BEGIN
    v_username := lower(trim(p_username));

    SELECT password_hash INTO v_hash
    FROM   auth_users
    WHERE  username = v_username;

    IF NOT FOUND OR v_hash IS NULL OR v_hash <> crypt(p_password, v_hash) THEN
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

    -- Replace the literal below with your actual JWT secret.
    RETURN json_build_object('token', _sign_jwt(v_payload, 'CHANGE_ME_replace_with_32+_random_characters'));
END;
$$ LANGUAGE plpgsql SECURITY DEFINER;

REVOKE ALL     ON FUNCTION rpc_login(TEXT, TEXT) FROM PUBLIC;
GRANT  EXECUTE ON FUNCTION rpc_login(TEXT, TEXT) TO anon;


-- =============================================================================
-- To add a sync user (run as superuser after setup):
--
--   CREATE ROLE alice NOLOGIN;
--   GRANT app_user TO alice;
--   GRANT alice TO authenticator;
--   INSERT INTO auth_users (username, password_hash)
--   VALUES ('alice', crypt('secret123', gen_salt('bf', 12)));
--
-- Or use the SQLSync Administrator application.
-- =============================================================================
