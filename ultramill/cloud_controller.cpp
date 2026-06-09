#include "cloud_controller.h"

#include "nmainwindow.h"
#ifdef ENABLE_CLOUD_SERVER_MODULE
#include <cloud_server/cloud_file_state.h>
#include <cloud_server/desktop_web.h>
#include <cloud_server/desktop_web_server.h>
#include <cloud_server/file_manager_view.h>
#include <cloud_server/open_request_context.h>
#include <cloud_server/title_bar_user_chip.h>
#include <cloud_server/upload_picker_dialog.h>
#include <SARibbonBar/SARibbonBar.h>
#include <SARibbonBar/SARibbonButtonGroupWidget.h>
#include <SARibbonBar/SARibbonSystemButtonBar.h>

#include <QAbstractButton>
#include <QDesktopServices>
#include <QEvent>
#include <QMenu>
#include <QMessageBox>
#include <QStatusBar>
#include <QTimer>
#include <QToolButton>
#include <QWidget>

#include <QCefContext.h>

using qianjizn::cloudserver::UserSession;

namespace
{
	QVariantMap FirstUploadedFile(const AuthHttpClient::Response& response)
	{
		const QVariantList uploaded_files = response.dataValue.toList();
		if (!uploaded_files.isEmpty()) {
			return uploaded_files.constFirst().toMap();
		}

		return response.data;
	}
}
#endif

QJ_NAMESPACE_ULTRACAM_ULTRAMILL_BEGIN

CloudController::CloudController(NMainWindow* main_window)
#ifdef ENABLE_CLOUD_SERVER_MODULE
	: QObject(main_window)
	, _mainWindow(main_window)
	, _userAuth(qianjizn::cloudserver::CloudServerConfig{})
	, _cloudFileService(nullptr)
	, _desktopWebServer(nullptr)
	, _fileManagerView(nullptr)
	, _uploadTargetPickerDialog(nullptr)
	, _syncStatusButton(nullptr)
	, _shareButton(nullptr)
	, _userChip(nullptr)
	, _loginMenu(nullptr)
	, _personalCenterAction(nullptr)
	, _teamAction(nullptr)
	, _logoutAction(nullptr)
	, _lastUploadTarget()
{
	connect(_userAuth.Session(), &UserSession::AuthStateChanged, this, &CloudController::RefreshUserChipFromSession);
	connect(_userAuth.Session(), &UserSession::UserProfileChanged, this, &CloudController::RefreshUserChipFromSession);

	_cloudFileService = new qianjizn::cloudserver::CloudFileService(&_userAuth, this);
	_desktopWebServer = new qianjizn::cloudserver::DesktopWebServer(&_userAuth, this);
}
#else
	: QObject(main_window)
	, _mainWindow(main_window)
{
}
#endif

CloudController::~CloudController() = default;

void CloudController::Initialize()
{
#ifdef ENABLE_CLOUD_SERVER_MODULE
	InitUserChip();
	EnsureDesktopWebServerReady();
	RefreshUserChipFromSession();
	_userAuth.InitFromStoredToken();
	HideFileManagerView();
	SetSyncStatusVisual(SyncStatusVisual::NotUploaded);
#endif
}

void CloudController::HandleMainWindowEvent(QEvent* event)
{
#ifdef ENABLE_CLOUD_SERVER_MODULE
	if (event && event->type() == QEvent::WindowActivate) {
		_userAuth.OnWindowActivateEvent();
		if (_fileManagerView && _fileManagerView->isVisible()) {
			_fileManagerView->RefreshCurrentPage();
		}
	}

	if (event
		&& (event->type() == QEvent::Move || event->type() == QEvent::Resize
			|| event->type() == QEvent::LayoutRequest || event->type() == QEvent::Show)) {
		UpdateFileManagerOverlayGeometry();
	}
#else
	Q_UNUSED(event);
#endif
}

