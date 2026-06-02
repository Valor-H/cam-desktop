#pragma once

#include "cloud_server_global.h"

#include <QObject>
#include <QString>
#include <QVariantMap>

QJ_NAMESPACE_FIT_CLOUD_SERVER_BEGIN

class CLOUD_SERVER_EXPORT UserSession final : public QObject
{
	Q_OBJECT

public:
	/** 构造用户会话对象。 */
	explicit UserSession(QObject* parent = nullptr);
	/** 返回当前是否已登录。 */
	bool IsAuthenticated() const { return _authenticated; }
	/** 返回当前认证令牌。 */
	QString AuthToken() const { return _authToken; }
	/** 返回当前用户资料。 */
	QVariantMap CurrentUser() const { return _currentUser; }
	/** 根据登录返回数据更新会话。 */
	void ApplyFromLoginPayload(const QVariantMap& payload);
	/** 根据探测接口数据更新会话。 */
	void ApplyFromProbe(const QVariantMap& data);
	/** 清理当前会话并退出登录。 */
	void Logout();

signals:
	/** 在认证状态变化时发出通知。 */
	void AuthStateChanged(bool authenticated);
	/** 在用户资料变化时发出通知。 */
	void UserProfileChanged();

private:
	/** 设置当前认证状态。 */
	void SetAuthenticatedState(bool on);

	/** 标识当前是否已认证。 */
	bool _authenticated{ false };
	/** 保存当前认证令牌。 */
	QString _authToken;
	/** 保存当前用户资料。 */
	QVariantMap _currentUser;
};

QJ_NAMESPACE_FIT_CLOUD_SERVER_END
