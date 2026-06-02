#include "user_auth_service.h"

#include "auth_http_client.h"
#include "account_auth_dialog.h"
#include "desktop_web.h"
#include "local_auth_sync_channel.h"

#include <QSettings>
#include <QWidget>

QJ_USING_NAMESPACE_FIT_CLOUD_SERVER

namespace
{
	constexpr int kWindowActivateRefreshDebounceMs = 800;
	constexpr int kWindowActivateRefreshThrottleMs = 12000;

	QString apiBaseStringForClient(const QUrl& u)
	{
		QString s = u.toString(QUrl::RemoveQuery | QUrl::RemoveFragment);
		while (s.endsWith(QLatin1Char('/'))) {
			s.chop(1);
		}
		return s;
	}
}

QJ_NAMESPACE_FIT_CLOUD_SERVER_BEGIN

UserAuthService::UserAuthService(const CloudServerConfig& cfg, QObject* parent)
	: QObject(parent)
	, _cfg(cfg)
	, _userSession(this)
	, _authClient(new AuthHttpClient(apiBaseStringForClient(_cfg.apiBaseUrl), this))
	, _authSyncChannel(new LocalAuthSyncChannel(
		_cfg,
		[this](bool authenticated) { ApplyAuthStateChangedFromPeer(authenticated); },
		this))
{
	_windowActivateRefreshDebounceTimer = new QTimer(this);
	_windowActivateRefreshDebounceTimer->setSingleShot(true);
	connect(_windowActivateRefreshDebounceTimer,
		&QTimer::timeout,
		this,
		&UserAuthService::TryRefreshUserProfileOnWindowActivate);
}

UserAuthService::~UserAuthService()
{
	CancelAllPendingRequests();
}

void UserAuthService::CancelAllPendingRequests()
{
	if (_authClient) {
		_authClient->CancelAll();
	}
}

void UserAuthService::BroadcastAuthStateChanged(bool authenticated)
{
	if (!_authSyncChannel) {
		return;
	}
	_authSyncChannel->Broadcast(authenticated);
}

void UserAuthService::ApplyAuthStateChangedFromPeer(bool authenticated)
{
	if (authenticated) {
		SyncSessionFromSharedSettings();
		return;
	}

	if (!LoadAuthTokenFromSettings().isEmpty()) {
		return;
	}

	CancelAllPendingRequests();
	_userHydrationInFlight = false;
	_userSession.Logout();
}

void UserAuthService::SyncSessionFromSharedSettings()
{
	const QString token = LoadAuthTokenFromSettings();
	if (token.isEmpty()) {
		return;
	}

	QVariantMap data;
	data.insert(QStringLiteral("token"), token);
	data.insert(QStringLiteral("loggedIn"), true);
	_userSession.ApplyFromProbe(data);
	StartDirectUserHydration(token, true);
}

void UserAuthService::ShowAccountAuthDialog(QWidget* parent)
{
	const QUrl loginUrl = buildDesktopLoginUrl(_cfg.frontendBaseUrl);
	AccountAuthDialog dlg(parent, loginUrl, this);
	connect(&dlg, &AccountAuthDialog::AuthSucceeded, this, &UserAuthService::OnLoginSucceeded);
	dlg.exec();
}

void UserAuthService::Logout()
{
	CancelAllPendingRequests();
	_userHydrationInFlight = false;
	ClearAuthTokenFromSettings();
	_userSession.Logout();
	BroadcastAuthStateChanged(false);
}

void UserAuthService::OnLoginSucceeded(const QVariantMap& payload)
{
	CancelAllPendingRequests();
	SaveAuthTokenToSettings(payload.value(QStringLiteral("token")).toString());
	_userSession.ApplyFromLoginPayload(payload);
	BroadcastAuthStateChanged(true);
}

void UserAuthService::InitFromStoredToken()
{
	SyncSessionFromSharedSettings();
}

void UserAuthService::BuildExternalWebSsoUrl(const QString& redirectPath, WebSsoUrlCallback callback)
{
	if (!callback) {
		return;
	}

	const QString token = _userSession.AuthToken().trimmed();
	if (token.isEmpty()) {
		callback(QUrl(), QStringLiteral("Not logged in"));
		return;
	}
	if (!_authClient) {
		callback(QUrl(), QStringLiteral("Auth service unavailable"));
		return;
	}

	_authClient->Post(QStringLiteral("/api/auth/ticket/exchange"), token, 10,
		[this, redirectPath, cb = std::move(callback)](const AuthHttpClient::Response& resp) mutable {
			if (!resp.networkOk) {
				cb(QUrl(), QStringLiteral("Network error"));
				return;
			}
			if (resp.bizCode != 200) {
				const QString msg = resp.bizMsg.trimmed().isEmpty()
					? QStringLiteral("Failed to create SSO ticket")
					: resp.bizMsg.trimmed();
				cb(QUrl(), msg);
				return;
			}

			const QString ticket = resp.data.value(QStringLiteral("ticket")).toString().trimmed();
			if (ticket.isEmpty()) {
				cb(QUrl(), QStringLiteral("SSO ticket is missing"));
				return;
			}

			cb(buildExternalSsoLoginUrl(_cfg.externalFrontendBaseUrl, ticket, redirectPath), QString());
		});
}