void CloudController::ToggleDocumentOverlay()
{
#ifdef ENABLE_CLOUD_SERVER_MODULE
	if (_fileManagerView && _fileManagerView->isVisible()) {
		HideFileManagerView();
		return;
	}

	OpenFileManager();
	emit DocumentOverlayVisibleChanged(_fileManagerView && _fileManagerView->isVisible());
#else
	emit DocumentOverlayVisibleChanged(false);
#endif
}

void CloudController::PrepareForLocalOpen()
{
#ifdef ENABLE_CLOUD_SERVER_MODULE
	HideFileManagerView();
	if (_mainWindow) {
		qianjizn::cloudserver::OpenRequestContext context;
		context.source = qianjizn::cloudserver::OpenRequestSource::Local;
		_mainWindow->SetPendingOpenRequest(context);
	}
	SetSyncStatusVisual(SyncStatusVisual::NotUploaded);
#endif
}

void CloudController::SaveCloudFileIfNeeded()
{
#ifdef ENABLE_CLOUD_SERVER_MODULE
	if (!_cloudFileService || !_mainWindow) {
		return;
	}

	const qianjizn::cloudserver::CloudFileState& document = _mainWindow->CurrentFileState();
	if (!document.IsCloud()) {
		if (_mainWindow && _mainWindow->statusBar()) {
			_mainWindow->statusBar()->showMessage(tr("Local file save completed."), 3000);
		}
		SetSyncStatusVisual(SyncStatusVisual::NotUploaded);
		return;
	}

	if (_cloudFileService->SaveInFlight()) {
		if (_mainWindow && _mainWindow->statusBar()) {
			_mainWindow->statusBar()->showMessage(tr("Cloud save is already running."), 2000);
		}
		return;
	}

	QString error_message;
	const bool started = _cloudFileService->SaveCloudFile(
		document.LocalFilePath(),
		document.cloudFileUuid,
		[this](const AuthHttpClient::Response& response) {
			if (!_mainWindow) {
				return;
			}

			if (!response.networkOk) {
				QMessageBox::warning(_mainWindow,
					tr("Warning"),
					response.bizMsg.isEmpty() ? tr("Cloud file upload failed.") : response.bizMsg);
				return;
			}

			if (response.httpStatus < 200 || response.httpStatus >= 300 || response.bizCode != 200) {
				QMessageBox::warning(_mainWindow,
					tr("Warning"),
					response.bizMsg.isEmpty() ? tr("Cloud file update was rejected.") : response.bizMsg);
				return;
			}

			if (_mainWindow->statusBar()) {
				_mainWindow->statusBar()->showMessage(tr("Cloud file saved."), 3000);
			}
			SetSyncStatusVisual(SyncStatusVisual::Synced);
		},
		&error_message);

	if (!started) {
		QMessageBox::warning(_mainWindow, tr("Warning"), error_message);
		return;
	}

	if (_mainWindow && _mainWindow->statusBar()) {
		_mainWindow->statusBar()->showMessage(tr("Saving cloud file..."), 2000);
	}
#endif
}

void CloudController::HideFileManagerView()
{
#ifdef ENABLE_CLOUD_SERVER_MODULE
	if (_fileManagerView) {
		_fileManagerView->hide();
	}
	emit DocumentOverlayVisibleChanged(false);
#else
	emit DocumentOverlayVisibleChanged(false);
#endif
}

void CloudController::NotifyRecentFilesChanged()
{
#ifdef ENABLE_CLOUD_SERVER_MODULE
	if (_fileManagerView) {
		_fileManagerView->NotifyRecentFilesChanged();
	}
#endif
}

