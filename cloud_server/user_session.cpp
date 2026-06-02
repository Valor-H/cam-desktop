#include "user_session.h"

namespace
{
	const QString s_tokenKey = QStringLiteral("token");
	const QString s_userKey = QStringLiteral("user");
	const QString s_loggedInKey = QStringLiteral("loggedIn");
}

QJ_NAMESPACE_FIT_CLOUD_SERVER_BEGIN

UserSession::UserSession(QObject* parent)
	: QObject(parent)
	, _authenticated(false)
	, _authToken()
	, _currentUser()
{
}

void UserSession::SetAuthenticatedState(bool on)
{
	if (_authenticated == on) {
		return;
	}
	_authenticated = on;
	emit AuthStateChanged(on);
}

void UserSession::ApplyFromLoginPayload(const QVariantMap& payload)
{
	const QString token = payload.value(s_tokenKey).toString().trimmed();
	if (token.isEmpty()) {
		_authToken.clear();
		_currentUser.clear();
		SetAuthenticatedState(false);
		emit UserProfileChanged();
		return;
	}

	const QVariantMap user = payload.value(s_userKey).toMap();
	_authToken = token;
	if (!user.isEmpty()) {
		_currentUser = user;
	}
	SetAuthenticatedState(true);
	emit UserProfileChanged();
}

void UserSession::ApplyFromProbe(const QVariantMap& data)
{
	const bool logged_in = data.value(s_loggedInKey).toBool();
	if (!logged_in) {
		_authToken.clear();
		_currentUser.clear();
		SetAuthenticatedState(false);
		emit UserProfileChanged();
		return;
	}

	const QString token = data.value(s_tokenKey).toString().trimmed();
	_authToken = token;
	_currentUser.clear();
	SetAuthenticatedState(true);
	emit UserProfileChanged();
}

void UserSession::Logout()
{
	_authToken.clear();
	_currentUser.clear();
	SetAuthenticatedState(false);
	emit UserProfileChanged();
}

QJ_NAMESPACE_FIT_CLOUD_SERVER_END
