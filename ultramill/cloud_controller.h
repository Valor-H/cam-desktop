#pragma once

#include "ultramill_global.h"

#include <cloud_server/user_auth_service.h>

#include <QObject>

class QAction;
class QEvent;
class QMenu;
class QUrl;
class QWidget;

QJ_NAMESPACE_BEGIN1(cloudserver)
class CloudFileService;
class UserAuthService;
class DesktopWebServer;
class FileManagerView;
class TitleBarUserChip;
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

    void Initialize();
    void HandleMainWindowEvent(QEvent* event);
    void ToggleDocumentOverlay();
    void PrepareForLocalOpen();
    void SaveCloudFileIfNeeded();
    void HideFileManagerView();

signals:
    void OpenRequested();
    void NewProjectRequested();
    void DocumentOverlayVisibleChanged(bool visible);

private:
    bool EnsureDesktopWebServerReady(bool show_warning = true);
    void InitUserChip();
    void RefreshUserChipFromSession();
    void SyncUserChipIntoTitleBar();
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
    qianjizn::cloudserver::TitleBarUserChip* _userChip;
    QMenu* _loginMenu;
    QAction* _personalCenterAction;
    QAction* _teamAction;
    QAction* _logoutAction;
};

QJ_NAMESPACE_ULTRACAM_ULTRAMILL_END