void CloudController::ShowUploadTargetPicker()
{
#ifdef ENABLE_CLOUD_SERVER_MODULE
	if (!_mainWindow) {
		return;
	}

	if (!QCefContext::instance()) {
		QMessageBox::warning(_mainWindow, tr("Warning"), tr("Browser runtime is not ready yet. Please try again."));
		return;
	}

	if (!EnsureDesktopWebServerReady()) {
		return;
	}

	if (!_userAuth.Session() || !_userAuth.Session()->IsAuthenticated()) {
		_userAuth.ShowAccountAuthDialog(_mainWindow);
		if (!_userAuth.Session() || !_userAuth.Session()->IsAuthenticated()) {
			return;
		}
	}

	if (_uploadTargetPickerDialog) {
		_uploadTargetPickerDialog->deleteLater();
		_uploadTargetPickerDialog = nullptr;
	}

	const QUrl page_url = qianjizn::cloudserver::BuildDesktopUploadPickerUrl(_userAuth.FrontendBaseUrl());
	_uploadTargetPickerDialog = new qianjizn::cloudserver::UploadPickerDialog(_mainWindow, page_url, &_userAuth);
	connect(_uploadTargetPickerDialog,
		&qianjizn::cloudserver::UploadPickerDialog::UploadTargetSelected,
		this,
		&CloudController::HandleUploadTargetSelected);
	connect(_uploadTargetPickerDialog, &QObject::destroyed, this, [this]() { _uploadTargetPickerDialog = nullptr; });
	_uploadTargetPickerDialog->exec();
#endif
}

#ifdef ENABLE_CLOUD_SERVER_MODULE
bool CloudController::EnsureDesktopWebServerReady(bool show_warning)
{
	if (!_desktopWebServer) {
		if (show_warning && _mainWindow) {
			QMessageBox::warning(_mainWindow, tr("Warning"), tr("Local embedded web server is unavailable."));
		}
		return false;
	}

	if (!_desktopWebServer->Start()) {
		if (show_warning && _mainWindow) {
			QMessageBox::warning(_mainWindow, tr("Warning"), tr("Local embedded web server failed to start."));
		}
		return false;
	}

	_userAuth.SetFrontendBaseUrl(_desktopWebServer->BaseUrl());
	return true;
}

void CloudController::InitUserChip()
{
	if (_userChip || !_mainWindow) {
		return;
	}

	SARibbonSystemButtonBar* bar = _mainWindow->windowButtonBar();
	if (!bar) {
		return;
	}

	QWidget* spacer_widget = new QWidget(bar);
	spacer_widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	bar->addWidget(spacer_widget);

	InitSyncStatusButton();
	InitShareButton();
	_userChip = new qianjizn::cloudserver::TitleBarUserChip(bar, _userAuth.ApiBaseUrl());
	bar->addWidget(_userChip);

	QTimer::singleShot(0, this, [this]() { SyncTitleBarWidgets(); });

	_loginMenu = new QMenu(_mainWindow);
	_personalCenterAction = _loginMenu->addAction(tr("Personal center"));
	_teamAction = _loginMenu->addAction(tr("Team Management"));
	_logoutAction = _loginMenu->addAction(tr("Log out"));

	connect(_logoutAction, &QAction::triggered, this, &CloudController::Logout);
	connect(_personalCenterAction, &QAction::triggered, this, &CloudController::OpenPersonalProfile, Qt::UniqueConnection);
	connect(_teamAction, &QAction::triggered, this, &CloudController::OpenTeam, Qt::UniqueConnection);
	connect(_userChip,
		&qianjizn::cloudserver::TitleBarUserChip::loginRequested,
		this,
		&CloudController::ShowAccountAuthDialog);
	connect(
		_userChip, &qianjizn::cloudserver::TitleBarUserChip::accountMenuRequested, this, &CloudController::ShowAccountMenu);
}

