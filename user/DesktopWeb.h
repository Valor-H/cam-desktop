#pragma once

#include "user_global.h"

#include <QString>
#include <QUrl>
#include <QUrlQuery>

QJ_NAMESPACE_FIT_USER_BEGIN
inline QUrl buildDesktopLoginUrl(const QUrl& frontendBase)
{
    QUrl url = frontendBase;
    if (!url.path().endsWith(QLatin1Char('/'))) {
        url.setPath(url.path() + QLatin1Char('/'));
    }
    QUrlQuery query(url);
    query.removeAllQueryItems(QStringLiteral("source"));
    query.addQueryItem(QStringLiteral("source"), QStringLiteral("desktop"));
    url.setQuery(query);
    return url;
}

inline QUrl buildPersonalProfileUrl(const QUrl& frontendBase)
{
    return frontendBase.resolved(QUrl(QStringLiteral("profile-personal")));
}

inline QUrl buildTeamUrl(const QUrl& frontendBase)
{
    return frontendBase.resolved(QUrl(QStringLiteral("team")));
}

inline QUrl buildExternalSsoLoginUrl(const QUrl& frontendBase, const QString& ticket, const QString& redirectPath)
{
    QUrl url = frontendBase.resolved(QUrl(QStringLiteral("sso-login")));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("ticket"), ticket.trimmed());
    QString normalizedRedirect = redirectPath.trimmed();
    if (normalizedRedirect.isEmpty()) {
        normalizedRedirect = QStringLiteral("/profile-personal");
    } else if (!normalizedRedirect.startsWith(QLatin1Char('/'))) {
        normalizedRedirect.prepend(QLatin1Char('/'));
    }
    query.addQueryItem(QStringLiteral("redirect"), normalizedRedirect);
    url.setQuery(query);
    return url;
}

inline QUrl buildFileManagerUrl(const QUrl& frontendBase, const QString& /*authToken*/)
{
    QUrl url = frontendBase.resolved(QUrl(QStringLiteral("local-files")));
    QUrlQuery query(url);
    query.removeAllQueryItems(QStringLiteral("source"));
    query.addQueryItem(QStringLiteral("source"), QStringLiteral("desktop"));
    url.setQuery(query);
    return url;
}

inline QUrl buildRecentFilesUrl(const QUrl& frontendBase)
{
    QUrl url = frontendBase.resolved(QUrl(QStringLiteral("recent-files")));
    QUrlQuery query(url);
    query.removeAllQueryItems(QStringLiteral("source"));
    query.addQueryItem(QStringLiteral("source"), QStringLiteral("desktop"));
    url.setQuery(query);
    return url;
}
QJ_NAMESPACE_FIT_USER_END
