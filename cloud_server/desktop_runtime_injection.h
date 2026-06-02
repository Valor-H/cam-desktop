#pragma once

#include "cloud_server_global.h"

#include <QString>

class QCefView;
class QJsonObject;

QJ_NAMESPACE_FIT_CLOUD_SERVER_BEGIN
class UserAuthService;
/** 构建桌面运行时注入数据。 */
QJsonObject BuildDesktopRuntimePayload(const UserAuthService* auth_service);
/** 构建桌面运行时注入脚本。 */
QString BuildDesktopRuntimeInjectionScript(const UserAuthService* auth_service);
/** 向指定浏览器视图注入桌面运行时。 */
bool InjectDesktopRuntimeIntoView(QCefView* view,
	const UserAuthService* auth_service,
	const QString& source_url = QString());

QJ_NAMESPACE_FIT_CLOUD_SERVER_END