void CloudController::InitSyncStatusButton()
{
	if (_syncStatusButton) {
		return;
	}

	if (!_mainWindow) {
		return;
	}

	SARibbonBar* ribbon_bar = _mainWindow->ribbonBar();
	if (!ribbon_bar) {
		return;
	}

	SARibbonButtonGroupWidget* right_group = ribbon_bar->activeRightButtonGroup();
	if (!right_group) {
		return;
	}

	_syncStatusButton = new QToolButton(right_group);
	_syncStatusButton->setObjectName(QStringLiteral("CamSyncStatusButton"));
	_syncStatusButton->setAutoRaise(true);
	_syncStatusButton->setFocusPolicy(Qt::NoFocus);
	_syncStatusButton->setCursor(Qt::PointingHandCursor);
	_syncStatusButton->setToolButtonStyle(Qt::ToolButtonTextOnly);
	_syncStatusButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
	_syncStatusButton->setStyleSheet(QStringLiteral(
		"QToolButton#CamSyncStatusButton {"
		"padding: 0 4px;"
		"min-width: 0px;"
		"background: transparent;"
		"border: none;"
		"border-radius: 4px;"
		"}"
		"QToolButton#CamSyncStatusButton:hover {"
		"background: #dbeafe;"
		"color: #1558b0;"
		"}"));
	connect(_syncStatusButton, &QToolButton::clicked, this, [this]() {
		if (!_mainWindow || !_syncStatusButton || !_syncStatusButton->property("allowUploadAction").toBool()) {
			return;
		}

		if (!_mainWindow->SaveFile(false)) {
			return;
		}

		ShowUploadTargetPicker();
	});
	SetSyncStatusVisual(SyncStatusVisual::NotUploaded);

	right_group->addWidget(_syncStatusButton);
}

void CloudController::InitShareButton()
{
	if (_shareButton) {
		return;
	}

	if (!_mainWindow) {
		return;
	}

	SARibbonBar* ribbon_bar = _mainWindow->ribbonBar();
	if (!ribbon_bar) {
		return;
	}

	SARibbonButtonGroupWidget* right_group = ribbon_bar->activeRightButtonGroup();
	if (!right_group) {
		return;
	}

	_shareButton = new QToolButton(right_group);
	_shareButton->setObjectName(QStringLiteral("CamShareButton"));
	_shareButton->setAutoRaise(true);
	_shareButton->setFocusPolicy(Qt::NoFocus);
	_shareButton->setCursor(Qt::PointingHandCursor);
	_shareButton->setToolButtonStyle(Qt::ToolButtonTextOnly);
	_shareButton->setText(QStringLiteral("分享"));
	_shareButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
	_shareButton->setStyleSheet(QStringLiteral(
		"QToolButton#CamShareButton {"
		"padding: 0 4px;"
		"min-width: 0px;"
		"background: #549bed;"
		"border: none;"
		"color:#FFFFFF;"
		"border-radius: 4px;"
		"}"
		"QToolButton#CamShareButton:hover {"
		"background: #dbeafe;"
		"color: #1558b0;"
		"}"));
	right_group->addWidget(_shareButton);
}

void CloudController::RefreshUserChipFromSession()
{
	if (!_userChip) {
		return;
	}

	_userChip->SyncFromSession(_userAuth.Session());
	SyncTitleBarWidgets();
	QTimer::singleShot(0, this, [this]() { SyncTitleBarWidgets(); });
}

void CloudController::SyncTitleBarWidgets()
{
	if (!_mainWindow) {
		return;
	}

	SARibbonSystemButtonBar* bar = _mainWindow->windowButtonBar();
	if (!bar) {
		return;
	}

	int row_h = bar->windowTitleHeight();
	if (QAbstractButton* min_button = bar->minimizeButton()) {
		const int min_button_height = min_button->height();
		if (min_button_height > 0) {
			row_h = min_button_height;
		}
	}

	const int min_height = qianjizn::cloudserver::TitleBarUserChip::s_avatarButtonSide;
	const int height = row_h > 0 ? qMax(row_h, min_height) : min_height;
	const int sync_height = _mainWindow->ribbonBar() ? qMax(24, _mainWindow->ribbonBar()->tabBarHeight() - 4) : height;
	if (_syncStatusButton) {
		_syncStatusButton->setFixedHeight(sync_height);
		_syncStatusButton->adjustSize();
	}
	if (_shareButton) {
		_shareButton->setFixedHeight(sync_height);
		_shareButton->adjustSize();
	}
	if (_userChip) {
		_userChip->setFixedHeight(height);
		_userChip->RelayoutInParent();
	}
}

