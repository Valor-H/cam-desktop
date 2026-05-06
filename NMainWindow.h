#pragma once

#include "SARibbonMainWindow.h"
#include "UserAuthService.h"

class TitleBarUserChip;
class QAction;
class QMenu;
class QEvent;
class QUrl;
class DesktopFrontendServer;
class FileManagerView;
class MockMainWorkspace;
class QStackedLayout;
class QWidget;

class NMainWindow : public SARibbonMainWindow
{
    Q_OBJECT

public:
    explicit NMainWindow(QWidget* parent = nullptr);
    ~NMainWindow() override;

    bool OpenFile(const QString& file_name, const QString& backup_file = "", bool silent = false);
    qianjizn::user::UserAuthService& UserAuth() { return _userAuth; }
    const qianjizn::user::UserAuthService& UserAuth() const { return _userAuth; }

protected:
    bool event(QEvent* e) override;
    void InitializeMainWindowShell();

protected slots:
    void RefreshUserChipFromSession();

private:
    bool EnsureDesktopFrontendServerReady(bool showWarning = true);
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

    QAction* _actionNew { nullptr };
    QAction* _actionOpen { nullptr };
    QAction* _actionSave { nullptr };
    QAction* _actionDocument { nullptr };
    qianjizn::user::UserAuthService _userAuth { qianjizn::user::UserModuleConfig {} };
    DesktopFrontendServer* _desktopFrontendServer { nullptr };
    QWidget* _contentHost { nullptr };
    QStackedLayout* _contentStack { nullptr };
    MockMainWorkspace* _homeWorkspace { nullptr };
    FileManagerView* _fileManagerView { nullptr };
    TitleBarUserChip* _userChip { nullptr };
    QMenu* _loginMenu { nullptr };
    QAction* _personalCenterAction { nullptr };
    QAction* _teamAction { nullptr };
    QAction* _logoutAction { nullptr };
    int _cefAuthRetryCount { 0 };
};
