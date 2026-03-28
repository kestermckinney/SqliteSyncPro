#include "httpclient.h"

#include <QNetworkReply>
#include <QEventLoop>
#include <QUrl>
#include <QDebug>

HttpClient::HttpClient(QObject *parent)
    : QObject(parent)
    , m_nam(new QNetworkAccessManager(this))
{
}

void HttpClient::setBaseUrl(const QString &url)
{
    m_baseUrl = url;
    if (m_baseUrl.endsWith(QLatin1Char('/')))
        m_baseUrl.chop(1);
}

void HttpClient::setAuthToken(const QString &token)
{
    m_authToken = token;
}

void HttpClient::setApiKey(const QString &key)
{
    m_apiKey = key;
}

QNetworkRequest HttpClient::buildRequest(const QString &endpoint, const QUrlQuery &query) const
{
    QString urlStr = m_baseUrl + QLatin1Char('/') + endpoint;
    QUrl url(urlStr);
    if (!query.isEmpty())
        url.setQuery(query);

    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    req.setRawHeader("Accept", "application/json");

    if (!m_apiKey.isEmpty())
        req.setRawHeader("apikey", m_apiKey.toUtf8());

    if (!m_authToken.isEmpty())
        req.setRawHeader("Authorization",
                         QStringLiteral("Bearer %1").arg(m_authToken).toUtf8());

    return req;
}

QByteArray HttpClient::executeRequest(QNetworkRequest &request,
                                       const QByteArray &verb,
                                       const QByteArray &body)
{
    m_lastStatusCode = 0;
    m_lastError.clear();
    m_lastContentRange.clear();


    // qDebug().noquote() << QStringLiteral("[HttpClient] --> %1 %2")
    //                           .arg(QString::fromLatin1(verb), request.url().toString());
    // if (!body.isEmpty())
    //     qDebug().noquote() << QStringLiteral("[HttpClient]     body (%1 bytes)").arg(body.size());

    QNetworkReply *reply = nullptr;
    if (verb == "GET") {
        reply = m_nam->get(request);
    } else if (verb == "HEAD") {
        reply = m_nam->head(request);
    } else if (verb == "POST") {
        reply = m_nam->post(request, body);
    } else if (verb == "PATCH") {
        reply = m_nam->sendCustomRequest(request, "PATCH", body);
    } else {
        m_lastError = QStringLiteral("Unsupported HTTP verb: %1").arg(QString::fromLatin1(verb));
        return {};
    }

    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    const QByteArray responseBody = reply->readAll();
    m_lastStatusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    m_lastContentRange = QString::fromLatin1(reply->rawHeader("Content-Range"));

    // qDebug().noquote() << QStringLiteral("[HttpClient] <-- %1 (%2 bytes) %3")
    //                           .arg(m_lastStatusCode)
    //                           .arg(responseBody.size())
    //                           .arg(request.url().toString());
    // if (!responseBody.isEmpty())
    //     qDebug().noquote() << QStringLiteral("[HttpClient]     response: %1")
    //                               .arg(QString::fromUtf8(responseBody.left(1000)));

    if (reply->error() != QNetworkReply::NoError) {
        m_lastError = reply->errorString();
        qWarning().noquote() << QStringLiteral("[HttpClient]     network error: %1").arg(m_lastError);
        if (!responseBody.isEmpty())
            m_lastError += QStringLiteral(": ") + QString::fromUtf8(responseBody);
    }

    if (m_lastStatusCode == 0)
        qWarning().noquote()
            << "[HttpClient]     (status 0 — server unreachable or connection refused)";

    reply->deleteLater();
    return responseBody;
}

QByteArray HttpClient::get(const QString &endpoint, const QUrlQuery &query)
{
    auto req = buildRequest(endpoint, query);
    return executeRequest(req, "GET", {});
}

QByteArray HttpClient::post(const QString &endpoint, const QByteArray &body,
                             const QStringList &preferHeaders)
{
    auto req = buildRequest(endpoint);
    if (!preferHeaders.isEmpty())
        req.setRawHeader("Prefer", preferHeaders.join(QLatin1Char(',')).toUtf8());
    return executeRequest(req, "POST", body);
}

QByteArray HttpClient::patch(const QString &endpoint, const QUrlQuery &query,
                              const QByteArray &body, const QStringList &preferHeaders)
{
    auto req = buildRequest(endpoint, query);
    if (!preferHeaders.isEmpty())
        req.setRawHeader("Prefer", preferHeaders.join(QLatin1Char(',')).toUtf8());
    return executeRequest(req, "PATCH", body);
}

int HttpClient::countRows(const QString &endpoint, const QUrlQuery &query)
{
    auto req = buildRequest(endpoint, query);
    req.setRawHeader("Prefer", "count=exact");
    executeRequest(req, "HEAD", {});
    if (!wasSuccessful())
        return -1;
    // Content-Range: 0-99/12345  or  */12345  (when 0 rows: */0)
    const int slashPos = m_lastContentRange.lastIndexOf(QLatin1Char('/'));
    if (slashPos < 0)
        return -1;
    bool ok = false;
    const int count = m_lastContentRange.mid(slashPos + 1).toInt(&ok);
    return ok ? count : -1;
}
