#pragma once

#include "user_global.h"

#include <QDialog>
#include <QString>
#include <QVariantMap>
#include <QUrl>

class QCefQuery;
class QCefView;
class QWidget;

QJ_NAMESPACE_FIT_USER_BEGIN
class UserAuthService;
QJ_NAMESPACE_FIT_USER_END

class AccountAuthDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AccountAuthDialog(
        QWidget* parent, const QUrl& authPageUrl, qianjizn::user::UserAuthService* authService);
    ~AccountAuthDialog() override;

signals:
    void AuthSucceeded(const QVariantMap& payload);

private slots:
    void OnAddressChanged(const QString& url);
    void OnLoadStart();
    void OnLoadEnd(int httpStatusCode);
    void OnCefQueryRequest(const QCefQuery& query);

private:
    bool IsTrustedUiSource() const;
    void HandleAuthSucceeded(const QVariantMap& payload);
    void InjectDesktopRuntime();
    void SyncWindowTitleFromCurrentUrl();
    void UpdateUiFromUrl(const QUrl& url);

    QCefView* m_view { nullptr };
    QUrl m_currentUrl;
    QUrl m_authPageUrl;
    qianjizn::user::UserAuthService* m_authService { nullptr };
};
