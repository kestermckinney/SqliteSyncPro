// Copyright (C) 2026 Paul McKinney
#pragma once

#include <QObject>
#include <QString>

class HttpClient;

/**
 * Manages authentication for both self-hosted PostgREST and Supabase.
 *
 * Single login() method accepts any auth endpoint plus email/password.
 * On success the JWT is set on the HttpClient so all subsequent sync
 * requests carry the correct Authorization header.
 */
class AuthManager : public QObject
{
    Q_OBJECT

public:
    explicit AuthManager(QObject *parent = nullptr);

    /**
     * Authenticate with an endpoint, email, and password.
     *
     * Works for both self-hosted PostgREST and Supabase:
     *   - Self-hosted: pass a relative endpoint (e.g. "rpc/rpc_login"); the
     *     request is routed through the HttpClient using its base URL.
     *   - Supabase:    pass the full auth URL
     *     (e.g. "https://xxx.supabase.co/auth/v1/token?grant_type=password");
     *     a direct network call is made and the apikey from client->apiKey()
     *     is included automatically.
     *
     * Body sent: {"email": email, "password": password}
     * Token field accepted: "access_token" (Supabase) or "token" (self-hosted).
     *
     * On success the token is set on the client via setAuthToken().
     */
    bool login(HttpClient   *client,
               const QString &authEndpoint,
               const QString &email,
               const QString &password);

    /** Use a pre-obtained JWT directly. Sets the token on the client. */
    bool setToken(HttpClient *client, const QString &token);

    QString token()          const { return m_token;          }
    bool    isAuthenticated() const { return !m_token.isEmpty(); }
    void    clearToken()            { m_token.clear();         }

signals:
    void authenticated();
    void authenticationFailed(const QString &reason);
    void networkError(const QString &reason);  // emitted when server is unreachable (HTTP status 0)

private:
    QString m_token;
};
