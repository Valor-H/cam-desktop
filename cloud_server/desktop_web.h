#pragma once

#include "cloud_server_global.h"

#include <QString>
#include <QUrl>
#include <QUrlQuery>

QJ_NAMESPACE_FIT_CLOUD_SERVER_BEGIN
/** 构建桌面登录页地址。 */
inline QUrl BuildDesktopLoginUrl(const QUrl& frontend_base)
{
	/** 保存待返回的登录地址。 */
	QUrl url = frontend_base;
	if (!url.path().endsWith(QLatin1Char('/'))) {
		url.setPath(url.path() + QLatin1Char('/'));
	}
	return url;
}

/** 构建外部单点登录地址。 */
inline QUrl BuildExternalSsoLoginUrl(const QUrl& frontend_base, const QString& ticket, const QString& redirect_path)
{
	/** 保存待返回的单点登录地址。 */
	QUrl url = frontend_base.resolved(QUrl(QStringLiteral("sso-login")));
	/** 组织单点登录查询参数。 */
	QUrlQuery query;
	query.addQueryItem(QStringLiteral("ticket"), ticket.trimmed());
	/** 保存规范化后的跳转路径。 */
	QString normalized_redirect = redirect_path.trimmed();
	if (normalized_redirect.isEmpty()) {
		normalized_redirect = QStringLiteral("/personal-profile");
	}
	else if (!normalized_redirect.startsWith(QLatin1Char('/'))) {
		normalized_redirect.prepend(QLatin1Char('/'));
	}
	query.addQueryItem(QStringLiteral("redirect"), normalized_redirect);
	url.setQuery(query);
	return url;
}

/** 构建最近文件页地址。 */
inline QUrl BuildRecentFilesUrl(const QUrl& frontend_base)
{
	return frontend_base.resolved(QUrl(QStringLiteral("recent-files")));
}

/** 构建桌面上传选择页地址。 */
inline QUrl BuildDesktopUploadPickerUrl(const QUrl& frontend_base)
{
	return frontend_base.resolved(QUrl(QStringLiteral("desktop-upload-picker")));
}

QJ_NAMESPACE_FIT_CLOUD_SERVER_END
