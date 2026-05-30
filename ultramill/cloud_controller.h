#pragma once

#include "ultramill_global.h"

#include <cloud_server/cloud_file_service.h>
#include <cloud_server/user_auth_service.h>

#include <QObject>

class QAction;
class QEvent;
class QMenu;
class QUrl;
class QWidget;
class FileManagerView;

QJ_NAMESPACE_BEGIN1(cloudserver)
class DesktopWebServer;
class TitleBarUserChip;
QJ_NAMESPACE_END1

QJ_NAMESPACE_ULTRACAM_ULTRAMILL_BEGIN

class NMainWindow;

class CloudController final : public QObject
{
    Q_OBJECT

public:
    explicit CloudController(NMainWindow* mainWindow);
    ~CloudController() override;

    qianjizn::cloudserver::UserAuthService& UserAuth() { return _userAuth; }
    const qianjizn::cloudserver::UserAuthService& UserAuth() const { return _userAuth; }

    void Initialize();
    void HandleMainWindowEvent(QEvent* event);
    void ToggleDocumentOverlay();
    void PrepareForLocalOpen();
    void SaveCloudFileIfNeeded();
    void HideFileManagerView();

signals:
    void OpenRequested();
    void NewProjectRequested();

private:
    bool EnsureDesktopWebServerReady(bool showWarning = true);
    void InitUserChip();
    void RefreshUserChipFromSession();
    void SyncUserChipIntoTitleBar();
    void UpdateFileManagerOverlayGeometry();
    void OpenFileManager();
    void ShowFileManagerWorkspace(const QUrl& pageUrl);
    void ShowAccountAuthDialog();
    void ShowAccountMenu();
    void Logout();
    void OpenPersonalProfile();
    void OpenTeam();

    NMainWindow* _mainWindow { nullptr };
    qianjizn::cloudserver::UserAuthService _userAuth { qianjizn::cloudserver::CloudServerConfig {} };
    qianjizn::cloudserver::CloudFileService* _cloudFileService { nullptr };
    qianjizn::cloudserver::DesktopWebServer* _desktopWebServer { nullptr };
    FileManagerView* _fileManagerView { nullptr };
    qianjizn::cloudserver::TitleBarUserChip* _userChip { nullptr };
    QMenu* _loginMenu { nullptr };
    QAction* _personalCenterAction { nullptr };
    QAction* _teamAction { nullptr };
    QAction* _logoutAction { nullptr };
};

QJ_NAMESPACE_ULTRACAM_ULTRAMILL_END
