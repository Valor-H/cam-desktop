#include "account_auth_dialog.h"

#include "desktop_auth_messages.h"
#include "login_web_auth_helpers.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QByteArray>
#include <QUrl>
#include <QVariantList>
#include <QVBoxLayout>
#include <QString>
#include <QWidget>
#include <QPalette>

#include <QCefQuery.h>
#include <QCefSetting.h>
#include <QCefView.h>

AccountAuthDialog::AccountAuthDialog(QWidget* parent, const QUrl& authPageUrl)
    : QDialog(parent)
    , m_authPageUrl(authPageUrl)
{
    setWindowTitle(tr("Login"));

    setFixedSize(450, 550);

    setAutoFillBackground(true);
    {
        QPalette pal = palette();
        pal.setColor(QPalette::Window, Qt::white);
        setPalette(pal);
    }

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    const QString startUrl = m_authPageUrl.toString();
    m_currentUrl = m_authPageUrl;

    QCefSetting setting;
    setting.setBackgroundColor(QColor(Qt::white));
    m_view = new QCefView(startUrl, &setting, this);
    m_view->setAutoFillBackground(true);
    m_view->setPalette(palette());

    layout->addWidget(m_view);

    connect(m_view,
            &QCefView::addressChanged,
            this,
            [this](const QCefFrameId&, const QString& url) { OnAddressChanged(url); });
    connect(m_view,
            &QCefView::loadEnd,
            this,
            [this](const QCefBrowserId&, const QCefFrameId&, bool isMainFrame, int httpStatusCode) {
                if (isMainFrame) {
                    OnLoadEnd(httpStatusCode);
                }
            });
    connect(m_view,
            &QCefView::cefQueryRequest,
            this,
            [this](const QCefBrowserId&, const QCefFrameId&, const QCefQuery& query) {
                OnCefQueryRequest(query);
            });
}

AccountAuthDialog::~AccountAuthDialog()
{}

bool AccountAuthDialog::IsTrustedUiSource() const
{
    return LoginWebAuth::IsTrustedUiSource(m_currentUrl, m_authPageUrl);
}

void AccountAuthDialog::HandleAuthSucceeded(const QVariantMap& payload)
{
    emit AuthSucceeded(payload);
    accept();
}

void AccountAuthDialog::OnAddressChanged(const QString& url)
{
    UpdateUiFromUrl(QUrl(url));
}

void AccountAuthDialog::UpdateUiFromUrl(const QUrl& url)
{
    m_currentUrl = url;
    if (!IsTrustedUiSource()) {
        return;
    }
    SyncWindowTitleFromCurrentUrl();
}

void AccountAuthDialog::SyncWindowTitleFromCurrentUrl()
{
    using LoginWebAuth::AuthRoute;
    const AuthRoute route = LoginWebAuth::RouteFromPath(LoginWebAuth::ExtractAuthRoutePath(m_currentUrl));
    switch (route) {
        case AuthRoute::Register:
            setWindowTitle(tr("Register"));
            return;
        case AuthRoute::Reset:
            setWindowTitle(tr("Reset password"));
            return;
        case AuthRoute::Login:
        case AuthRoute::Unknown:
        default:
            setWindowTitle(tr("Login"));
            return;
    }
}

void AccountAuthDialog::OnLoadEnd(int httpStatusCode)
{
    Q_UNUSED(httpStatusCode);
    UpdateUiFromUrl(m_currentUrl);
}

void AccountAuthDialog::OnCefQueryRequest(const QCefQuery& query)
{
    if (!m_view || !LoginWebAuth::IsTrustedInvokeSource(m_currentUrl, m_authPageUrl)) {
        return;
    }

    QString method;
    QVariantMap payload;
    const QByteArray rawRequest = query.request().toUtf8();
    QJsonParseError parseError {};
    const QJsonDocument document = QJsonDocument::fromJson(rawRequest, &parseError);
    if (parseError.error == QJsonParseError::NoError && document.isObject()) {
        const QJsonObject requestObject = document.object();
        method = requestObject.value(QStringLiteral("method")).toString().trimmed();
        payload = requestObject.value(QStringLiteral("payload")).toObject().toVariantMap();
    } else {
        method = QString::fromUtf8(rawRequest).trimmed();
    }

    if (method != DesktopAuthMessages::MethodOnLoginSuccess()) {
        return;
    }

    HandleAuthSucceeded(LoginWebAuth::SanitizeLoginPayload(payload));

    QCefQuery successQuery = query;
    successQuery.reply(true, QStringLiteral("{}"));
    m_view->responseQCefQuery(successQuery);
}
