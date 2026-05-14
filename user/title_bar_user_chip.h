#pragma once

#include "user_global.h"

#include <QUrl>
#include <QWidget>

class SARibbonToolButton;
class QNetworkAccessManager;
class QNetworkReply;

QJ_NAMESPACE_FIT_USER_BEGIN
class UserSession;

class USER_EXPORT TitleBarUserChip final : public QWidget
{
    Q_OBJECT

public:
    static constexpr int kAvatarButtonSide = 20;
    static constexpr int kAvatarIconSide = 18;

    explicit TitleBarUserChip(QWidget* parent, const QUrl& apiBaseUrl);
    void SyncFromSession(const UserSession* session);
    void RelayoutInParent();

signals:
    void loginRequested();
    void accountMenuRequested();

private slots:
    void OnAvatarDownloadFinished(QNetworkReply* reply);

private:
    void AbortAvatarRequest();
    void ApplyDefaultAvatar();
    void ApplyLoggedInAppearance(const UserSession* session);
    QPixmap MakeInitialAvatarWithRing(const QString& nickName, const QString& userName) const;
    static QString PickInitialChar(const QString& nickName, const QString& userName);
    QPixmap MakeCircularAvatar(const QPixmap& source) const;
    QUrl ResolveAvatarUrl(const QString& raw) const;
    static QPixmap LoadAvatarRaster(const char* resourcePath, int side);

    QUrl _apiBaseUrl;

    SARibbonToolButton* _avatarButton { nullptr };
    QNetworkAccessManager* _nam { nullptr };
    QNetworkReply* _avatarReply { nullptr };
    bool _loggedIn { false };
    QString _fallbackNickName;
    QString _fallbackUserName;
};

QJ_NAMESPACE_FIT_USER_END


