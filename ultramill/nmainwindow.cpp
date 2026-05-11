#include "nmainwindow.h"

#include "desktop_frontend_server.h"
#include "desktop_web.h"
#include "file_manager_view.h"
#include "mock_main_workspace.h"
#include "SARibbonBar.h"
#include "SARibbonCategory.h"
#include "SARibbonPanel.h"
#include "SARibbonQuickAccessBar.h"
#include "SARibbonSystemButtonBar.h"
#include "title_bar_user_chip.h"

#include <QAbstractButton>
#include <QAction>
#include <QDesktopServices>
#include <QEvent>
#include <QIcon>
#include <QMenu>
#include <QMessageBox>
#include <QSizePolicy>
#include <QStatusBar>
#include <QStyle>
#include <QTimer>
#include <QWidget>

#include <QCefContext.h>

using qianjizn::user::UserSession;

QJ_NAMESPACE_ULTRACAM_ULTRAMILL_BEGIN

NMainWindow::NMainWindow(QWidget* parent)
    : SARibbonMainWindow(parent)
{
    _actionNew = new QAction(QIcon(), tr("New"), this);
    _actionOpen = new QAction(QIcon(), tr("Open"), this);
    _actionDocument = new QAction(QStringLiteral("Document"), this);

    connect(_userAuth.Session(), &UserSession::AuthStateChanged, this, &NMainWindow::RefreshUserChipFromSession);
    connect(_userAuth.Session(), &UserSession::UserProfileChanged, this, &NMainWindow::RefreshUserChipFromSession);
    connect(_actionDocument, &QAction::triggered, this, &NMainWindow::OnShowDocumentOverlay);
    connect(_actionOpen, &QAction::triggered, this, &NMainWindow::OnOpen);
    connect(_actionNew, &QAction::triggered, this, &NMainWindow::OnNewProject);

    _desktopFrontendServer = new DesktopFrontendServer(&_userAuth, this);
    InitializeMainWindowShell();
}

NMainWindow::~NMainWindow() = default;

bool NMainWindow::EnsureDesktopFrontendServerReady(bool showWarning)
{
    if (!_desktopFrontendServer) {
        if (showWarning) {
            QMessageBox::warning(this, tr("Warning"), tr("Local embedded web server is unavailable."));
        }
        return false;
    }

    if (!_desktopFrontendServer->Start()) {
        if (showWarning) {
            QMessageBox::warning(this, tr("Warning"), tr("Local embedded web server failed to start."));
        }
        return false;
    }

    _userAuth.SetFrontendBaseUrl(_desktopFrontendServer->BaseUrl());
    return true;
}

bool NMainWindow::OpenFile(const QString& file_name, const QString& backup_file, bool silent)
{
    Q_UNUSED(file_name);
    Q_UNUSED(backup_file);
    Q_UNUSED(silent);
    return true;
}

bool NMainWindow::event(QEvent* e)
{
    if (e && e->type() == QEvent::WindowActivate) {
        _userAuth.OnWindowActivateEvent();
    }
    if (e && (e->type() == QEvent::Resize || e->type() == QEvent::LayoutRequest || e->type() == QEvent::Show)) {
        UpdateFileManagerOverlayGeometry();
    }
    return SARibbonMainWindow::event(e);
}

void NMainWindow::InitializeMainWindowShell()
{
    InitRibbonBar();
    InitUserChip();
    InitCentralWorkspace();
    EnsureDesktopFrontendServerReady();
    RefreshUserChipFromSession();
    _userAuth.InitFromStoredToken();
}

void NMainWindow::InitCentralWorkspace()
{
    if (_homeWorkspace) {
        return;
    }

    _homeWorkspace = new MockMainWorkspace(this);
    setCentralWidget(_homeWorkspace);
}

void NMainWindow::RefreshUserChipFromSession()
{
    if (!_userChip) {
        return;
    }

    _userChip->SyncFromSession(_userAuth.Session());
    SyncUserChipIntoTitleBar();
    QTimer::singleShot(0, this, [this]() { SyncUserChipIntoTitleBar(); });
}

