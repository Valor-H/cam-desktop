#pragma once

#include "user_global.h"

#include <QString>
#include <QUrl>

QJ_NAMESPACE_FIT_USER_BEGIN

struct USER_EXPORT UserModuleConfig
{
    QUrl apiBaseUrl { QStringLiteral("http://localhost:8080/") };
    QUrl websocketUrl { QStringLiteral("ws://localhost:8091/ws") };
    QUrl helpDocUrl { QStringLiteral("http://localhost:4173") };
    QUrl frontendBaseUrl { QStringLiteral("http://localhost:31870/") };
    QUrl externalFrontendBaseUrl { QStringLiteral("http://localhost:5173/") };
    QUrl mockServiceUrl { QStringLiteral("http://localhost:3001") };
    QString settingsOrg { QStringLiteral("QianJiZN") };
    QString settingsApp { QStringLiteral("CamDemo") };
    QString authTokenKey { QStringLiteral("auth/token") };
};

QJ_NAMESPACE_FIT_USER_END