void CloudController::SetSyncStatusVisual(SyncStatusVisual visual)
{
	if (!_syncStatusButton) {
		return;
	}

	QString text;
	QString tool_tip;
	if (visual == SyncStatusVisual::Synced) {
		text = QStringLiteral("已同步");
		tool_tip = QStringLiteral("当前文件内容已同步到云端");
	}
	else {
		text = QStringLiteral("未上传");
		tool_tip = QStringLiteral("当前文件仅保存在本地，尚未上传到云端");
	}

	_syncStatusButton->setProperty("allowUploadAction", visual == SyncStatusVisual::NotUploaded);
	_syncStatusButton->setText(text);
	_syncStatusButton->setToolTip(tool_tip);
	_syncStatusButton->adjustSize();
}

void CloudController::UpdateFileManagerOverlayGeometry()
{
	if (!_fileManagerView || !_mainWindow) {
		return;
	}

	constexpr int kOverlayResizeBorder = 2;

	int overlay_top = 0;
	if (SARibbonSystemButtonBar* bar = _mainWindow->windowButtonBar()) {
		overlay_top = bar->geometry().bottom() + 1;
		if (overlay_top <= 0) {
			overlay_top = bar->windowTitleHeight();
		}
	}

	overlay_top = qMax(0, overlay_top) + kOverlayResizeBorder;
	const int overlay_left = kOverlayResizeBorder;
	const int overlay_width = qMax(0, _mainWindow->width() - kOverlayResizeBorder * 2);
	const int overlay_height = qMax(0, _mainWindow->height() - overlay_top - kOverlayResizeBorder);
	const QPoint top_left = _mainWindow->mapToGlobal(QPoint(overlay_left, overlay_top));
	_fileManagerView->setGeometry(QRect(top_left, QSize(overlay_width, overlay_height)));
}

void CloudController::OpenFileManager()
{
	if (!EnsureDesktopWebServerReady()) {
		return;
	}

	if (!_mainWindow || !_mainWindow->centralWidget()) {
		return;
	}

	const QUrl page_url = qianjizn::cloudserver::BuildRecentFilesUrl(_userAuth.FrontendBaseUrl());
	ShowFileManagerWorkspace(page_url);
}

void CloudController::ShowFileManagerWorkspace(const QUrl& page_url)
{
	if (!_mainWindow || !_mainWindow->centralWidget()) {
		return;
	}

	if (!_fileManagerView) {
		_fileManagerView = new qianjizn::cloudserver::FileManagerView(_mainWindow, &_userAuth, _cloudFileService, page_url);
		connect(_fileManagerView, &qianjizn::cloudserver::FileManagerView::OpenFileRequested, this, [this](const QString& file_path) {
			HideFileManagerView();
			if (!_mainWindow) {
				return;
			}

			qianjizn::cloudserver::OpenRequestContext context;
			context.filePath = file_path;
			context.source = qianjizn::cloudserver::OpenRequestSource::Local;
			context.fromRecent = true;
			_mainWindow->SetPendingOpenRequest(context);
			const bool opened = _mainWindow->OpenFile(file_path, QString(), false);
			if (!opened) {
				return;
			}
			SetSyncStatusVisual(SyncStatusVisual::NotUploaded);
			});
		connect(_fileManagerView,
			&qianjizn::cloudserver::FileManagerView::OpenCloudFileRequested,
			this,
			[this](const QString& file_path, const QString& file_uuid) {
				HideFileManagerView();
				OpenCloudFileInWorkspace(file_path, file_uuid);
			});
		connect(_fileManagerView, &qianjizn::cloudserver::FileManagerView::OpenRequested, this, &CloudController::OpenRequested);
		connect(_fileManagerView, &qianjizn::cloudserver::FileManagerView::NewProjectRequested, this, &CloudController::NewProjectRequested);
		connect(_fileManagerView, &qianjizn::cloudserver::FileManagerView::ReturnToWorkspaceRequested, this, &CloudController::HideFileManagerView);
		connect(_fileManagerView, &qianjizn::cloudserver::FileManagerView::HelpDocRequested, this, &CloudController::HelpDocRequested);
		connect(_fileManagerView, &qianjizn::cloudserver::FileManagerView::LicenseRequested, this, &CloudController::LicenseRequested);
		connect(_fileManagerView, &qianjizn::cloudserver::FileManagerView::AboutRequested, this, &CloudController::AboutRequested);
	}
	else {
		_fileManagerView->RefreshCurrentPage();
	}

	UpdateFileManagerOverlayGeometry();
	_fileManagerView->SyncViewportGeometryNow();
	_fileManagerView->show();
	_fileManagerView->raise();
	_fileManagerView->SyncViewportGeometryNow();
	emit DocumentOverlayVisibleChanged(true);
}


