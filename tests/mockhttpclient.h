// Copyright (C) 2026 Paul McKinney
#pragma once

#include "httpclient.h"

#include <QByteArray>
#include <QList>
#include <QPair>
#include <QMap>
#include <QString>
#include <QNetworkRequest>

/**
 * Test double for HttpClient.
 *
 * Callers register canned responses keyed by a substring of the request URL.
 * The first matching key wins.  Unmatched requests return an empty 200 body.
 *
 * Recorded requests can be inspected afterwards to assert what the engine sent.
 */
class MockHttpClient : public HttpClient
{
public:
    struct MockResponse
    {
        int       statusCode = 200;
        QByteArray body;
    };

    struct RecordedRequest
    {
        QString    verb;
        QString    url;
        QByteArray body;
    };

    void addResponse(const QString &urlSubstring, const MockResponse &response)
    {
        m_responses.append({urlSubstring, response});
    }

    /** Shortcut for a plain 200 JSON response. */
    void addJsonResponse(const QString &urlSubstring, const QByteArray &json, int statusCode = 200)
    {
        addResponse(urlSubstring, {statusCode, json});
    }

    void clearResponses() { m_responses.clear(); }

    QList<RecordedRequest> recordedRequests;

protected:
    QByteArray executeRequest(QNetworkRequest &request,
                              const QByteArray &verb,
                              const QByteArray &body) override
    {
        const QString url = request.url().toString();
        recordedRequests.append({QString::fromLatin1(verb), url, body});

        for (const auto &entry : m_responses) {
            if (url.contains(entry.first)) {
                setLastStatusCode(entry.second.statusCode);
                setLastError(entry.second.statusCode >= 200 &&
                             entry.second.statusCode < 300
                                 ? QString()
                                 : QStringLiteral("Mock error %1").arg(entry.second.statusCode));
                return entry.second.body;
            }
        }

        // Default: empty 200
        setLastStatusCode(200);
        setLastError({});
        return {};
    }

private:
    QList<QPair<QString, MockResponse>> m_responses;
};
