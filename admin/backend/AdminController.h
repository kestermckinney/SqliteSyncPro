#pragma once

#include <QObject>
#include <QString>
#include <QThread>
#include <QVariantList>
#include <QSqlDatabase>
#include <QJsonDocument>

#include "ServerMode.h"

class SetupWorker;
class TeardownWorker;

class AdminController : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString host         READ host         WRITE setHost         NOTIFY hostChanged)
    Q_PROPERTY(int     port         READ port         WRITE setPort         NOTIFY portChanged)
    Q_PROPERTY(QString superuser    READ superuser    WRITE setSuperuser    NOTIFY superuserChanged)
    Q_PROPERTY(QString superPass    READ superPass    WRITE setSuperPass    NOTIFY superPassChanged)
    Q_PROPERTY(int     serverMode   READ serverMode   WRITE setServerMode   NOTIFY serverModeChanged)
    Q_PROPERTY(QString dbName       READ dbName       WRITE setDbName       NOTIFY dbNameChanged)
    Q_PROPERTY(QString supabaseUrl        READ supabaseUrl        WRITE setSupabaseUrl        NOTIFY supabaseUrlChanged)
    Q_PROPERTY(QString supabaseServiceKey READ supabaseServiceKey WRITE setSupabaseServiceKey NOTIFY supabaseServiceKeyChanged)

    Q_PROPERTY(QString authenticatorPassword READ authenticatorPassword NOTIFY secretsReady)
    Q_PROPERTY(QString jwtSecret             READ jwtSecret             NOTIFY secretsReady)

    Q_PROPERTY(bool    setupRunning  READ isSetupRunning  NOTIFY setupStateChanged)
    Q_PROPERTY(bool    setupDone     READ isSetupDone     NOTIFY setupStateChanged)
    Q_PROPERTY(int     totalSteps    READ totalSteps      CONSTANT)

    Q_PROPERTY(bool teardownRunning READ isTeardownRunning NOTIFY teardownStateChanged)
    Q_PROPERTY(bool teardownDone    READ isTeardownDone    NOTIFY teardownStateChanged)

    Q_PROPERTY(QVariantList users READ users NOTIFY usersChanged)

public:
    explicit AdminController(QObject *parent = nullptr);
    ~AdminController() override;

    QString host()      const { return m_host;      }
    int     port()      const { return m_port;       }
    QString superuser() const { return m_superuser;  }
    QString superPass() const { return m_superPass;  }
    int     serverMode() const { return static_cast<int>(m_serverMode); }
    QString dbName()    const { return m_dbName;     }
    QString supabaseUrl()        const { return m_supabaseUrl;        }
    QString supabaseServiceKey() const { return m_supabaseServiceKey;  }

    void setHost(const QString &v)     { if (m_host      != v) { m_host      = v; emit hostChanged();      } }
    void setPort(int v)                { if (m_port      != v) { m_port      = v; emit portChanged();      } }
    void setSuperuser(const QString &v){ if (m_superuser  != v) { m_superuser  = v; emit superuserChanged(); } }
    void setSuperPass(const QString &v){ if (m_superPass  != v) { m_superPass  = v; emit superPassChanged(); } }
    void setServerMode(int v) {
        const ServerMode mode = static_cast<ServerMode>(v);
        if (m_serverMode != mode) { m_serverMode = mode; emit serverModeChanged(); }
    }
    void setDbName(const QString &v)   { if (m_dbName != v) { m_dbName = v; emit dbNameChanged(); } }
    void setSupabaseUrl(const QString &v) {
        if (m_supabaseUrl != v) { m_supabaseUrl = v; emit supabaseUrlChanged(); }
    }
    void setSupabaseServiceKey(const QString &v) {
        if (m_supabaseServiceKey != v) { m_supabaseServiceKey = v; emit supabaseServiceKeyChanged(); }
    }

    QString authenticatorPassword() const { return m_authenticatorPassword; }
    QString jwtSecret()             const { return m_jwtSecret; }
    bool    isSupabaseMode()        const { return m_serverMode == ServerMode::Supabase; }

    // Supabase mode implies hosted-style behavior (soft role steps).
    bool    hostedMode()            const { return m_serverMode != ServerMode::SelfHosted; }

    bool isSetupRunning() const { return m_setupRunning; }
    bool isSetupDone()    const { return m_setupDone;    }
    int  totalSteps()     const { return 21; }

    QVariantList users() const { return m_users; }

    Q_INVOKABLE void testConnection();
    Q_INVOKABLE void checkIfAlreadySetup();
    Q_INVOKABLE void startSetup();
    Q_INVOKABLE void cancelSetup();

    bool isTeardownRunning() const { return m_teardownRunning; }
    bool isTeardownDone()    const { return m_teardownDone;    }
    int  teardownTotalSteps() const;

    Q_INVOKABLE void startTeardown();

    Q_INVOKABLE void saveConnectionSettings();

    Q_INVOKABLE void loadUsers();
    Q_INVOKABLE void addUser(const QString &username, const QString &password);
    Q_INVOKABLE void removeUser(const QString &username);
    Q_INVOKABLE void editUser(const QString &username, const QString &newPassword);

    Q_INVOKABLE void copyToClipboard(const QString &text);
    Q_INVOKABLE QString postgrestConfig() const;

