#pragma once

#include "cloud_server_global.h"
#include <QObject>
#include <QScopedPointer>
#include <QString>
#include <QUrl>

QJ_NAMESPACE_FIT_CLOUD_SERVER_BEGIN
class UserAuthService;

class CLOUD_SERVER_EXPORT DesktopWebServer final : public QObject
{
public:
    /** 构造桌面网页服务实例。 */
    explicit DesktopWebServer(UserAuthService* authService, QObject* parent = nullptr);
    /** 析构桌面网页服务实例。 */
    ~DesktopWebServer() override;
    /** 启动本地桌面网页服务。 */
    bool Start();
    /** 停止本地桌面网页服务。 */
    void Stop();
    /** 返回本地桌面网页服务基础地址。 */
    QUrl BaseUrl() const;

private:
    /** 配置服务可用的路由。 */
    void ConfigureRoutes();
    /** 返回注入桌面 bootstrap 后的 index.html 内容。 */
    QString BuildIndexHtmlResponse() const;
    /** 将请求路径解析为静态文件路径。 */
    QString ResolveStaticFilePath(const QString& requestPath) const;
    /** 返回静态网页根目录路径。 */
    QString WebRootPath() const;
    /** 声明私有实现结构。 */
    struct Private;
    /** 持有私有实现对象。 */
    QScopedPointer<Private> d;
    /** 保存认证服务实例。 */
    UserAuthService* m_authService { nullptr };
    /** 标识服务是否已经启动。 */
    bool m_started { false };
    /** 保存服务监听端口。 */
    int m_port { 31870 };
};

QJ_NAMESPACE_FIT_CLOUD_SERVER_END
