#pragma once

#include "ultramill_global.h"
#include "user_global.h"
#include <QObject>
#include <QString>
#include <QUrl>
#include <QVariantMap>

#include <hv/HttpServer.h>

QJ_NAMESPACE_FIT_USER_BEGIN
class UserAuthService;
QJ_NAMESPACE_FIT_USER_END

QJ_NAMESPACE_ULTRACAM_ULTRAMILL_BEGIN

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
        QString backendUrl;
        QString websocketUrl;
        QString helpDocUrl;
        QString mockServiceUrl;
        QString token;
        QVariantMap user;
    };

    void ConfigureRoutes();
    QString NormalizeRequestPath(HttpRequest* req) const;
    QString BuildBootstrapJson() const;
    BootstrapSnapshot CaptureSnapshot() const;
    BootstrapSnapshot CaptureSnapshotSync() const;
    QString ResolveStaticFilePath(const QString& requestPath) const;
    QString WebRootPath() const;

    qianjizn::user::UserAuthService* m_authService { nullptr };
    mutable hv::HttpService m_service;
    mutable hv::HttpServer m_server;
    bool m_started { false };
    int m_port { 31870 };
};

QJ_NAMESPACE_ULTRACAM_ULTRAMILL_END
