#pragma once

#include <QObject>
#include <QString>
#include <QUrl>
#include <QVariantMap>

#include <hv/HttpServer.h>

namespace qianjizn { namespace user {
class UserAuthService;
} }

class DesktopFrontendServer final : public QObject
{
    Q_OBJECT

public:
    explicit DesktopFrontendServer(qianjizn::user::UserAuthService* authService, QObject* parent = nullptr);
    ~DesktopFrontendServer() override;

    bool Start();
    void Stop();
    QUrl BaseUrl() const;

private:
    struct BootstrapSnapshot
    {
        bool loggedIn { false };
        bool offline { false };
        QString backendUrl;
        QString websocketUrl;
        QString helpDocUrl;
        QString token;
        QVariantMap user;
    };

    void ConfigureRoutes();
    QString NormalizeRequestPath(HttpRequest* req) const;
    QString BuildBootstrapJson() const;
    BootstrapSnapshot CaptureSnapshot() const;
    BootstrapSnapshot CaptureSnapshotSync() const;
    bool IsApiReachable() const;
    QString ResolveStaticFilePath(const QString& requestPath) const;
    QString WebRootPath() const;

    qianjizn::user::UserAuthService* m_authService { nullptr };
    mutable hv::HttpService m_service;
    mutable hv::HttpServer m_server;
    bool m_started { false };
    int m_port { 31870 };
};
