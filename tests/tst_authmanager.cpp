#include <QtTest>
#include <QJsonDocument>
#include <QJsonObject>

#include "authmanager.h"
#include "mockhttpclient.h"

class tst_AuthManager : public QObject
{
    Q_OBJECT

private slots:

    void test_login_success_selfHosted()
    {
        MockHttpClient http;
        http.setBaseUrl("https://localhost:3000");

        // Self-hosted rpc_login returns {"token": "..."}
        const QJsonObject responseBody{{QStringLiteral("token"), QStringLiteral("jwt.abc.123")}};
        http.addJsonResponse("rpc/rpc_login", QJsonDocument(responseBody).toJson());

        AuthManager auth;
        QSignalSpy spyOk(&auth,  &AuthManager::authenticated);
        QSignalSpy spyFail(&auth, &AuthManager::authenticationFailed);

        // Relative endpoint: routed through client (MockHttpClient intercepts)
        const bool result = auth.login(&http,
                                        QStringLiteral("rpc/rpc_login"),
                                        QStringLiteral("user@example.com"),
                                        QStringLiteral("secret"));

        QVERIFY(result);
        QCOMPARE(spyOk.count(), 1);
        QCOMPARE(spyFail.count(), 0);
        QCOMPARE(auth.token(), QStringLiteral("jwt.abc.123"));
        QVERIFY(auth.isAuthenticated());
        QCOMPARE(http.authToken(), QStringLiteral("jwt.abc.123"));
    }

    void test_login_supabaseResponseFormat()
    {
        MockHttpClient http;
        http.setBaseUrl("https://localhost:3000");

        // Supabase-style response with "access_token" field
        const QJsonObject responseBody{{QStringLiteral("access_token"), QStringLiteral("supa.jwt.456")}};
        http.addJsonResponse("rpc/rpc_login", QJsonDocument(responseBody).toJson());

        AuthManager auth;
        QSignalSpy spyOk(&auth, &AuthManager::authenticated);

        const bool result = auth.login(&http,
                                        QStringLiteral("rpc/rpc_login"),
                                        QStringLiteral("user@example.com"),
                                        QStringLiteral("secret"));

        QVERIFY(result);
        QCOMPARE(spyOk.count(), 1);
        QCOMPARE(auth.token(), QStringLiteral("supa.jwt.456"));
    }

    void test_login_serverError()
    {
        MockHttpClient http;
        http.setBaseUrl("https://localhost:3000");
        http.addJsonResponse("rpc/rpc_login", {}, 401);

        AuthManager auth;
        QSignalSpy spyOk(&auth,   &AuthManager::authenticated);
        QSignalSpy spyFail(&auth, &AuthManager::authenticationFailed);

        const bool result = auth.login(&http,
                                        QStringLiteral("rpc/rpc_login"),
                                        QStringLiteral("user@example.com"),
                                        QStringLiteral("wrongpassword"));

        QVERIFY(!result);
        QCOMPARE(spyOk.count(), 0);
        QCOMPARE(spyFail.count(), 1);
        QVERIFY(!auth.isAuthenticated());
        QVERIFY(auth.token().isEmpty());
    }

    void test_login_missingTokenInResponse()
    {
        MockHttpClient http;
        http.setBaseUrl("https://localhost:3000");

        const QJsonObject badResponse{{QStringLiteral("message"), QStringLiteral("ok")}};
        http.addJsonResponse("rpc/rpc_login", QJsonDocument(badResponse).toJson());

        AuthManager auth;
        QSignalSpy spyFail(&auth, &AuthManager::authenticationFailed);

        const bool result = auth.login(&http,
                                        QStringLiteral("rpc/rpc_login"),
                                        QStringLiteral("user@example.com"),
                                        QStringLiteral("secret"));

        QVERIFY(!result);
        QCOMPARE(spyFail.count(), 1);
        QVERIFY(!auth.isAuthenticated());
    }

    void test_setToken_valid()
    {
        MockHttpClient http;
        http.setBaseUrl("https://localhost:3000");

        AuthManager auth;
        QSignalSpy spyOk(&auth, &AuthManager::authenticated);

        const bool result = auth.setToken(&http, "my.pre.obtained.jwt");

        QVERIFY(result);
        QCOMPARE(spyOk.count(), 1);
        QCOMPARE(auth.token(), QStringLiteral("my.pre.obtained.jwt"));
        QCOMPARE(http.authToken(), QStringLiteral("my.pre.obtained.jwt"));
    }

    void test_setToken_emptyString()
    {
        MockHttpClient http;
        AuthManager auth;
        QSignalSpy spyFail(&auth, &AuthManager::authenticationFailed);

        const bool result = auth.setToken(&http, QString());

        QVERIFY(!result);
        QCOMPARE(spyFail.count(), 1);
        QVERIFY(!auth.isAuthenticated());
    }

    void test_clearToken()
    {
        MockHttpClient http;
        http.setBaseUrl("https://localhost:3000");

        AuthManager auth;
        auth.setToken(&http, "some.jwt");
        QVERIFY(auth.isAuthenticated());

        auth.clearToken();
        QVERIFY(!auth.isAuthenticated());
        QVERIFY(auth.token().isEmpty());
    }

    void test_login_sendsCorrectPayload()
    {
        MockHttpClient http;
        http.setBaseUrl("https://localhost:3000");

        const QJsonObject responseBody{{QStringLiteral("token"), QStringLiteral("tok")}};
        http.addJsonResponse("rpc/rpc_login", QJsonDocument(responseBody).toJson());

        AuthManager auth;
        auth.login(&http,
                   QStringLiteral("rpc/rpc_login"),
                   QStringLiteral("alice@example.com"),
                   QStringLiteral("pass123"));

        QVERIFY(!http.recordedRequests.isEmpty());
        const auto &req = http.recordedRequests.first();
        QCOMPARE(req.verb, QStringLiteral("POST"));
        QVERIFY(req.url.contains("rpc/rpc_login"));

        // Self-hosted relative endpoint sends "username" (not "email" — that's Supabase)
        const QJsonObject payload = QJsonDocument::fromJson(req.body).object();
        QCOMPARE(payload["username"].toString(), QStringLiteral("alice@example.com"));
        QCOMPARE(payload["password"].toString(), QStringLiteral("pass123"));
    }
};

QTEST_MAIN(tst_AuthManager)
#include "tst_authmanager.moc"
