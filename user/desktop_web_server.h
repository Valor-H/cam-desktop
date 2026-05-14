#pragma once

#include "user_global.h"
#include <QObject>
#include <QScopedPointer>
#include <QString>
#include <QUrl>

QJ_NAMESPACE_FIT_USER_BEGIN
class UserAuthService;

class USER_EXPORT DesktopWebServer final : public QObject
{
public:
    explicit DesktopWebServer(UserAuthService* authService, QObject* parent = nullptr);
    ~DesktopWebServer() override;

    bool Start();
    void Stop();
    QUrl BaseUrl() const;

private:
    void ConfigureRoutes();
    QString ResolveStaticFilePath(const QString& requestPath) const;
    QString WebRootPath() const;

    struct Private;
    QScopedPointer<Private> d;

    UserAuthService* m_authService { nullptr };
    bool m_started { false };
    int m_port { 31870 };
};

QJ_NAMESPACE_FIT_USER_END
