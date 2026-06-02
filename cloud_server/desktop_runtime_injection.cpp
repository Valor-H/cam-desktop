#include "desktop_runtime_injection.h"

#include "user_auth_service.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

#include <QCefView.h>

QJ_NAMESPACE_FIT_CLOUD_SERVER_BEGIN

QJsonObject BuildDesktopRuntimePayload(const UserAuthService* authService)
{
	QJsonObject payload{
		{ QStringLiteral("runtimeMode"), QStringLiteral("desktop") },
		{ QStringLiteral("loggedIn"), false },
		{ QStringLiteral("token"), QString() },
		{ QStringLiteral("user"), QJsonValue::Null },
	};

	if (!authService || !authService->Session()) {
		return payload;
	}

	const QVariantMap currentUser = authService->Session()->CurrentUser();
	const QString token = authService->Session()->AuthToken().trimmed();
	const bool loggedIn = authService->Session()->IsAuthenticated() && !token.isEmpty() && !currentUser.isEmpty();

	payload.insert(QStringLiteral("loggedIn"), loggedIn);
	payload.insert(QStringLiteral("token"), loggedIn ? token : QString());
	if (loggedIn) {
		payload.insert(QStringLiteral("user"), QJsonObject::fromVariantMap(currentUser));
	}

	return payload;
}

QString BuildDesktopRuntimeInjectionScript(const UserAuthService* authService)
{
	const QString payloadJson = QString::fromUtf8(
		QJsonDocument(BuildDesktopRuntimePayload(authService)).toJson(QJsonDocument::Compact));
	return QStringLiteral(
		"(function(){"
		"window.__QJCAM_DESKTOP_RUNTIME__=Object.freeze(%1);"
		"window.dispatchEvent(new CustomEvent('qjcam-desktop-runtime-ready'));"
		"})();")
		.arg(payloadJson);
}

bool InjectDesktopRuntimeIntoView(QCefView* view, const UserAuthService* authService, const QString& sourceUrl)
{
	if (!view) {
		return false;
	}

	const QString scriptUrl =
		sourceUrl.trimmed().isEmpty() ? QStringLiteral("qjcam://desktop-runtime.js") : sourceUrl.trimmed();
	return view->executeJavascript(
		QCefView::MainFrameID, BuildDesktopRuntimeInjectionScript(authService), scriptUrl);
}

QJ_NAMESPACE_FIT_CLOUD_SERVER_END
