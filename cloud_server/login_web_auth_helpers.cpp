#include "login_web_auth_helpers.h"

namespace LoginWebAuth
{
	QString ExtractAuthRoutePath(const QUrl& url)
	{
		QString path = url.path().trimmed();
		if (!path.isEmpty() && path != QLatin1String("/")) {
			return path;
		}

		const QString frag = url.fragment(QUrl::FullyDecoded).trimmed();
		if (frag.startsWith(QLatin1Char('/'))) {
			const int q = frag.indexOf(QLatin1Char('?'));
			return q >= 0 ? frag.left(q) : frag;
		}
		return path;
	}

	AuthRoute RouteFromPath(const QString& path)
	{
		QString p = path.trimmed();
		if (p.isEmpty() || p == QLatin1Char('/')) {
			return AuthRoute::LOGIN;
		}
		if (p.size() > 1 && p.endsWith(QLatin1Char('/'))) {
			p.chop(1);
		}

		if (p == QStringLiteral("/login")) {
			return AuthRoute::LOGIN;
		}
		if (p == QStringLiteral("/register")) {
			return AuthRoute::REGISTER;
		}
		if (p == QStringLiteral("/reset-password")) {
			return AuthRoute::RESET;
		}

		return AuthRoute::UNKNOWN;
	}

	QVariantMap SanitizeLoginPayload(const QVariantMap& input)
	{
		QVariantMap sanitized;
		sanitized.insert(QStringLiteral("token"), input.value(QStringLiteral("token")).toString());

		const QVariantMap user = input.value(QStringLiteral("user")).toMap();
		if (!user.isEmpty()) {
			QVariantMap user_payload;
			user_payload.insert(QStringLiteral("uuid"), user.value(QStringLiteral("uuid")).toString());
			user_payload.insert(QStringLiteral("userName"), user.value(QStringLiteral("userName")).toString());
			user_payload.insert(QStringLiteral("nickName"), user.value(QStringLiteral("nickName")).toString());
			user_payload.insert(QStringLiteral("email"), user.value(QStringLiteral("email")).toString());
			user_payload.insert(QStringLiteral("phone"), user.value(QStringLiteral("phone")).toString());
			user_payload.insert(QStringLiteral("sex"), user.value(QStringLiteral("sex")).toInt());
			user_payload.insert(QStringLiteral("avatar"), user.value(QStringLiteral("avatar")).toString());
			user_payload.insert(QStringLiteral("role"), user.value(QStringLiteral("role")).toInt());
			sanitized.insert(QStringLiteral("user"), user_payload);
		}
		return sanitized;
	}

	bool IsTrustedUiSource(const QUrl& current_url, const QUrl& login_page_url)
	{
		if (!current_url.isValid() || !login_page_url.isValid()) {
			return false;
		}
		return current_url.scheme() == login_page_url.scheme() && current_url.host() == login_page_url.host()
			&& current_url.port() == login_page_url.port();
	}

	bool IsTrustedInvokeSource(const QUrl& current_url, const QUrl& login_page_url)
	{
		if (!IsTrustedUiSource(current_url, login_page_url)) {
			return false;
		}
		const AuthRoute route = RouteFromPath(ExtractAuthRoutePath(current_url));
		return route == AuthRoute::LOGIN || route == AuthRoute::REGISTER || route == AuthRoute::RESET;
	}
}