void CloudController::OpenCloudFileInWorkspace(const QString& file_path, const QString& file_uuid)
{
	if (!_mainWindow) {
		return;
	}

	qianjizn::cloudserver::OpenRequestContext context;
	context.filePath = file_path;
	context.fileUuid = file_uuid;
	context.source = qianjizn::cloudserver::OpenRequestSource::Cloud;
	context.fromRecent = true;
	_mainWindow->SetPendingOpenRequest(context);
	const bool opened = _mainWindow->OpenFile(file_path, QString(), false);
	if (!opened) {
		return;
	}
	SetSyncStatusVisual(SyncStatusVisual::Synced);
}

void CloudController::OpenUploadedCloudFile(const QString& file_uuid, const QString& suggested_file_name)
{
	if (!_mainWindow || !_cloudFileService) {
		return;
	}

	QString error_message;
	const bool started = _cloudFileService->OpenCloudFile(
		file_uuid,
		suggested_file_name,
		[this](const QString& file_path, const QString& opened_file_uuid, const QString& open_error_message) {
			if (open_error_message.isEmpty()) {
				OpenCloudFileInWorkspace(file_path, opened_file_uuid);
				return;
			}

			if (_mainWindow) {
				QMessageBox::warning(_mainWindow,
					tr("Warning"),
					tr("File uploaded, but opening the cloud file failed: %1").arg(open_error_message));
			}
		},
		&error_message);

	if (!started) {
		QMessageBox::warning(_mainWindow,
			tr("Warning"),
			tr("File uploaded, but preparing the cloud file failed: %1").arg(error_message));
		return;
	}

	if (_mainWindow->statusBar()) {
		_mainWindow->statusBar()->showMessage(tr("Opening uploaded cloud file..."), 2000);
	}
}

void CloudController::ShowAccountAuthDialog()
{
	if (!_mainWindow) {
		return;
	}

	if (!QCefContext::instance()) {
		QMessageBox::warning(_mainWindow, tr("Warning"), tr("Browser runtime is not ready yet. Please try again."));
		return;
	}

	if (!EnsureDesktopWebServerReady()) {
		return;
	}

	_userAuth.ShowAccountAuthDialog(_mainWindow);
}

void CloudController::ShowAccountMenu()
{
	if (!_userChip || !_loginMenu) {
		return;
	}

	const QPoint popup_pos = _userChip->mapToGlobal(QPoint(0, _userChip->height()));
	_loginMenu->popup(popup_pos);
}

