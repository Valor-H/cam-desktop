#include "account_auth_dialog.h"

#include "desktop_runtime_injection.h"
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

namespace {
	const QString s_methodOnLoginSuccess = QStringLiteral("Desktop.OnLoginSuccess");
}

QJ_NAMESPACE_FIT_CLOUD_SERVER_BEGIN

AccountAuthDialog::AccountAuthDialog(
	QWidget* parent, const QUrl& auth_page_url, UserAuthService* auth_service)
	: QDialog(parent)
	, _view(nullptr)
	, _currentUrl()
	, _authPageUrl(auth_page_url)
	, _authService(auth_service)
{
	setWindowTitle(tr("Login"));

	setFixedSize(450, 550);

	setAutoFillBackground(true);
	{
		QPalette pal = palette();
		pal.setColor(QPalette::Window, Qt::white);
		setPalette(pal);
	}

	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);

	const QString start_url = _authPageUrl.toString();
	_currentUrl = _authPageUrl;

	QCefSetting setting;
	setting.setBackgroundColor(QColor(Qt::white));
	_view = new QCefView(start_url, &setting, this);
	_view->setAutoFillBackground(true);
	_view->setPalette(palette());

	layout->addWidget(_view);

	connect(_view,
		&QCefView::loadStart,
		this,
		[this](const QCefBrowserId&, const QCefFrameId&, bool is_main_frame, int) {
			if (is_main_frame) {
				OnLoadStart();
			}
		});
	connect(_view,
		&QCefView::addressChanged,
		this,
		[this](const QCefFrameId&, const QString& url) { OnAddressChanged(url); });
	connect(_view,
		&QCefView::loadEnd,
		this,
		[this](const QCefBrowserId&, const QCefFrameId&, bool is_main_frame, int http_status_code) {
			if (is_main_frame) {
				OnLoadEnd(http_status_code);
			}
		});
	connect(_view,
		&QCefView::cefQueryRequest,
		this,
		[this](const QCefBrowserId&, const QCefFrameId&, const QCefQuery& query) {
			OnCefQueryRequest(query);
		});
}

AccountAuthDialog::~AccountAuthDialog()
{
}

void AccountAuthDialog::InjectDesktopRuntime()
{
	InjectDesktopRuntimeIntoView(_view, _authService, _currentUrl.toString());
}

bool AccountAuthDialog::IsTrustedUiSource() const
{
	return LoginWebAuth::IsTrustedUiSource(_currentUrl, _authPageUrl);
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

void AccountAuthDialog::OnLoadStart()
{
	InjectDesktopRuntime();
}

void AccountAuthDialog::UpdateUiFromUrl(const QUrl& url)
{
	_currentUrl = url;
	if (!IsTrustedUiSource()) {
		return;
	}
	SyncWindowTitleFromCurrentUrl();
}

void AccountAuthDialog::SyncWindowTitleFromCurrentUrl()
{
	using LoginWebAuth::AuthRoute;
	const AuthRoute route = LoginWebAuth::RouteFromPath(LoginWebAuth::ExtractAuthRoutePath(_currentUrl));
	switch (route) {
	case AuthRoute::REGISTER:
		setWindowTitle(tr("Register"));
		return;
	case AuthRoute::RESET:
		setWindowTitle(tr("Reset password"));
		return;
	case AuthRoute::LOGIN:
	case AuthRoute::UNKNOWN:
	default:
		setWindowTitle(tr("Login"));
		return;
	}
}

void AccountAuthDialog::OnLoadEnd(int http_status_code)
{
	Q_UNUSED(http_status_code);
	InjectDesktopRuntime();
	UpdateUiFromUrl(_currentUrl);
}

void AccountAuthDialog::OnCefQueryRequest(const QCefQuery& query)
{
	if (!_view || !LoginWebAuth::IsTrustedInvokeSource(_currentUrl, _authPageUrl)) {
		return;
	}

	QString method;
	QVariantMap payload;
	const QByteArray raw_request = query.request().toUtf8();
	QJsonParseError parse_error{};
	const QJsonDocument document = QJsonDocument::fromJson(raw_request, &parse_error);
	if (parse_error.error == QJsonParseError::NoError && document.isObject()) {
		const QJsonObject request_object = document.object();
		method = request_object.value(QStringLiteral("method")).toString().trimmed();
		payload = request_object.value(QStringLiteral("payload")).toObject().toVariantMap();
	}
	else {
		method = QString::fromUtf8(raw_request).trimmed();
	}

	if (method != s_methodOnLoginSuccess) {
		return;
	}

	HandleAuthSucceeded(LoginWebAuth::SanitizeLoginPayload(payload));

	QCefQuery successQuery = query;
	successQuery.reply(true, QStringLiteral("{}"));
	_view->responseQCefQuery(successQuery);
}

QJ_NAMESPACE_FIT_CLOUD_SERVER_END