void NMainWindow::InitRibbonBar()
{
    SARibbonBar* ribbonBarWidget = ribbonBar();
    if (!ribbonBarWidget) {
        return;
    }

    ribbonBarWidget->setRibbonStyle(SARibbonBar::RibbonStyleLooseThreeRow);

    if (SARibbonQuickAccessBar* quickAccessBar = ribbonBarWidget->quickAccessBar()) {
        quickAccessBar->addAction(_actionDocument);
        quickAccessBar->addAction(_actionNew);
        quickAccessBar->addAction(_actionOpen);
    }

    const auto makeAction = [this](const QString& text, QStyle::StandardPixmap iconType) {
        auto* action = new QAction(style()->standardIcon(iconType), text, this);
        connect(action, &QAction::triggered, this, [this, text]() {
            if (statusBar()) {
                statusBar()->showMessage(tr("Triggered: %1").arg(text), 2000);
            }
        });
        return action;
    };

    const auto fillPanel = [](SARibbonPanel* panel, const QList<QAction*>& largeActions) {
        if (!panel) {
            return;
        }
        for (QAction* action : largeActions) {
            panel->addLargeAction(action);
        }
    };

    SARibbonCategory* categoryFile = ribbonBarWidget->addCategoryPage(QStringLiteral("File"));
    fillPanel(categoryFile->addPanel(QStringLiteral("Project")),
              {
                  _actionDocument,
                  _actionNew,
                  _actionOpen,
              });

    SARibbonCategory* categoryHome = ribbonBarWidget->addCategoryPage(QStringLiteral("Home"));
    fillPanel(categoryHome->addPanel(QStringLiteral("Workspace")),
              {
                  makeAction(QStringLiteral("Recent Files"), QStyle::SP_FileDialogDetailedView),
                  makeAction(QStringLiteral("Local Files"), QStyle::SP_DirIcon),
                  makeAction(QStringLiteral("Resource Library"), QStyle::SP_ComputerIcon),
                  makeAction(QStringLiteral("View"), QStyle::SP_DesktopIcon),
              });
    fillPanel(categoryHome->addPanel(QStringLiteral("Common")),
              {
                  makeAction(QStringLiteral("Options"), QStyle::SP_FileDialogContentsView),
                  makeAction(QStringLiteral("Layout"), QStyle::SP_TitleBarNormalButton),
                  makeAction(QStringLiteral("Refresh"), QStyle::SP_BrowserReload),
                  makeAction(QStringLiteral("Help"), QStyle::SP_MessageBoxInformation),
              });

    ribbonBarWidget->setCurrentIndex(ribbonBarWidget->categoryIndex(categoryHome));
}

void NMainWindow::InitUserChip()
{
    if (_userChip) {
        return;
    }

    SARibbonSystemButtonBar* bar = windowButtonBar();
    if (!bar) {
        return;
    }

    QWidget* spacer = new QWidget(bar);
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    bar->addWidget(spacer);

    _userChip = new TitleBarUserChip(bar, _userAuth.ApiBaseUrl());
    bar->addWidget(_userChip);

    QTimer::singleShot(0, this, [this]() { SyncUserChipIntoTitleBar(); });

    _loginMenu = new QMenu(this);
    _personalCenterAction = _loginMenu->addAction(tr("Personal center"));
    _teamAction = _loginMenu->addAction(QStringLiteral("Team Management"));
    _logoutAction = _loginMenu->addAction(tr("Log out"));

    connect(_logoutAction, &QAction::triggered, this, &NMainWindow::OnLogout);
    connect(_personalCenterAction, &QAction::triggered, this, &NMainWindow::OnOpenPersonalProfile, Qt::UniqueConnection);
    connect(_teamAction, &QAction::triggered, this, &NMainWindow::OnOpenTeam, Qt::UniqueConnection);

    connect(_userChip, &TitleBarUserChip::loginRequested, this, &NMainWindow::OnShowAccountAuthDialog);
    connect(_userChip, &TitleBarUserChip::accountMenuRequested, this, &NMainWindow::OnShowAccountMenu);
}

void NMainWindow::SyncUserChipIntoTitleBar()
{
    if (!_userChip) {
        return;
    }

    SARibbonSystemButtonBar* bar = windowButtonBar();
    if (!bar) {
        return;
    }

    int rowH = bar->windowTitleHeight();
    if (QAbstractButton* minBtn = bar->minimizeButton()) {
        const int mh = minBtn->height();
        if (mh > 0) {
            rowH = mh;
        }
    }

    const int minH = TitleBarUserChip::kAvatarButtonSide;
    const int h = rowH > 0 ? qMax(rowH, minH) : minH;
    _userChip->setFixedHeight(h);
    _userChip->RelayoutInParent();
}

void NMainWindow::UpdateFileManagerOverlayGeometry()
{
    if (!_fileManagerView) {
        return;
    }

    int overlayTop = 0;
    if (SARibbonSystemButtonBar* bar = windowButtonBar()) {
        overlayTop = bar->geometry().bottom() + 1;
        if (overlayTop <= 0) {
            overlayTop = bar->windowTitleHeight();
        }
    }

    overlayTop = qMax(0, overlayTop);
    const int overlayHeight = qMax(0, height() - overlayTop);
    _fileManagerView->setGeometry(0, overlayTop, width(), overlayHeight);
}

