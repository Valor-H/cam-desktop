#include "nmainwindow.h"
#include "cloud_controller.h"
#include "home_workspace.h"
#include "tool_lib_dialog.h"

#include <SARibbonBar/SARibbonBar.h>
#include <SARibbonBar/SARibbonCategory.h>
#include <SARibbonBar/SARibbonPanel.h>
#include <SARibbonBar/SARibbonQuickAccessBar.h>

#include <QAction>
#include <QCoreApplication>
#include <QEvent>
#include <QIcon>
#include <QMessageBox>
#include <QProcess>
#include <QStatusBar>
#include <QStyle>

QJ_NAMESPACE_ULTRACAM_ULTRAMILL_BEGIN

NMainWindow::NMainWindow(QWidget* parent)
    : SARibbonMainWindow(parent)
{
    ApplyWindowPresentation();

    _actionDocument = new QAction(QStringLiteral("Document"), this);
    _actionDocument->setCheckable(true);
    _actionNew = new QAction(QIcon(), tr("New"), this);
    _actionOpen = new QAction(QIcon(), tr("Open"), this);
    _actionSave = new QAction(QIcon(), tr("Save"), this);
    connect(_actionOpen, &QAction::triggered, this, &NMainWindow::OnOpen);
    connect(_actionSave, &QAction::triggered, this, &NMainWindow::OnSave);
    connect(_actionNew, &QAction::triggered, this, &NMainWindow::OnNewProject);

    InitCloudController();

    InitializeMainWindowShell();
}

NMainWindow::~NMainWindow() = default;

qianjizn::cloudserver::UserAuthService& NMainWindow::UserAuth()
{
    return _cloudController->UserAuth();
}

const qianjizn::cloudserver::UserAuthService& NMainWindow::UserAuth() const
{
    return _cloudController->UserAuth();
}

void NMainWindow::ApplyWindowPresentation()
{
    setWindowTitle(tr("QJCAM DEMO"));
    setWindowIcon(QIcon(QStringLiteral(":/qjcam/resource/logo.ico")));
    setMinimumSize(600, 400);
}

void NMainWindow::InitCloudController()
{
    if (_cloudController) {
        return;
    }

    _cloudController = new CloudController(this);
    connect(_actionDocument, &QAction::triggered, _cloudController, &CloudController::ToggleDocumentOverlay);
    connect(_cloudController, &CloudController::DocumentOverlayVisibleChanged, this, [this](bool visible) {
        _actionDocument->setChecked(visible);
    });
    connect(_cloudController, &CloudController::OpenRequested, this, &NMainWindow::OnOpen);
    connect(_cloudController, &CloudController::NewProjectRequested, this, &NMainWindow::OnNewProject);
}

bool NMainWindow::OpenFile(const QString& file_name, const QString& backup_file, bool silent)
{
    Q_UNUSED(file_name);
    Q_UNUSED(backup_file);
    Q_UNUSED(silent);
    return true;
}

bool NMainWindow::SaveFile(bool silent)
{
    if (!silent && statusBar()) {
        statusBar()->showMessage(tr("Save command requested."), 2000);
    }

    return true;
}

bool NMainWindow::event(QEvent* e)
{
    if (_cloudController) {
        _cloudController->HandleMainWindowEvent(e);
    }
    return SARibbonMainWindow::event(e);
}

void NMainWindow::InitializeMainWindowShell()
{
    InitRibbonBar();
    InitCentralWorkspace();
    if (_cloudController) {
        _cloudController->Initialize();
    }
}

void NMainWindow::InitCentralWorkspace()
{
    if (_homeWorkspace) {
        return;
    }

    _homeWorkspace = new HomeWorkspace(this);
    connect(_homeWorkspace, &HomeWorkspace::NewProjectRequested, this, &NMainWindow::OnNewProject);
    connect(_homeWorkspace, &HomeWorkspace::ToolLibRequested, this, &NMainWindow::OnShowToolLibDialog);

    setCentralWidget(_homeWorkspace);
}

void NMainWindow::InitRibbonBar()
{
    SARibbonBar* ribbonBarWidget = ribbonBar();
    if (!ribbonBarWidget) {
        return;
    }

    ribbonBarWidget->setRibbonStyle(SARibbonBar::RibbonStyleLooseThreeRow);
    ribbonBarWidget->setApplicationButton(nullptr);

    if (SARibbonQuickAccessBar* quickAccessBar = ribbonBarWidget->quickAccessBar()) {
        quickAccessBar->addAction(_actionDocument);
        quickAccessBar->addAction(_actionNew);
        quickAccessBar->addAction(_actionOpen);
        quickAccessBar->addAction(_actionSave);
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
                  _actionSave,
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

void NMainWindow::OnShowToolLibDialog()
{
    if (!_toolLibDialog) {
        _toolLibDialog = new ToolLibDialog(this);
        _toolLibDialog->setModal(false);
    }

    _toolLibDialog->show();
    _toolLibDialog->raise();
    _toolLibDialog->activateWindow();
}

void NMainWindow::OnOpen()
{
    if (_cloudController) {
        _cloudController->PrepareForLocalOpen();
    }

    if (!OpenFile(QString(), QString(), false)) {
        return;
    }

    if (statusBar()) {
        statusBar()->showMessage(tr("Open command requested."), 2000);
    }
}

void NMainWindow::OnSave()
{
    if (!SaveFile(false)) {
        return;
    }

    if (_cloudController) {
        _cloudController->SaveCloudFileIfNeeded();
    }
}

void NMainWindow::OnNewProject()
{
    if (_cloudController) {
        _cloudController->HideFileManagerView();
    }
    const QString program = QCoreApplication::applicationFilePath();
    const bool started = QProcess::startDetached(program, QStringList {}, QCoreApplication::applicationDirPath());
    if (statusBar()) {
        statusBar()->showMessage(
            started ? tr("Started a new project window.") : tr("Failed to start a new project window."),
            2000);
    }
    if (!started) {
        QMessageBox::warning(this, tr("Warning"), tr("Failed to start a new project window."));
    }
}

QJ_NAMESPACE_ULTRACAM_ULTRAMILL_END
