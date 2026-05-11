#include "user_session.h"

namespace
{
const QString kTokenKey = QStringLiteral("token");
const QString kUserKey = QStringLiteral("user");
const QString kLoggedInKey = QStringLiteral("loggedIn");
}

QJ_NAMESPACE_FIT_USER_BEGIN

UserSession::UserSession(QObject* parent)
    : QObject(parent)
{}

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
    const QString token = payload.value(kTokenKey).toString().trimmed();
    if (token.isEmpty()) {
        _authToken.clear();
        _currentUser.clear();
        SetAuthenticatedState(false);
        emit UserProfileChanged();
        return;
    }

    const QVariantMap user = payload.value(kUserKey).toMap();
    _authToken = token;
    if (!user.isEmpty()) {
        _currentUser = user;
    }
    SetAuthenticatedState(true);
    emit UserProfileChanged();
}

void UserSession::ApplyFromProbe(const QVariantMap& data)
{
    const bool loggedIn = data.value(kLoggedInKey).toBool();
    if (!loggedIn) {
        _authToken.clear();
        _currentUser.clear();
        SetAuthenticatedState(false);
        emit UserProfileChanged();
        return;
    }

    const QString token = data.value(kTokenKey).toString().trimmed();
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

QJ_NAMESPACE_FIT_USER_END