void NMainWindow::OnShowAccountAuthDialog()
{
    constexpr int kMaxRetries = 40;
    constexpr int kRetryMs = 50;

    if (!QCefContext::instance()) {
        if (_cefAuthRetryCount >= kMaxRetries) {
            _cefAuthRetryCount = 0;
            QMessageBox::warning(this, tr("Warning"), tr("Browser runtime is not ready yet. Please try again."));
            return;
        }

        ++_cefAuthRetryCount;
        QTimer::singleShot(kRetryMs, this, &NMainWindow::OnShowAccountAuthDialog);
        return;
    }

    _cefAuthRetryCount = 0;
    if (!EnsureDesktopFrontendServerReady()) {
        return;
    }
    _userAuth.ShowAccountAuthDialog(this);
}

void NMainWindow::OnShowAccountMenu()
{
    if (!_userChip || !_loginMenu) {
        return;
    }

    const QPoint pos = _userChip->mapToGlobal(QPoint(0, _userChip->height()));
    _loginMenu->popup(pos);
}

void NMainWindow::OnLogout()
{
    _userAuth.Logout();
}

void NMainWindow::OnOpenPersonalProfile()
{
    _userAuth.BuildExternalWebSsoUrl(QStringLiteral("/personal-profile"),
        [this](const QUrl& url, const QString& errorMessage) {
            if (!errorMessage.isEmpty() || !url.isValid()) {
                QMessageBox::warning(
                    this,
                    tr("Warning"),
                    errorMessage.isEmpty() ? tr("Failed to open personal center.") : errorMessage);
                return;
            }
            QDesktopServices::openUrl(url);
        });
}

void NMainWindow::OnOpenFileManager()
{
    if (!EnsureDesktopFrontendServerReady()) {
        return;
    }

    if (!_homeWorkspace) {
        InitCentralWorkspace();
    }

    const QUrl pageUrl = qianjizn::user::buildRecentFilesUrl(_userAuth.FrontendBaseUrl());
    ShowFileManagerWorkspace(pageUrl);
}

void NMainWindow::ShowHomeWorkspace()
{
    if (_fileManagerView) {
        _fileManagerView->hide();
    }
}

void NMainWindow::ShowFileManagerWorkspace(const QUrl& pageUrl)
{
    if (!_homeWorkspace) {
        return;
    }

    if (!_fileManagerView) {
        _fileManagerView = new FileManagerView(this, &_userAuth, pageUrl);
        connect(_fileManagerView, &FileManagerView::OpenFileRequested, this, [this](const QString& filePath) {
            OpenFile(filePath, QString(), false);
        });
        connect(_fileManagerView, &FileManagerView::OpenRequested, this, &NMainWindow::OnOpen);
        connect(_fileManagerView, &FileManagerView::NewProjectRequested, this, &NMainWindow::NewProject);
        _fileManagerView->hide();
    } else {
        _fileManagerView->NavigateTo(pageUrl);
    }

    UpdateFileManagerOverlayGeometry();
    _fileManagerView->show();
    _fileManagerView->raise();
}

void NMainWindow::OnShowDocumentOverlay()
{
    if (_fileManagerView && _fileManagerView->isVisible()) {
        ShowHomeWorkspace();
        return;
    }

    OnOpenFileManager();
}

void NMainWindow::OnOpenTeam()
{
    _userAuth.BuildExternalWebSsoUrl(QStringLiteral("/team"),
        [this](const QUrl& url, const QString& errorMessage) {
            if (!errorMessage.isEmpty() || !url.isValid()) {
                QMessageBox::warning(
                    this,
                    tr("Warning"),
                    errorMessage.isEmpty() ? tr("Failed to open team page.") : errorMessage);
                return;
            }
            QDesktopServices::openUrl(url);
        });
}

void NMainWindow::OnOpen()
{
    OpenFile(QString(), QString(), false);
    if (statusBar()) {
        statusBar()->showMessage(tr("Open command requested."), 2000);
    }
}

void NMainWindow::OnNewProject()
{
    NewProject();
}

void NMainWindow::NewProject()
{
    if (statusBar()) {
        statusBar()->showMessage(tr("New project command requested."), 2000);
    }
}

QJ_NAMESPACE_ULTRACAM_ULTRAMILL_END
