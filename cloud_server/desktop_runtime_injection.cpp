#include "desktop_runtime_injection.h"

#include "user_auth_service.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

#include <QCefView.h>

QJ_NAMESPACE_FIT_CLOUD_SERVER_BEGIN

QJsonObject BuildDesktopRuntimePayload(const UserAuthService* auth_service)
{
	QJsonObject payload{
		{ QStringLiteral("runtimeMode"), QStringLiteral("desktop") },
		{ QStringLiteral("loggedIn"), false },
		{ QStringLiteral("token"), QString() },
		{ QStringLiteral("user"), QJsonValue::Null },
	};

	if (!auth_service || !auth_service->Session()) {
		return payload;
	}

	const QVariantMap current_user = auth_service->Session()->CurrentUser();
	const QString token = auth_service->Session()->AuthToken().trimmed();
	const bool logged_in = auth_service->Session()->IsAuthenticated() && !token.isEmpty() && !current_user.isEmpty();

	payload.insert(QStringLiteral("loggedIn"), logged_in);
	payload.insert(QStringLiteral("token"), logged_in ? token : QString());
	if (logged_in) {
		payload.insert(QStringLiteral("user"), QJsonObject::fromVariantMap(current_user));
	}

	return payload;
}

QString BuildDesktopRuntimeInjectionScript(const UserAuthService* auth_service)
{
	const QString payload_json = QString::fromUtf8(
		QJsonDocument(BuildDesktopRuntimePayload(auth_service)).toJson(QJsonDocument::Compact));
	return QStringLiteral(
		"(function(){"
		"window.__QJCAM_DESKTOP_RUNTIME__=Object.freeze(%1);"
		"window.dispatchEvent(new CustomEvent('qjcam-desktop-runtime-ready'));"
		"})();")
		.arg(payload_json);
}

bool InjectDesktopRuntimeIntoView(QCefView* view, const UserAuthService* auth_service, const QString& source_url)
{
	if (!view) {
		return false;
	}

	const QString script_url =
		source_url.trimmed().isEmpty() ? QStringLiteral("qjcam://desktop-runtime.js") : source_url.trimmed();
	return view->executeJavascript(
		QCefView::MainFrameID, BuildDesktopRuntimeInjectionScript(auth_service), script_url);
}

QJ_NAMESPACE_FIT_CLOUD_SERVER_END
