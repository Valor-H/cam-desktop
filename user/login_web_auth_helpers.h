#pragma once

#include <QUrl>
#include <QVariantMap>

namespace LoginWebAuth
{
enum class AuthRoute
{
    Login,
    Register,
    Reset,
    Unknown
};

QString ExtractAuthRoutePath(const QUrl& url);
AuthRoute RouteFromPath(const QString& path);
QVariantMap SanitizeLoginPayload(const QVariantMap& input);

bool IsTrustedUiSource(const QUrl& currentUrl, const QUrl& loginPageUrl);
bool IsTrustedInvokeSource(const QUrl& currentUrl, const QUrl& loginPageUrl);
}
