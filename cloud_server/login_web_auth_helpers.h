#pragma once

#include <QUrl>
#include <QVariantMap>

namespace LoginWebAuth
{
	/** 定义认证网页路由类型。 */
	enum class AuthRoute
	{
		Login,          /** 登录页面路由。 */
		Register,       /** 注册页面路由。 */
		Reset,          /** 重置密码页面路由。 */
		Unknown         /** 未知页面路由。 */
	};

	/** 从 URL 中提取认证路由路径。 */
	QString ExtractAuthRoutePath(const QUrl& url);
	/** 根据路径解析认证路由类型。 */
	AuthRoute RouteFromPath(const QString& path);
	/** 清洗网页登录返回的数据。 */
	QVariantMap SanitizeLoginPayload(const QVariantMap& input);
	/** 判断当前页面来源是否可信。 */
	bool IsTrustedUiSource(const QUrl& currentUrl, const QUrl& loginPageUrl);
	/** 判断当前调用来源是否可信。 */
	bool IsTrustedInvokeSource(const QUrl& currentUrl, const QUrl& loginPageUrl);
}
