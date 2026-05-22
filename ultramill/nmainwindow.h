#pragma once

#include "ultramill_global.h"
#include <SARibbonBar/SARibbonMainWindow.h>
#include <cloud_server/user_auth_service.h>

class QAction;
class QMenu;
class QEvent;
class QUrl;
class QWidget;
class FileManagerView;

QJ_NAMESPACE_FIT_CLOUD_SERVER_BEGIN
class DesktopWebServer;
class TitleBarUserChip;
QJ_NAMESPACE_FIT_CLOUD_SERVER_END

QJ_NAMESPACE_ULTRACAM_ULTRAMILL_BEGIN

class HomeWorkspace;
class ToolLibDialog;

class NMainWindow : public SARibbonMainWindow
{
    Q_OBJECT

public:
    explicit NMainWindow(QWidget* parent = nullptr);
    ~NMainWindow() override;

    bool OpenFile(const QString& file_name, const QString& backup_file = "", bool silent = false);
    qianjizn::cloudserver::UserAuthService& UserAuth() { return _userAuth; }
    const qianjizn::cloudserver::UserAuthService& UserAuth() const { return _userAuth; }

protected:
    bool event(QEvent* e) override;

protected slots:
    void RefreshUserChipFromSession();

private:
    void InitializeMainWindowShell();
    void ApplyWindowPresentation();
    bool EnsureDesktopWebServerReady(bool showWarning = true);
    void InitRibbonBar();
    void InitUserChip();
    void InitCentralWorkspace();
    void ShowHomeWorkspace();
    void ShowFileManagerWorkspace(const QUrl& pageUrl);
    void UpdateFileManagerOverlayGeometry();
    void SyncUserChipIntoTitleBar();
    void OnShowDocumentOverlay();
    void OnShowAccountAuthDialog();
    void OnShowAccountMenu();
    void OnLogout();
    void OnOpenPersonalProfile();
    void OnOpenFileManager();
    void OnOpenTeam();
    void OnShowToolLibDialog();
    void OnOpen();
    void OnNewProject();

    QAction* _actionDocument { nullptr };
    QAction* _actionNew { nullptr };
    QAction* _actionOpen { nullptr };
    qianjizn::cloudserver::UserAuthService _userAuth { qianjizn::cloudserver::CloudServerConfig {} };
    qianjizn::cloudserver::DesktopWebServer* _desktopWebServer { nullptr };
    HomeWorkspace* _homeWorkspace { nullptr };
    FileManagerView* _fileManagerView { nullptr };
    ToolLibDialog* _toolLibDialog { nullptr };
    qianjizn::cloudserver::TitleBarUserChip* _userChip { nullptr };
    QMenu* _loginMenu { nullptr };
    QAction* _personalCenterAction { nullptr };
    QAction* _teamAction { nullptr };
    QAction* _logoutAction { nullptr };
};

QJ_NAMESPACE_ULTRACAM_ULTRAMILL_END
