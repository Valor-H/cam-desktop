#pragma once

#include "user_global.h"

#include <QString>

class QCefView;
class QJsonObject;

QJ_NAMESPACE_FIT_USER_BEGIN
class UserAuthService;

QJsonObject BuildDesktopRuntimePayload(const UserAuthService* authService);
QString BuildDesktopRuntimeInjectionScript(const UserAuthService* authService);
bool InjectDesktopRuntimeIntoView(QCefView* view, const UserAuthService* authService, const QString& sourceUrl = QString());

QJ_NAMESPACE_FIT_USER_END
