#pragma once

#include "cloud_server_config.h"
#include "user_session.h"
#include "cloud_server_global.h"

#include <QElapsedTimer>
#include <functional>
#include <QObject>
#include <QTimer>
#include <QUrl>

class AuthHttpClient;
class QWidget;

QJ_NAMESPACE_FIT_CLOUD_SERVER_BEGIN

class LocalAuthSyncChannel;

class CLOUD_SERVER_EXPORT UserAuthService final : public QObject
{
	Q_OBJECT

public:
	/** 定义外部单点登录地址回调类型。 */
	using WebSsoUrlCallback = std::function<void(const QUrl& url, const QString& errorMessage)>;
	/** 构造用户认证服务。 */
	explicit UserAuthService(const CloudServerConfig& cfg, QObject* parent = nullptr);
	/** 析构用户认证服务。 */
	~UserAuthService() override;
	/** 返回可写的当前用户会话对象。 */
	UserSession* Session() { return &_userSession; }
	/** 返回只读的当前用户会话对象。 */
	const UserSession* Session() const { return &_userSession; }
	/** 返回当前模块配置。 */
	const CloudServerConfig& Config() const { return _cfg; }
	/** 返回接口基础地址。 */
	QUrl ApiBaseUrl() const { return _cfg.apiBaseUrl; }
	/** 返回前端基础地址。 */
	QUrl FrontendBaseUrl() const { return _cfg.frontendBaseUrl; }
	/** 设置前端基础地址。 */
	void SetFrontendBaseUrl(const QUrl& url) { _cfg.frontendBaseUrl = url; }
	/** 取消全部待处理认证请求。 */
	void CancelAllPendingRequests();
	/** 显示账号认证对话框。 */
	void ShowAccountAuthDialog(QWidget* parent);
	/** 执行当前用户退出登录。 */
	void Logout();
	/** 使用已存储令牌初始化登录状态。 */
	void InitFromStoredToken();
	/** 处理窗口激活事件。 */
	void OnWindowActivateEvent();
	/** 构建外部网页登录单点登录地址。 */
	void BuildExternalWebSsoUrl(const QString& redirectPath, WebSsoUrlCallback callback);

private slots:
	/** 处理网页登录成功事件。 */
	void OnLoginSucceeded(const QVariantMap& payload);
	/** 在窗口激活时尝试刷新用户资料。 */
	void TryRefreshUserProfileOnWindowActivate();

private:
	/** 向其他进程广播认证状态变化。 */
	void BroadcastAuthStateChanged(bool authenticated);
	/** 应用来自其他进程的认证状态变化。 */
	void ApplyAuthStateChangedFromPeer(bool authenticated);
	/** 从共享设置同步当前实例的认证状态。 */
	void SyncSessionFromSharedSettings();
	/** 安排窗口激活后的资料刷新。 */
	void ScheduleWindowActivateRefresh();
	/** 将认证令牌保存到设置。 */
	void SaveAuthTokenToSettings(const QString& token);
	/** 从设置中加载认证令牌。 */
	QString LoadAuthTokenFromSettings() const;
	/** 清除设置中的认证令牌。 */
	void ClearAuthTokenFromSettings();
	/** 直接使用令牌开始用户信息填充。 */
	void StartDirectUserHydration(const QString& token, bool allowRefresh);
	/** 直接获取当前用户信息。 */
	void FetchCurrentUserDirect(const QString& token, bool allowRefresh);
	/** 直接刷新令牌并重试请求。 */
	void RefreshTokenDirectAndRetry(const QString& token);
	/** 保存当前模块配置。 */
	CloudServerConfig _cfg;
	/** 保存当前用户会话。 */
	UserSession _userSession;
	/** 保存认证 HTTP 客户端。 */
	AuthHttpClient* _authClient{ nullptr };
	/** 保存跨进程认证同步通道。 */
	LocalAuthSyncChannel* _authSyncChannel{ nullptr };
	/** 保存窗口激活刷新防抖定时器。 */
	QTimer* _windowActivateRefreshDebounceTimer{ nullptr };
	/** 记录上次窗口激活刷新时间。 */
	QElapsedTimer _lastWindowActivateRefreshAt;
	/** 标识用户信息填充是否正在进行。 */
	bool _userHydrationInFlight{ false };
};

QJ_NAMESPACE_FIT_CLOUD_SERVER_END