signals:
    void hostChanged();
    void portChanged();
    void superuserChanged();
    void superPassChanged();
    void serverModeChanged();
    void dbNameChanged();
    void supabaseUrlChanged();
    void supabaseServiceKeyChanged();
    void secretsReady();
    void setupStateChanged();
    void usersChanged();

    void connectionTestResult(bool success, const QString &message);

    void stepCompleted(int index, bool success,
                       const QString &stepName,
                       const QString &detail);

    void setupFinished(bool success, const QString &message);
    void alreadySetup(bool isSetup);

    void teardownStateChanged();
    void teardownStepCompleted(int index, bool success,
                                const QString &stepName,
                                const QString &detail);
    void teardownFinished(bool success, const QString &message);

    void userAdded(bool success, const QString &message);
    void userRemoved(bool success, const QString &username);
    void userEdited(bool success, const QString &message);

private:
    void generateSecrets();
    void loadConnectionSettings();
    bool ensureDatabaseExists(QString &errorOut);
    QSqlDatabase openAdminConnection() const;
    void closeAdminConnection(QSqlDatabase &db) const;

    /**
     * Make a synchronous HTTP call to the Supabase Auth admin API.
     * method:     "GET", "POST", "PUT", or "DELETE"
     * path:       e.g. "/auth/v1/admin/users"
     * body:       request body (may be null for GET/DELETE)
     * statusOut:  if non-null, receives the HTTP status code
     * Uses m_supabaseUrl and m_supabaseServiceKey for the request.
     */
    QJsonDocument callSupabase(const QString      &method,
                                const QString      &path,
                                const QJsonDocument &body,
                                int                *statusOut = nullptr) const;

    static bool isValidUsername(const QString &name);

    QString m_host      = QStringLiteral("localhost");
    int     m_port      = 5432;
    QString m_dbName    = QStringLiteral("sqlitesyncpro");
    QString m_superuser = QStringLiteral("postgres");
    QString m_superPass;
    ServerMode m_serverMode = ServerMode::SelfHosted;
    QString m_supabaseUrl;
    QString m_supabaseServiceKey;

    QString m_authenticatorPassword;
    QString m_jwtSecret;

    bool         m_setupRunning = false;
    bool         m_setupDone    = false;
    QVariantList m_users;

    QThread     *m_workerThread = nullptr;
    SetupWorker *m_worker       = nullptr;

    bool            m_teardownRunning = false;
    bool            m_teardownDone    = false;
    QThread        *m_teardownThread  = nullptr;
    TeardownWorker *m_teardownWorker  = nullptr;
};
