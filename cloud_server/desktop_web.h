#pragma once

#include "cloud_server_global.h"

#include <QString>
#include <QUrl>
#include <QUrlQuery>

QJ_NAMESPACE_FIT_CLOUD_SERVER_BEGIN
/** 构建桌面登录页地址。 */
inline QUrl buildDesktopLoginUrl(const QUrl& frontendBase)
{
	/** 保存待返回的登录地址。 */
	QUrl url = frontendBase;
	if (!url.path().endsWith(QLatin1Char('/'))) {
		url.setPath(url.path() + QLatin1Char('/'));
	}
	return url;
}

/** 构建外部单点登录地址。 */
inline QUrl buildExternalSsoLoginUrl(const QUrl& frontendBase, const QString& ticket, const QString& redirectPath)
{
	/** 保存待返回的单点登录地址。 */
	QUrl url = frontendBase.resolved(QUrl(QStringLiteral("sso-login")));
	/** 组织单点登录查询参数。 */
	QUrlQuery query;
	query.addQueryItem(QStringLiteral("ticket"), ticket.trimmed());
	/** 保存规范化后的跳转路径。 */
	QString normalizedRedirect = redirectPath.trimmed();
	if (normalizedRedirect.isEmpty()) {
		normalizedRedirect = QStringLiteral("/personal-profile");
	}
	else if (!normalizedRedirect.startsWith(QLatin1Char('/'))) {
		normalizedRedirect.prepend(QLatin1Char('/'));
	}
	query.addQueryItem(QStringLiteral("redirect"), normalizedRedirect);
	url.setQuery(query);
	return url;
}

/** 构建最近文件页地址。 */
inline QUrl buildRecentFilesUrl(const QUrl& frontendBase)
{
	return frontendBase.resolved(QUrl(QStringLiteral("recent-files")));
}

QJ_NAMESPACE_FIT_CLOUD_SERVER_END
