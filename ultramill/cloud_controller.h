#pragma once

#include "ultramill_global.h"
#include <cloud_server/cloud_file_state.h>
#include <cloud_server/open_request_context.h>
#include <cloud_server/user_auth_service.h>

#include <QObject>
#include <QVariantMap>

class QAction;
class QEvent;
class QMenu;
class QUrl;
class QWidget;
class QToolButton;

QJ_NAMESPACE_BEGIN1(cloudserver)
class CloudFileService;
class UserAuthService;
class DesktopWebServer;
class FileManagerView;
class TitleBarUserChip;
class UploadPickerDialog;
QJ_NAMESPACE_END1

QJ_NAMESPACE_ULTRACAM_ULTRAMILL_BEGIN

class NMainWindow;

class CloudController final : public QObject
{
	Q_OBJECT

public:
	explicit CloudController(NMainWindow* main_window);
	~CloudController() override;

	qianjizn::cloudserver::UserAuthService& UserAuth() { return _userAuth; }
	const qianjizn::cloudserver::UserAuthService& UserAuth() const { return _userAuth; }
	const qianjizn::cloudserver::CloudFileState& CurrentFileState() const { return _currentFileState; }
	void SetPendingOpenRequest(const qianjizn::cloudserver::OpenRequestContext& context) { _pendingOpenRequest = context; }
	qianjizn::cloudserver::OpenRequestContext TakePendingOpenRequest();
	void ApplyOpenedFileState(const qianjizn::cloudserver::OpenRequestContext& open_context);
	void AssignLocalFile(const QString& file_path) { _currentFileState.AssignLocalFile(file_path); }

	void Initialize();
	void HandleMainWindowEvent(QEvent* event);
	void ToggleDocumentOverlay();
	void PrepareForLocalOpen();
	void SaveCloudFileIfNeeded();
	void HideFileManagerView();
	void NotifyRecentFilesChanged();
	void ShowUploadTargetPicker();

signals:
	void OpenRequested();
	void NewProjectRequested();
	void DocumentOverlayVisibleChanged(bool visible);
	void HelpDocRequested();
	void LicenseRequested();
	void AboutRequested();

private:
	enum class SyncStatusVisual {
		NotUploaded,
		Synced,
	};

	bool EnsureDesktopWebServerReady(bool show_warning = true);
	void InitUserChip();
	void InitSyncStatusButton();
	void RefreshUserChipFromSession();
	void SyncTitleBarWidgets();
	void SetSyncStatusVisual(SyncStatusVisual visual);
	void UpdateFileManagerOverlayGeometry();
	void OpenFileManager();
	void OpenCloudFileInWorkspace(const QString& file_path, const QString& file_uuid);
	void ShowFileManagerWorkspace(const QUrl& page_url);
	void ShowAccountAuthDialog();
	void ShowAccountMenu();
	void OpenUploadedCloudFile(const QString& file_uuid, const QString& suggested_file_name);
	void HandleUploadTargetSelected(const QVariantMap& payload);
	void Logout();
	void OpenPersonalProfile();
	void OpenTeam();
	void OpenTrialApplication();

	NMainWindow* _mainWindow;
	qianjizn::cloudserver::UserAuthService _userAuth;
	qianjizn::cloudserver::CloudFileService* _cloudFileService;
	qianjizn::cloudserver::DesktopWebServer* _desktopWebServer;
	qianjizn::cloudserver::FileManagerView* _fileManagerView;
	qianjizn::cloudserver::UploadPickerDialog* _uploadTargetPickerDialog;
	QToolButton* _syncStatusButton;
	QToolButton* _trialButton;
	qianjizn::cloudserver::TitleBarUserChip* _userChip;
	QMenu* _loginMenu;
	QAction* _personalCenterAction;
	QAction* _teamAction;
	QAction* _logoutAction;
	QVariantMap _lastUploadTarget;
	qianjizn::cloudserver::CloudFileState _currentFileState;
	qianjizn::cloudserver::OpenRequestContext _pendingOpenRequest;
};

QJ_NAMESPACE_ULTRACAM_ULTRAMILL_END
