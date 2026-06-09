#include "upload_picker_dialog.h"

#include "desktop_runtime_injection.h"

#include <QByteArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QPalette>
#include <QString>
#include <QUrl>
#include <QVBoxLayout>
#include <QWidget>
#include <QtGlobal>

#include <QCefQuery.h>
#include <QCefSetting.h>
#include <QCefView.h>

namespace {
	constexpr int s_desktopQueryErrorBadRequest = 400;
	const QString s_methodOnUploadTargetSelected = QStringLiteral("Desktop.OnUploadTargetSelected");
	const QString s_methodRequestCloseUploadPicker = QStringLiteral("Desktop.RequestCloseUploadPicker");
	constexpr int s_uploadPickerWidth = 600;
	constexpr int s_uploadPickerHeight = 480;

	bool HasSameOrigin(const QUrl& lhs, const QUrl& rhs)
	{
		return lhs.isValid() && rhs.isValid() && lhs.scheme() == rhs.scheme() && lhs.host() == rhs.host()
			&& lhs.port() == rhs.port();
	}

	QVariantMap SanitizeUploadTargetPayload(const QVariantMap& payload)
	{
		QVariantMap sanitized;
		const QString scope = payload.value(QStringLiteral("scope")).toString().trimmed();
		if (scope != QStringLiteral("personal") && scope != QStringLiteral("team")) {
			return QVariantMap{};
		}

		const QString folder_name = payload.value(QStringLiteral("folderName")).toString().trimmed();
		if (folder_name.isEmpty()) {
			return QVariantMap{};
		}

		sanitized.insert(QStringLiteral("scope"), scope);
		sanitized.insert(QStringLiteral("folderUuid"), payload.value(QStringLiteral("folderUuid")).toString().trimmed());
		sanitized.insert(QStringLiteral("folderName"), folder_name);

		const QString team_uuid = payload.value(QStringLiteral("teamUuid")).toString().trimmed();
		if (scope == QStringLiteral("team")) {
			if (team_uuid.isEmpty()) {
				return QVariantMap{};
			}
			sanitized.insert(QStringLiteral("teamUuid"), team_uuid);
		}

		return sanitized;
	}
}

QJ_NAMESPACE_FIT_CLOUD_SERVER_BEGIN

UploadPickerDialog::UploadPickerDialog(QWidget* parent, const QUrl& page_url, UserAuthService* auth_service)
	: QDialog(parent)
	, _view(nullptr)
	, _currentUrl(page_url)
	, _pageUrl(page_url)
	, _authService(auth_service)
{
	setWindowTitle(tr("Upload"));
	setFixedSize(s_uploadPickerWidth, s_uploadPickerHeight);
	setAutoFillBackground(true);
	{
		QPalette pal = palette();
		pal.setColor(QPalette::Window, Qt::white);
		setPalette(pal);
	}

	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);

	QCefSetting setting;
	setting.setBackgroundColor(QColor(Qt::white));
	_view = new QCefView(_pageUrl.toString(), &setting, this);
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
		[this](const QCefBrowserId&, const QCefFrameId&, const QCefQuery& query) { OnCefQueryRequest(query); });
}

UploadPickerDialog::~UploadPickerDialog() = default;

bool UploadPickerDialog::IsTrustedUiSource() const
{
	return HasSameOrigin(_currentUrl, _pageUrl);
}

void UploadPickerDialog::InjectDesktopRuntime()
{
	InjectDesktopRuntimeIntoView(_view, _authService, _currentUrl.toString());
}

void UploadPickerDialog::OnAddressChanged(const QString& url)
{
	_currentUrl = QUrl(url);
}

void UploadPickerDialog::OnLoadStart()
{
	InjectDesktopRuntime();
}

void UploadPickerDialog::OnLoadEnd(int http_status_code)
{
	Q_UNUSED(http_status_code);
	InjectDesktopRuntime();
}

void UploadPickerDialog::OnCefQueryRequest(const QCefQuery& query)
{
	if (!_view || !IsTrustedUiSource()) {
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

	if (method == s_methodRequestCloseUploadPicker) {
		QCefQuery success_query = query;
		success_query.reply(true, QStringLiteral("{}"));
		_view->responseQCefQuery(success_query);
		reject();
		return;
	}

	if (method != s_methodOnUploadTargetSelected) {
		QCefQuery invalid_query = query;
		invalid_query.reply(false, QStringLiteral("Unsupported desktop query method."), s_desktopQueryErrorBadRequest);
		_view->responseQCefQuery(invalid_query);
		return;
	}

	const QVariantMap sanitized = SanitizeUploadTargetPayload(payload);
	if (sanitized.isEmpty()) {
		QCefQuery invalid_query = query;
		invalid_query.reply(false, QStringLiteral("Upload target payload is invalid."), s_desktopQueryErrorBadRequest);
		_view->responseQCefQuery(invalid_query);
		return;
	}

	QCefQuery success_query = query;
	success_query.reply(true, QStringLiteral("{}"));
	_view->responseQCefQuery(success_query);
	emit UploadTargetSelected(sanitized);
	accept();
}

QJ_NAMESPACE_FIT_CLOUD_SERVER_END
