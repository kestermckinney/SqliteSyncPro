#include "authmanager.h"
#include "httpclient.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QEventLoop>
#include <QUrl>

AuthManager::AuthManager(QObject *parent)
    : QObject(parent)
{
}

bool AuthManager::login(HttpClient   *client,
                         const QString &authEndpoint,
                         const QString &email,
                         const QString &password)
{
    Q_ASSERT(client);
    m_token.clear();

    const bool isAbsolute = authEndpoint.startsWith(QStringLiteral("http://"))
                         || authEndpoint.startsWith(QStringLiteral("https://"));

    // Supabase /auth/v1/token expects {"email", "password"}.
    // Self-hosted rpc_login expects {"username", "password"}.
    QJsonObject body;
    body[isAbsolute ? QStringLiteral("email") : QStringLiteral("username")] = email;
    body[QStringLiteral("password")] = password;
    const QByteArray payload = QJsonDocument(body).toJson(QJsonDocument::Compact);

    QByteArray response;
    int statusCode = 0;

    if (isAbsolute) {
        // Full URL (e.g. Supabase /auth/v1/token) — use a direct QNAM call so
        // the auth host is independent of the client's PostgREST base URL.
        // Include apikey header if the client has one set (required by Supabase).
        QNetworkAccessManager nam;
        QNetworkRequest req{QUrl(authEndpoint)};
        req.setHeader(QNetworkRequest::ContentTypeHeader,
                      QByteArrayLiteral("application/json"));
        if (!client->apiKey().isEmpty())
            req.setRawHeader("apikey", client->apiKey().toUtf8());

#ifdef QT_DEBUG
        qInfo() << "[AuthManager::login] POST" << authEndpoint;
#endif

        QNetworkReply *reply = nam.post(req, payload);
        QEventLoop loop;
        QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        loop.exec();

        statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        response   = reply->readAll();
        reply->deleteLater();
    } else {
        // Relative endpoint (e.g. "rpc/rpc_login") — route through the client
        // so the base URL is prepended and the mock works in unit tests.
        response   = client->post(authEndpoint, payload);
        statusCode = client->lastStatusCode();
    }

#ifdef QT_DEBUG
    qInfo() << "[AuthManager::login] status=" << statusCode;
#endif

    if (statusCode < 200 || statusCode >= 300) {
        const QString reason = QStringLiteral("HTTP %1: %2")
                                   .arg(statusCode)
                                   .arg(QString::fromUtf8(response));
#ifdef QT_DEBUG
        qWarning() << "[AuthManager::login]" << reason;
#endif
        // Status 0 means the server was unreachable (network down / connection refused).
        // Emit networkError rather than authenticationFailed so callers can distinguish
        // a connectivity problem from bad credentials.
        if (statusCode == 0) {
#ifdef QT_DEBUG
            qWarning() << "[AuthManager::login] Network error — server unreachable (status 0)";
#endif
            emit networkError(reason);
        } else {
#ifdef QT_DEBUG
            qWarning() << "[AuthManager::login] Authentication failed — HTTP" << statusCode;
#endif
            emit authenticationFailed(reason);
        }
        return false;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(response);
    if (!doc.isObject()) {
        const QString reason = QStringLiteral("Auth response is not JSON");
#ifdef QT_DEBUG
        qWarning() << "[AuthManager::login]" << reason;
#endif
        emit authenticationFailed(reason);
        return false;
    }

    const QJsonObject obj = doc.object();

    // Accept "access_token" (Supabase) or "token" (self-hosted rpc_login)
    QString jwt = obj.value(QStringLiteral("access_token")).toString();
    if (jwt.isEmpty())
        jwt = obj.value(QStringLiteral("token")).toString();

    if (jwt.isEmpty()) {
        const QString reason = QStringLiteral(
            "Auth response missing token field ('access_token' or 'token')");
#ifdef QT_DEBUG
        qWarning() << "[AuthManager::login]" << reason;
#endif
        emit authenticationFailed(reason);
        return false;
    }

    m_token = jwt;
    client->setAuthToken(m_token);
    // For Supabase: every PostgREST request also needs apikey: <anon_key>.
    // The apiKey was already set on the client by the caller; setAuthToken()
    // above does not clear it, so subsequent sync calls carry both headers.
#ifdef QT_DEBUG
    qInfo() << "[AuthManager::login] JWT obtained successfully";
#endif
    emit authenticated();
    return true;
}

bool AuthManager::setToken(HttpClient *client, const QString &token)
{
    Q_ASSERT(client);

    if (token.trimmed().isEmpty()) {
        emit authenticationFailed(QStringLiteral("Empty token provided"));
        return false;
    }

    m_token = token;
    client->setAuthToken(m_token);
    emit authenticated();
    return true;
}
