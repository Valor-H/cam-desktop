#pragma once

#include "cloud_server_global.h"

#include <QString>
#include <QUrl>

QJ_NAMESPACE_FIT_CLOUD_SERVER_BEGIN

struct CLOUD_SERVER_EXPORT CloudServerConfig
{
    /** 保存接口基础地址。 */
    QUrl apiBaseUrl { QStringLiteral("http://localhost:8080") };
    /** 保存 WebSocket 服务地址。 */
    QUrl websocketUrl { QStringLiteral("ws://localhost:8091/ws") };
    /** 保存帮助文档地址。 */
    QUrl helpDocUrl { QStringLiteral("http://localhost:4173") };
    /** 保存桌面端前端基础地址。 */
    QUrl frontendBaseUrl { QStringLiteral("http://localhost:31870/") };
    /** 保存外部前端基础地址。 */
    QUrl externalFrontendBaseUrl { QStringLiteral("http://localhost:5173/") };
    /** 保存模拟服务地址。 */
    QUrl mockServiceUrl { QStringLiteral("http://localhost:3001") };
    /** 保存设置组织名称。 */
    QString settingsOrg { QStringLiteral("QianJiZN") };
    /** 保存设置应用名称。 */
    QString settingsApp { QStringLiteral("QJCAM") };
    /** 保存认证令牌键名。 */
    QString authTokenKey { QStringLiteral("auth/token") };
};

QJ_NAMESPACE_FIT_CLOUD_SERVER_END
