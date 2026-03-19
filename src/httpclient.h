#pragma once

#include <QObject>
#include <QString>
#include <QByteArray>
#include <QStringList>
#include <QUrlQuery>
#include <QNetworkAccessManager>
#include <QNetworkRequest>

/**
 * Thin wrapper around QNetworkAccessManager that communicates with a PostgREST server.
 *
 * All calls are synchronous from the caller's perspective (implemented with QEventLoop).
 * This is intentional: the sync operation is itself a sequential batch job.
 *
 * Sub-class and override executeRequest() to inject a mock in unit tests.
 */
class HttpClient : public QObject
{
    Q_OBJECT

public:
    explicit HttpClient(QObject *parent = nullptr);

    void setBaseUrl(const QString &url);
    void setAuthToken(const QString &token);

    /**
     * Set a Supabase anon key.  When set, every request includes
     * "apikey: <key>" in addition to the Authorization bearer token.
     * Leave empty (default) for self-hosted PostgREST, which does not
     * require this header.
     */
    void setApiKey(const QString &key);

    QString baseUrl()    const { return m_baseUrl;    }
    QString authToken()  const { return m_authToken;  }
    QString apiKey()     const { return m_apiKey;     }

    /** POST /endpoint with JSON body. preferHeaders are passed as the Prefer: header. */
    QByteArray get(const QString &endpoint, const QUrlQuery &query = {});
    QByteArray post(const QString &endpoint, const QByteArray &body,
                    const QStringList &preferHeaders = {});
    QByteArray patch(const QString &endpoint, const QUrlQuery &query,
                     const QByteArray &body,
                     const QStringList &preferHeaders = {QStringLiteral("return=minimal")});

    int     lastStatusCode() const { return m_lastStatusCode; }
    QString lastError()      const { return m_lastError;      }
    bool    wasSuccessful()  const { return m_lastStatusCode >= 200 && m_lastStatusCode < 300; }

protected:
    void setLastStatusCode(int code)          { m_lastStatusCode = code; }
    void setLastError(const QString &error)   { m_lastError = error;     }

    virtual QByteArray executeRequest(QNetworkRequest &request,
                                      const QByteArray &verb,
                                      const QByteArray &body);

private:
    QNetworkRequest buildRequest(const QString &endpoint, const QUrlQuery &query = {}) const;

    QNetworkAccessManager *m_nam;
    QString  m_baseUrl;
    QString  m_authToken;
    QString  m_apiKey;          // Supabase anon key; empty for self-hosted
    int      m_lastStatusCode = 0;
    QString  m_lastError;
};
