#pragma once

#include "UserModuleConfig.h"
#include "UserSession.h"
#include "user_global.h"

#include <QElapsedTimer>
#include <functional>
#include <QObject>
#include <QTimer>
#include <QUrl>

class AuthHttpClient;
class QWidget;

#ifdef Q_MOC_RUN
namespace qianjizn { namespace user {
#else
QJ_NAMESPACE_FIT_USER_BEGIN
#endif

class USER_EXPORT UserAuthService final : public QObject
{
    Q_OBJECT

public:
    using WebSsoUrlCallback = std::function<void(const QUrl& url, const QString& errorMessage)>;

    explicit UserAuthService(const UserModuleConfig& cfg, QObject* parent = nullptr);
    ~UserAuthService() override;

    UserSession* Session() { return &_userSession; }
    const UserSession* Session() const { return &_userSession; }

    const UserModuleConfig& Config() const { return _cfg; }
    QUrl ApiBaseUrl() const { return _cfg.apiBaseUrl; }
    QUrl FrontendBaseUrl() const { return _cfg.frontendBaseUrl; }
    QUrl ExternalFrontendBaseUrl() const { return _cfg.externalFrontendBaseUrl; }
    void SetFrontendBaseUrl(const QUrl& url) { _cfg.frontendBaseUrl = url; }

    void CancelAllPendingRequests();

    void ShowAccountAuthDialog(QWidget* parent);
    void Logout();
    void InitFromStoredToken();
    void OnWindowActivateEvent();
    void BuildExternalWebSsoUrl(const QString& redirectPath, WebSsoUrlCallback callback);

private slots:
    void OnLoginSucceeded(const QVariantMap& payload);
    void TryRefreshUserProfileOnWindowActivate();

private:
    void ScheduleWindowActivateRefresh();
    void SaveAuthTokenToSettings(const QString& token);
    QString LoadAuthTokenFromSettings() const;
    void ClearAuthTokenFromSettings();

    void StartDirectUserHydration(const QString& token, bool allowRefresh);
    void FetchCurrentUserDirect(const QString& token, bool allowRefresh);
    void RefreshTokenDirectAndRetry(const QString& token);

    UserModuleConfig _cfg;
    UserSession _userSession;
    AuthHttpClient* _authClient { nullptr };
    QTimer* _windowActivateRefreshDebounceTimer { nullptr };
    QElapsedTimer _lastWindowActivateRefreshAt;
    bool _userHydrationInFlight { false };
};

#ifdef Q_MOC_RUN
} }
#else
QJ_NAMESPACE_FIT_USER_END
#endif
