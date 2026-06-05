#pragma once

#include "ultramill_global.h"
#ifdef ENABLE_CLOUD_SERVER_MODULE
#include <cloud_server/user_auth_service.h>
#endif

#include <QObject>

class QAction;
class QEvent;
class QMenu;
class QUrl;
class QWidget;
class QToolButton;

QJ_NAMESPACE_BEGIN1(cloudserver)
#ifdef ENABLE_CLOUD_SERVER_MODULE
class CloudFileService;
class UserAuthService;
class DesktopWebServer;
class FileManagerView;
class TitleBarUserChip;
#endif
QJ_NAMESPACE_END1

QJ_NAMESPACE_ULTRACAM_ULTRAMILL_BEGIN

class NMainWindow;

class CloudController final : public QObject
{
	Q_OBJECT

public:
	explicit CloudController(NMainWindow* main_window);
	~CloudController() override;

#ifdef ENABLE_CLOUD_SERVER_MODULE
	qianjizn::cloudserver::UserAuthService& UserAuth() { return _userAuth; }
	const qianjizn::cloudserver::UserAuthService& UserAuth() const { return _userAuth; }
#endif

	void Initialize();
	void HandleMainWindowEvent(QEvent* event);
	void ToggleDocumentOverlay();
	void PrepareForLocalOpen();
	void SaveCloudFileIfNeeded();
	void HideFileManagerView();
	void NotifyRecentFilesChanged();

signals:
	void OpenRequested();
	void NewProjectRequested();
	void DocumentOverlayVisibleChanged(bool visible);
	void HelpDocRequested();
	void LicenseRequested();
	void AboutRequested();

private:
#ifdef ENABLE_CLOUD_SERVER_MODULE
	enum class SyncStatusVisual {
		NotUploaded,
		Synced,
	};

	bool EnsureDesktopWebServerReady(bool show_warning = true);
	void InitUserChip();
	void InitSyncStatusButton();
	void InitShareButton();
	void RefreshUserChipFromSession();
	void SyncTitleBarWidgets();
	void SetSyncStatusVisual(SyncStatusVisual visual);
	void UpdateFileManagerOverlayGeometry();
	void OpenFileManager();
	void ShowFileManagerWorkspace(const QUrl& page_url);
	void ShowAccountAuthDialog();
	void ShowAccountMenu();
	void Logout();
	void OpenPersonalProfile();
	void OpenTeam();

	NMainWindow* _mainWindow;
	qianjizn::cloudserver::UserAuthService _userAuth;
	qianjizn::cloudserver::CloudFileService* _cloudFileService;
	qianjizn::cloudserver::DesktopWebServer* _desktopWebServer;
	qianjizn::cloudserver::FileManagerView* _fileManagerView;
	QToolButton* _syncStatusButton;
	QToolButton* _shareButton;
	qianjizn::cloudserver::TitleBarUserChip* _userChip;
	QMenu* _loginMenu;
	QAction* _personalCenterAction;
	QAction* _teamAction;
	QAction* _logoutAction;
#else
	NMainWindow* _mainWindow;
#endif
};

QJ_NAMESPACE_ULTRACAM_ULTRAMILL_END