void CloudController::HandleUploadTargetSelected(const QVariantMap& payload)
{
	_lastUploadTarget = payload;
	if (!_mainWindow || !_cloudFileService) {
		return;
	}

	const QString folder_uuid = payload.value(QStringLiteral("folderUuid")).toString().trimmed();
	const QString folder_name = payload.value(QStringLiteral("folderName")).toString().trimmed();
	const QString scope = payload.value(QStringLiteral("scope")).toString().trimmed();
	const QString team_uuid = scope == QStringLiteral("team")
		? payload.value(QStringLiteral("teamUuid")).toString().trimmed()
		: QString();
	const QString scope_label = scope == QStringLiteral("team") ? tr("team") : tr("personal");
	const qianjizn::cloudserver::CloudFileState document = _mainWindow->CurrentFileState();

	if (folder_uuid.isEmpty()) {
		QMessageBox::warning(_mainWindow, tr("Warning"), tr("Upload target folder is missing."));
		return;
	}

	QString error_message;
	const bool started = _cloudFileService->UploadFileToTarget(
		document.LocalFilePath(),
		folder_uuid,
		team_uuid,
		[this, folder_name, scope_label](const AuthHttpClient::Response& response) {
			if (!_mainWindow) {
				return;
			}

			if (!response.networkOk) {
				QMessageBox::warning(_mainWindow,
					tr("Warning"),
					response.bizMsg.isEmpty() ? tr("Cloud file upload failed.") : response.bizMsg);
				return;
			}

			if (response.httpStatus < 200 || response.httpStatus >= 300 || response.bizCode != 200) {
				QMessageBox::warning(_mainWindow,
					tr("Warning"),
					response.bizMsg.isEmpty() ? tr("Cloud file upload was rejected.") : response.bizMsg);
				return;
			}

			if (_mainWindow->statusBar()) {
				_mainWindow->statusBar()->showMessage(
					tr("File uploaded to: %1 (%2)")
						.arg(folder_name.isEmpty() ? tr("Unnamed folder") : folder_name, scope_label),
					3000);
			}

			const QVariantMap uploaded_file = FirstUploadedFile(response);
			const QString uploaded_file_uuid = uploaded_file.value(QStringLiteral("fileUuid")).toString().trimmed();
			const QString uploaded_file_name = uploaded_file.value(QStringLiteral("fileName")).toString().trimmed();
			if (uploaded_file_uuid.isEmpty()) {
				QMessageBox::warning(_mainWindow,
					tr("Warning"),
					tr("File uploaded, but the returned cloud file identifier is missing."));
				return;
			}

			OpenUploadedCloudFile(uploaded_file_uuid, uploaded_file_name);
		},
		&error_message);

	if (!started) {
		QMessageBox::warning(_mainWindow, tr("Warning"), error_message);
		return;
	}

	if (_mainWindow->statusBar()) {
		_mainWindow->statusBar()->showMessage(
			tr("Uploading file to: %1 (%2)").arg(folder_name.isEmpty() ? tr("Unnamed folder") : folder_name, scope_label),
			2000);
	}
}

void CloudController::Logout()
{
	_userAuth.Logout();
}

void CloudController::OpenPersonalProfile()
{
	if (!_mainWindow) {
		return;
	}

	_userAuth.BuildExternalWebSsoUrl(QStringLiteral("/personal-profile"),
		[this](const QUrl& url, const QString& error_message) {
			if (!error_message.isEmpty() || !url.isValid()) {
				QMessageBox::warning(_mainWindow,
					tr("Warning"),
					error_message.isEmpty()
					? tr("Failed to open personal center.")
					: error_message);
				return;
			}
			QDesktopServices::openUrl(url);
		});
}

void CloudController::OpenTeam()
{
	if (!_mainWindow) {
		return;
	}

	_userAuth.BuildExternalWebSsoUrl(QStringLiteral("/team"), [this](const QUrl& url, const QString& error_message) {
		if (!error_message.isEmpty() || !url.isValid()) {
			QMessageBox::warning(
				_mainWindow, tr("Warning"), error_message.isEmpty() ? tr("Failed to open team page.") : error_message);
			return;
		}
		QDesktopServices::openUrl(url);
		});
}
#endif

QJ_NAMESPACE_ULTRACAM_ULTRAMILL_END
