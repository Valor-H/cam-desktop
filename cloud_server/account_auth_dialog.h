#pragma once

#include "cloud_server_global.h"

#include <QDialog>
#include <QString>
#include <QVariantMap>
#include <QUrl>

class QCefQuery;
class QCefView;
class QWidget;

QJ_NAMESPACE_FIT_CLOUD_SERVER_BEGIN
class UserAuthService;

class AccountAuthDialog : public QDialog
{
	Q_OBJECT

public:
	/** 构造账号认证对话框。 */
	explicit AccountAuthDialog(
		QWidget* parent, const QUrl& auth_page_url, UserAuthService* auth_service);
	/** 析构账号认证对话框。 */
	~AccountAuthDialog() override;


signals:
	/** 在认证成功后发送登录结果数据。 */
	void AuthSucceeded(const QVariantMap& payload);

private slots:
	/** 处理地址栏 URL 变化。 */
	void OnAddressChanged(const QString& url);
	/** 处理页面开始加载事件。 */
	void OnLoadStart();
	/** 处理页面加载完成事件。 */
	void OnLoadEnd(int http_status_code);
	/** 处理来自网页端的 CEF 查询请求。 */
	void OnCefQueryRequest(const QCefQuery& query);

private:
	/** 判断当前页面来源是否可信。 */
	bool IsTrustedUiSource() const;
	/** 处理认证成功后的业务逻辑。 */
	void HandleAuthSucceeded(const QVariantMap& payload);
	/** 向网页注入桌面运行时对象。 */
	void InjectDesktopRuntime();
	/** 根据当前 URL 同步窗口标题。 */
	void SyncWindowTitleFromCurrentUrl();
	/** 根据 URL 更新界面状态。 */
	void UpdateUiFromUrl(const QUrl& url);
	/** 内嵌认证页面的浏览器视图。 */
	QCefView* _view;
	/** 当前正在显示的页面 URL。 */
	QUrl _currentUrl;
	/** 认证页面的初始 URL。 */
	QUrl _authPageUrl;
	/** 用户认证服务实例。 */
	UserAuthService* _authService;
};

QJ_NAMESPACE_FIT_CLOUD_SERVER_END
