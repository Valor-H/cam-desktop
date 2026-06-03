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
	, _actionDocument(nullptr)
	, _actionNew(nullptr)
	, _actionOpen(nullptr)
	, _actionSave(nullptr)
	, _homeWorkspace(nullptr)
	, _toolLibDialog(nullptr)
	, _cloudController(nullptr)
{
	ApplyWindowPresentation();

	_actionDocument = new QAction(tr("Document"), this);
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

bool NMainWindow::event(QEvent* event)
{
	if (_cloudController) {
		_cloudController->HandleMainWindowEvent(event);
	}
	return SARibbonMainWindow::event(event);
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
	SARibbonBar* ribbon_bar_widget = ribbonBar();
	if (!ribbon_bar_widget) {
		return;
	}

	ribbon_bar_widget->setRibbonStyle(SARibbonBar::RibbonStyleLooseThreeRow);
	ribbon_bar_widget->setApplicationButton(nullptr);

	if (SARibbonQuickAccessBar* quick_access_bar = ribbon_bar_widget->quickAccessBar()) {
		quick_access_bar->addAction(_actionDocument);
		quick_access_bar->addAction(_actionNew);
		quick_access_bar->addAction(_actionOpen);
		quick_access_bar->addAction(_actionSave);
	}

	const auto makeAction = [this](const QString& text, QStyle::StandardPixmap icon_type) {
		QAction* action = new QAction(style()->standardIcon(icon_type), text, this);
		connect(action, &QAction::triggered, this, [this, text]() {
			if (statusBar()) {
				statusBar()->showMessage(tr("Triggered: %1").arg(text), 2000);
			}
			});
		return action;
		};

	const auto fillPanel = [](SARibbonPanel* panel, const QList<QAction*>& large_actions) {
		if (!panel) {
			return;
		}
		for (QAction* action : large_actions) {
			panel->addLargeAction(action);
		}
		};

	SARibbonCategory* category_file = ribbon_bar_widget->addCategoryPage(QStringLiteral("File"));
	fillPanel(category_file->addPanel(QStringLiteral("Project")),
		{
			_actionDocument,
			_actionNew,
			_actionOpen,
			_actionSave,
		});

	SARibbonCategory* category_home = ribbon_bar_widget->addCategoryPage(QStringLiteral("Home"));
	fillPanel(category_home->addPanel(QStringLiteral("Workspace")),
		{
			makeAction(QStringLiteral("Recent Files"), QStyle::SP_FileDialogDetailedView),
			makeAction(QStringLiteral("Local Files"), QStyle::SP_DirIcon),
			makeAction(QStringLiteral("Resource Library"), QStyle::SP_ComputerIcon),
			makeAction(QStringLiteral("View"), QStyle::SP_DesktopIcon),
		});
	fillPanel(category_home->addPanel(QStringLiteral("Common")),
		{
			makeAction(QStringLiteral("Options"), QStyle::SP_FileDialogContentsView),
			makeAction(QStringLiteral("Layout"), QStyle::SP_TitleBarNormalButton),
			makeAction(QStringLiteral("Refresh"), QStyle::SP_BrowserReload),
			makeAction(QStringLiteral("Help"), QStyle::SP_MessageBoxInformation),
		});

	ribbon_bar_widget->setCurrentIndex(ribbon_bar_widget->categoryIndex(category_home));
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
	const bool started = QProcess::startDetached(program, QStringList{}, QCoreApplication::applicationDirPath());
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
