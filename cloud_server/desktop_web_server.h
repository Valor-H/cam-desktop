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
	explicit DesktopWebServer(UserAuthService* auth_service, QObject* parent = nullptr);
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
	/** 构建桌面端运行时配置 JSON。 */
	QString BuildRuntimeConfigJson() const;
	/** 将请求路径解析为静态文件路径。 */
	QString ResolveStaticFilePath(const QString& request_path) const;
	/** 返回静态网页根目录路径。 */
	QString WebRootPath() const;
	/** 声明私有实现结构。 */
	struct Private;
	/** 持有私有实现对象。 */
	QScopedPointer<Private> d;
	/** 保存认证服务实例。 */
	UserAuthService* _authService;
	/** 标识 HTTP 服务是否已注册。 */
	bool _serviceRegistered;
	/** 标识服务是否已经启动。 */
	bool _started;
	/** 保存服务监听端口。 */
	int _port;
};

QJ_NAMESPACE_FIT_CLOUD_SERVER_END
