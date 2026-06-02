#pragma once

#include "cloud_server_global.h"

#include <QByteArray>
#include <QUrl>
#include <QWidget>

#include <atomic>
#include <memory>

class QToolButton;

QJ_NAMESPACE_FIT_CLOUD_SERVER_BEGIN
class UserSession;

class CLOUD_SERVER_EXPORT TitleBarUserChip final : public QWidget
{
	Q_OBJECT

public:
	/** 定义头像按钮边长。 */
	static constexpr int s_avatarButtonSide = 20;
	/** 定义头像图标边长。 */
	static constexpr int s_avatarIconSide = 18;
	/** 构造标题栏用户信息组件。 */
	explicit TitleBarUserChip(QWidget* parent, const QUrl& api_base_url);
	/** 根据会话信息同步显示状态。 */
	void SyncFromSession(const UserSession* session);
	/** 重新在父部件中布局自身。 */
	void RelayoutInParent();

signals:
	/** 请求显示登录入口。 */
	void loginRequested();
	/** 请求显示账号菜单。 */
	void accountMenuRequested();

private:
	/** 刷新头像按钮可视状态。 */
	void RefreshAvatarVisuals();
	/** 应用头像图标并刷新显示。 */
	void ApplyAvatarIcon(const QPixmap& pixmap);
	/** 应用昵称首字母兜底头像。 */
	void ApplyFallbackAvatar();
	/** 中止当前头像下载请求。 */
	void AbortAvatarRequest();
	/** 发起头像下载请求。 */
	void StartAvatarDownload(const QUrl& url);
	/** 处理头像下载结果。 */
	void ApplyAvatarDownloadResult(bool network_ok,
		int http_status,
		const QString& request_url,
		const QString& error_message,
		const QByteArray& body);
	/** 应用默认头像显示。 */
	void ApplyDefaultAvatar();
	/** 已登录但资料尚未拉取完成时应用中性默认头像。 */
	void ApplyPendingProfileAvatar();
	/** 应用已登录状态外观。 */
	void ApplyLoggedInAppearance(const UserSession* session);
	/** 生成带圆环的默认头像。 */
	QPixmap MakeInitialAvatarWithRing(const QString& nick_name) const;
	/** 选取头像显示首字符。 */
	static QString PickInitialChar(const QString& nick_name);
	/** 生成圆形头像图像。 */
	QPixmap MakeCircularAvatar(const QPixmap& source) const;
	/** 解析头像资源地址。 */
	QUrl ResolveAvatarUrl(const QString& raw) const;
	/** 从资源中加载头像位图。 */
	static QPixmap LoadAvatarRaster(const char* resource_path, int side);
	/** 保存接口基础地址。 */
	QUrl _apiBaseUrl;
	/** 保存头像按钮实例。 */
	QToolButton* _avatarButton;
	/** 保存头像下载取消代次。 */
	std::shared_ptr<std::atomic<quint64>> _avatarRequestEpoch;
	/** 标识当前是否处于登录状态。 */
	bool _loggedIn;
	/** 保存兜底昵称。 */
	QString _fallbackNickName;
};

QJ_NAMESPACE_FIT_CLOUD_SERVER_END

