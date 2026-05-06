#include "LoginWebAuthHelpers.h"

namespace LoginWebAuth
{
QString ExtractAuthRoutePath(const QUrl& url)
{
    QString path = url.path().trimmed();
    if (!path.isEmpty() && path != QLatin1String("/")) {
        return path;
    }

    const QString frag = url.fragment(QUrl::FullyDecoded).trimmed();
    if (frag.startsWith(QLatin1Char('/'))) {
        const int q = frag.indexOf(QLatin1Char('?'));
        return q >= 0 ? frag.left(q) : frag;
    }
    return path;
}

AuthRoute RouteFromPath(const QString& path)
{
    QString p = path.trimmed();
    if (p.isEmpty() || p == QLatin1Char('/')) {
        return AuthRoute::Login;
    }
    if (p.size() > 1 && p.endsWith(QLatin1Char('/'))) {
        p.chop(1);
    }

    if (p == QStringLiteral("/login")) {
        return AuthRoute::Login;
    }
    if (p == QStringLiteral("/register")) {
        return AuthRoute::Register;
    }
    if (p == QStringLiteral("/reset-password")) {
        return AuthRoute::Reset;
    }

    return AuthRoute::Unknown;
}

QVariantMap SanitizeLoginPayload(const QVariantMap& input)
{
    QVariantMap sanitized;
    sanitized.insert(QStringLiteral("token"), input.value(QStringLiteral("token")).toString());

    const QVariantMap user = input.value(QStringLiteral("user")).toMap();
    if (!user.isEmpty()) {
        QVariantMap u;
        u.insert(QStringLiteral("uuid"), user.value(QStringLiteral("uuid")).toString());
        u.insert(QStringLiteral("userName"), user.value(QStringLiteral("userName")).toString());
        u.insert(QStringLiteral("nickName"), user.value(QStringLiteral("nickName")).toString());
        u.insert(QStringLiteral("email"), user.value(QStringLiteral("email")).toString());
        u.insert(QStringLiteral("phone"), user.value(QStringLiteral("phone")).toString());
        u.insert(QStringLiteral("sex"), user.value(QStringLiteral("sex")).toInt());
        u.insert(QStringLiteral("avatar"), user.value(QStringLiteral("avatar")).toString());
        u.insert(QStringLiteral("role"), user.value(QStringLiteral("role")).toInt());
        sanitized.insert(QStringLiteral("user"), u);
    }
    return sanitized;
}

bool IsTrustedUiSource(const QUrl& currentUrl, const QUrl& loginPageUrl)
{
    if (!currentUrl.isValid() || !loginPageUrl.isValid()) {
        return false;
    }
    return currentUrl.scheme() == loginPageUrl.scheme() && currentUrl.host() == loginPageUrl.host()
        && currentUrl.port() == loginPageUrl.port();
}

bool IsTrustedInvokeSource(const QUrl& currentUrl, const QUrl& loginPageUrl)
{
    if (!IsTrustedUiSource(currentUrl, loginPageUrl)) {
        return false;
    }
    const AuthRoute route = RouteFromPath(ExtractAuthRoutePath(currentUrl));
    return route == AuthRoute::Login || route == AuthRoute::Register || route == AuthRoute::Reset;
}
}