void UserAuthService::StartDirectUserHydration(const QString& token, bool allowRefresh)
{
	const QString trimmed = token.trimmed();
	if (trimmed.isEmpty()) {
		_userHydrationInFlight = false;
		return;
	}

	CancelAllPendingRequests();
	_userHydrationInFlight = true;
	FetchCurrentUserDirect(trimmed, allowRefresh);
}

void UserAuthService::FetchCurrentUserDirect(const QString& token, bool allowRefresh)
{
	_authClient->Post(QStringLiteral("/api/user/current"), token, 10,
		[this, token, allowRefresh](const AuthHttpClient::Response& resp) {
			if (!resp.networkOk) {
				_userHydrationInFlight = false;
				return;
			}

			if (resp.bizCode == 200) {
				const QVariantMap userMap = resp.data;
				if (userMap.isEmpty()) {
					_userHydrationInFlight = false;
					return;
				}

				QVariantMap payload;
				payload.insert(QStringLiteral("token"), token);
				payload.insert(QStringLiteral("user"), userMap);
				_userSession.ApplyFromLoginPayload(payload);
				_userHydrationInFlight = false;
				return;
			}

			if (resp.bizCode == 401 && allowRefresh) {
				RefreshTokenDirectAndRetry(token);
				return;
			}

			if (resp.bizCode == 401) {
				CancelAllPendingRequests();
				ClearAuthTokenFromSettings();
				_userSession.Logout();
				_userHydrationInFlight = false;
				return;
			}

			_userHydrationInFlight = false;
		});
}

void UserAuthService::RefreshTokenDirectAndRetry(const QString& token)
{
	_authClient->Post(QStringLiteral("/api/auth/refresh"), token, 10,
		[this, token](const AuthHttpClient::Response& resp) {
			if (!resp.networkOk) {
				_userHydrationInFlight = false;
				return;
			}

			if (resp.bizCode == 200) {
				FetchCurrentUserDirect(token, false);
				return;
			}

			if (resp.bizCode == 401) {
				CancelAllPendingRequests();
				ClearAuthTokenFromSettings();
				_userSession.Logout();
				_userHydrationInFlight = false;
				return;
			}

			_userHydrationInFlight = false;
		});
}

void UserAuthService::SaveAuthTokenToSettings(const QString& token)
{
	QSettings settings(_cfg.settingsOrg, _cfg.settingsApp);
	const QString trimmed = token.trimmed();
	if (trimmed.isEmpty()) {
		settings.remove(_cfg.authTokenKey);
	}
	else {
		settings.setValue(_cfg.authTokenKey, trimmed);
	}
	settings.sync();
}

QString UserAuthService::LoadAuthTokenFromSettings() const
{
	QSettings settings(_cfg.settingsOrg, _cfg.settingsApp);
	return settings.value(_cfg.authTokenKey).toString().trimmed();
}

void UserAuthService::ClearAuthTokenFromSettings()
{
	QSettings settings(_cfg.settingsOrg, _cfg.settingsApp);
	settings.remove(_cfg.authTokenKey);
	settings.sync();
}

void UserAuthService::OnWindowActivateEvent()
{
	if (!_userSession.IsAuthenticated()) {
		SyncSessionFromSharedSettings();
	}
	ScheduleWindowActivateRefresh();
}

void UserAuthService::ScheduleWindowActivateRefresh()
{
	if (!_windowActivateRefreshDebounceTimer) {
		return;
	}
	_windowActivateRefreshDebounceTimer->start(kWindowActivateRefreshDebounceMs);
}

void UserAuthService::TryRefreshUserProfileOnWindowActivate()
{
	const QString token = _userSession.AuthToken().trimmed();
	if (token.isEmpty()) {
		return;
	}
	if (_userHydrationInFlight) {
		return;
	}
	if (_lastWindowActivateRefreshAt.isValid()
		&& _lastWindowActivateRefreshAt.elapsed() < kWindowActivateRefreshThrottleMs) {
		return;
	}

	_lastWindowActivateRefreshAt.restart();
	StartDirectUserHydration(token, true);
}

QJ_NAMESPACE_FIT_CLOUD_SERVER_END
