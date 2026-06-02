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

CloudController::CloudController(NMainWindow* mainWindow)
	: QObject(mainWindow)
	, _mainWindow(mainWindow)
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

	if (event && (event->type() == QEvent::Move || event->type() == QEvent::Resize || event->type() == QEvent::LayoutRequest
		|| event->type() == QEvent::Show)) {
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

	QString errorMessage;
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
		&errorMessage);

	if (!started) {
		QMessageBox::warning(_mainWindow, tr("Warning"), errorMessage);
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

bool CloudController::EnsureDesktopWebServerReady(bool showWarning)
{
	if (!_desktopWebServer) {
		if (showWarning && _mainWindow) {
			QMessageBox::warning(_mainWindow, tr("Warning"), tr("Local embedded web server is unavailable."));
		}
		return false;
	}

	if (!_desktopWebServer->Start()) {
		if (showWarning && _mainWindow) {
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

	QWidget* spacer = new QWidget(bar);
	spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	bar->addWidget(spacer);

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
	connect(_userChip, &qianjizn::cloudserver::TitleBarUserChip::loginRequested, this, &CloudController::ShowAccountAuthDialog);
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

	int rowH = bar->windowTitleHeight();
	if (QAbstractButton* minBtn = bar->minimizeButton()) {
		const int minButtonHeight = minBtn->height();
		if (minButtonHeight > 0) {
			rowH = minButtonHeight;
		}
	}

	const int minHeight = qianjizn::cloudserver::TitleBarUserChip::kAvatarButtonSide;
	const int height = rowH > 0 ? qMax(rowH, minHeight) : minHeight;
	_userChip->setFixedHeight(height);
	_userChip->RelayoutInParent();
}

void CloudController::UpdateFileManagerOverlayGeometry()
{
	if (!_fileManagerView || !_mainWindow) {
		return;
	}

	constexpr int kOverlayResizeBorder = 2;

	int overlayTop = 0;
	if (SARibbonSystemButtonBar* bar = _mainWindow->windowButtonBar()) {
		overlayTop = bar->geometry().bottom() + 1;
		if (overlayTop <= 0) {
			overlayTop = bar->windowTitleHeight();
		}
	}

	overlayTop = qMax(0, overlayTop) + kOverlayResizeBorder;
	const int overlayLeft = kOverlayResizeBorder;
	const int overlayWidth = qMax(0, _mainWindow->width() - kOverlayResizeBorder * 2);
	const int overlayHeight = qMax(0, _mainWindow->height() - overlayTop - kOverlayResizeBorder);
	const QPoint topLeft = _mainWindow->mapToGlobal(QPoint(overlayLeft, overlayTop));
	_fileManagerView->setGeometry(QRect(topLeft, QSize(overlayWidth, overlayHeight)));
}

void CloudController::OpenFileManager()
{
	if (!EnsureDesktopWebServerReady()) {
		return;
	}

	if (!_mainWindow || !_mainWindow->centralWidget()) {
		return;
	}

	const QUrl pageUrl = qianjizn::cloudserver::buildRecentFilesUrl(_userAuth.FrontendBaseUrl());
	ShowFileManagerWorkspace(pageUrl);
}

void CloudController::ShowFileManagerWorkspace(const QUrl& pageUrl)
{
	if (!_mainWindow || !_mainWindow->centralWidget()) {
		return;
	}

	if (!_fileManagerView) {
		_fileManagerView = new FileManagerView(_mainWindow, &_userAuth, _cloudFileService, pageUrl);
		connect(_fileManagerView, &FileManagerView::OpenFileRequested, this, [this](const QString& filePath) {
			HideFileManagerView();
			if (!_mainWindow) {
				return;
			}

			const bool opened = _mainWindow->OpenFile(filePath, QString(), false);
			if (!opened) {
				return;
			}

			if (_cloudFileService) {
				_cloudFileService->TrackOpenedLocalFile(filePath);
			}
			});
		connect(_fileManagerView,
			&FileManagerView::OpenCloudFileRequested,
			this,
			[this](const QString& filePath, const QString& fileUuid) {
				HideFileManagerView();
				if (!_mainWindow) {
					return;
				}

				const bool opened = _mainWindow->OpenFile(filePath, QString(), false);
				if (!opened) {
					return;
				}

				if (_cloudFileService) {
					_cloudFileService->TrackOpenedCloudFile(filePath, fileUuid);
				}
			});
		connect(_fileManagerView, &FileManagerView::OpenRequested, this, &CloudController::OpenRequested);
		connect(_fileManagerView, &FileManagerView::NewProjectRequested, this, &CloudController::NewProjectRequested);
		connect(_fileManagerView, &FileManagerView::ReturnToWorkspaceRequested, this, &CloudController::HideFileManagerView);
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

	const QPoint pos = _userChip->mapToGlobal(QPoint(0, _userChip->height()));
	_loginMenu->popup(pos);
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
		[this](const QUrl& url, const QString& errorMessage) {
			if (!errorMessage.isEmpty() || !url.isValid()) {
				QMessageBox::warning(_mainWindow,
					tr("Warning"),
					errorMessage.isEmpty()
					? tr("Failed to open personal center.")
					: errorMessage);
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

	_userAuth.BuildExternalWebSsoUrl(QStringLiteral("/team"), [this](const QUrl& url, const QString& errorMessage) {
		if (!errorMessage.isEmpty() || !url.isValid()) {
			QMessageBox::warning(
				_mainWindow, tr("Warning"), errorMessage.isEmpty() ? tr("Failed to open team page.") : errorMessage);
			return;
		}
		QDesktopServices::openUrl(url);
		});
}

QJ_NAMESPACE_ULTRACAM_ULTRAMILL_END
