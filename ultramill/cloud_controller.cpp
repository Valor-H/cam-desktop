#include "cloud_controller.h"

#include "nmainwindow.h"

#include <cloud_server/desktop_web.h>
#include <cloud_server/desktop_web_server.h>
#include <cloud_server/file_manager_view.h>
#include <cloud_server/title_bar_user_chip.h>
#include <SARibbonBar/SARibbonSystemButtonBar.h>

#include <QAbstractButton>
#include <QDesktopServices>
#include <QEvent>
#include <QMenu>
#include <QMessageBox>
#include <QStatusBar>
#include <QTimer>
#include <QWidget>

#include <QCefContext.h>

using qianjizn::cloudserver::UserSession;

QJ_NAMESPACE_ULTRACAM_ULTRAMILL_BEGIN

CloudController::CloudController(NMainWindow* main_window)
	: QObject(main_window)
	, _mainWindow(main_window)
	, _userAuth(qianjizn::cloudserver::CloudServerConfig {})
	, _cloudFileService(nullptr)
	, _desktopWebServer(nullptr)
	, _fileManagerView(nullptr)
	, _userChip(nullptr)
	, _loginMenu(nullptr)
	, _personalCenterAction(nullptr)
	, _teamAction(nullptr)
	, _logoutAction(nullptr)
{
	connect(_userAuth.Session(), &UserSession::AuthStateChanged, this, &CloudController::RefreshUserChipFromSession);
	connect(_userAuth.Session(), &UserSession::UserProfileChanged, this, &CloudController::RefreshUserChipFromSession);

	_cloudFileService = new qianjizn::cloudserver::CloudFileService(&_userAuth, this);
	_desktopWebServer = new qianjizn::cloudserver::DesktopWebServer(&_userAuth, this);
}

CloudController::~CloudController() = default;

void CloudController::Initialize()
{
	InitUserChip();
	EnsureDesktopWebServerReady();
	RefreshUserChipFromSession();
	_userAuth.InitFromStoredToken();
	HideFileManagerView();
}

void CloudController::HandleMainWindowEvent(QEvent* event)
{
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
}

void CloudController::ToggleDocumentOverlay()
{
	if (_fileManagerView && _fileManagerView->isVisible()) {
		HideFileManagerView();
		return;
	}

	OpenFileManager();
	emit DocumentOverlayVisibleChanged(_fileManagerView && _fileManagerView->isVisible());
}

void CloudController::PrepareForLocalOpen()
{
	HideFileManagerView();
	if (_cloudFileService) {
		_cloudFileService->ClearCurrentFile();
	}
}

void CloudController::SaveCloudFileIfNeeded()
{
	if (!_cloudFileService || !_cloudFileService->IsCurrentFileCloud()) {
		if (_mainWindow && _mainWindow->statusBar()) {
			_mainWindow->statusBar()->showMessage(tr("Local file save completed."), 3000);
		}
		return;
	}

	if (_cloudFileService->SaveInFlight()) {
		if (_mainWindow && _mainWindow->statusBar()) {
			_mainWindow->statusBar()->showMessage(tr("Cloud save is already running."), 2000);
		}
		return;
	}

	QString error_message;
	const bool started = _cloudFileService->SaveCurrentCloudFile(
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
		},
		&error_message);

	if (!started) {
		QMessageBox::warning(_mainWindow, tr("Warning"), error_message);
		return;
	}

	if (_mainWindow && _mainWindow->statusBar()) {
		_mainWindow->statusBar()->showMessage(tr("Saving cloud file..."), 2000);
	}
}

void CloudController::HideFileManagerView()
{
	if (_fileManagerView) {
		_fileManagerView->hide();
	}

	emit DocumentOverlayVisibleChanged(false);
}

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

	_userChip = new qianjizn::cloudserver::TitleBarUserChip(bar, _userAuth.ApiBaseUrl());
	bar->addWidget(_userChip);

	QTimer::singleShot(0, this, [this]() { SyncUserChipIntoTitleBar(); });

	_loginMenu = new QMenu(_mainWindow);
	_personalCenterAction = _loginMenu->addAction(tr("Personal center"));
	_teamAction = _loginMenu->addAction(QStringLiteral("Team Management"));
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

void CloudController::RefreshUserChipFromSession()
{
	if (!_userChip) {
		return;
	}

	_userChip->SyncFromSession(_userAuth.Session());
	SyncUserChipIntoTitleBar();
	QTimer::singleShot(0, this, [this]() { SyncUserChipIntoTitleBar(); });
}

void CloudController::SyncUserChipIntoTitleBar()
{
	if (!_userChip || !_mainWindow) {
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
	_userChip->setFixedHeight(height);
	_userChip->RelayoutInParent();
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

			const bool opened = _mainWindow->OpenFile(file_path, QString(), false);
			if (!opened) {
				return;
			}

			if (_cloudFileService) {
				_cloudFileService->TrackOpenedLocalFile(file_path);
			}
			});
		connect(_fileManagerView,
			&qianjizn::cloudserver::FileManagerView::OpenCloudFileRequested,
			this,
			[this](const QString& file_path, const QString& file_uuid) {
				HideFileManagerView();
				if (!_mainWindow) {
					return;
				}

				const bool opened = _mainWindow->OpenFile(file_path, QString(), false);
				if (!opened) {
					return;
				}

				if (_cloudFileService) {
					_cloudFileService->TrackOpenedCloudFile(file_path, file_uuid);
				}
			});
		connect(_fileManagerView, &qianjizn::cloudserver::FileManagerView::OpenRequested, this, &CloudController::OpenRequested);
		connect(_fileManagerView, &qianjizn::cloudserver::FileManagerView::NewProjectRequested, this, &CloudController::NewProjectRequested);
		connect(_fileManagerView, &qianjizn::cloudserver::FileManagerView::ReturnToWorkspaceRequested, this, &CloudController::HideFileManagerView);
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

QJ_NAMESPACE_ULTRACAM_ULTRAMILL_END
