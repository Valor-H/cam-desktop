#include "nmainwindow.h"
#ifdef ENABLE_CLOUD_SERVER_MODULE
#include "cloud_controller.h"
#endif
#include "home_workspace.h"
#include "tool_lib_dialog.h"
#include <project_management/cam_options.h>

#include <SARibbonBar/SARibbonBar.h>
#include <SARibbonBar/SARibbonCategory.h>
#include <SARibbonBar/SARibbonPanel.h>
#include <SARibbonBar/SARibbonQuickAccessBar.h>

#include <QAction>
#include <QCoreApplication>
#include <QDir>
#include <QEvent>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QIcon>
#include <QMessageBox>
#include <QProcess>
#include <QStatusBar>
#include <QStyle>
#include <QTextCodec>
#include <QTextStream>
#include <QStringList>

QJ_NAMESPACE_ULTRACAM_ULTRAMILL_BEGIN

NMainWindow::NMainWindow(QWidget* parent)
	: SARibbonMainWindow(parent)
#ifdef ENABLE_CLOUD_SERVER_MODULE
	, _actionDocument(nullptr)
	, _actionUpload(nullptr)
#endif
	, _actionNew(nullptr)
	, _actionOpen(nullptr)
	, _actionSave(nullptr)
	, _actionSaveAs(nullptr)
	, _currentFileState()
	, _pendingOpenRequest()
	, _homeWorkspace(nullptr)
	, _toolLibDialog(nullptr)
#ifdef ENABLE_CLOUD_SERVER_MODULE
	, _cloudController(nullptr)
#endif
{
	ApplyWindowPresentation();

#ifdef ENABLE_CLOUD_SERVER_MODULE
	_actionDocument = new QAction(tr("Document"), this);
	_actionDocument->setCheckable(true);
#endif
	_actionNew = new QAction(QIcon(), tr("New"), this);
	_actionOpen = new QAction(QIcon(), tr("Open"), this);
	_actionSave = new QAction(QIcon(), tr("Save"), this);
	_actionSaveAs = new QAction(QIcon(), tr("Save As"), this);

#ifdef ENABLE_CLOUD_SERVER_MODULE
	_actionUpload = new QAction(QIcon(), tr("Upload"), this);
#endif
	connect(_actionOpen, &QAction::triggered, this, &NMainWindow::OnOpen);
	connect(_actionSave, &QAction::triggered, this, &NMainWindow::OnSave);
	connect(_actionSaveAs, &QAction::triggered, this, &NMainWindow::OnSaveAs);

#ifdef ENABLE_CLOUD_SERVER_MODULE
	connect(_actionUpload, &QAction::triggered, this, &NMainWindow::OnUpload);
#endif
	connect(_actionNew, &QAction::triggered, this, &NMainWindow::OnNewProject);

#ifdef ENABLE_CLOUD_SERVER_MODULE
	InitCloudController();
#endif

	InitializeMainWindowShell();
}

NMainWindow::~NMainWindow() = default;

#ifdef ENABLE_CLOUD_SERVER_MODULE
qianjizn::cloudserver::UserAuthService& NMainWindow::UserAuth()
{
	return _cloudController->UserAuth();
}

const qianjizn::cloudserver::UserAuthService& NMainWindow::UserAuth() const
{
	return _cloudController->UserAuth();
}
#endif

void NMainWindow::ApplyWindowPresentation()
{
	setWindowTitle(tr("QJCAM DEMO"));
	setWindowIcon(QIcon(QStringLiteral(":/qjcam/resource/logo.ico")));
	setMinimumSize(600, 400);
}

#ifdef ENABLE_CLOUD_SERVER_MODULE
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
	connect(_cloudController, &CloudController::HelpDocRequested, this, &NMainWindow::OnHelpDoc);
	connect(_cloudController, &CloudController::LicenseRequested, this, &NMainWindow::OnLicense);
	connect(_cloudController, &CloudController::AboutRequested, this, &NMainWindow::OnAbout);
}
#endif

bool NMainWindow::OpenFile(const QString& file_name, const QString& backup_file, bool silent)
{
	Q_UNUSED(backup_file);
	qianjizn::cloudserver::OpenRequestContext open_context = _pendingOpenRequest;
	_pendingOpenRequest = qianjizn::cloudserver::OpenRequestContext{};

	QString target_file = file_name.trimmed();
	if (target_file.isEmpty()) {
		target_file = QFileDialog::getOpenFileName(
			this,
			tr("Open QJP File"),
			_currentFileState.LocalFilePath(),
			tr("QJP Files (*.qjp)"));
		if (target_file.isEmpty()) {
			return false;
		}
	}
	open_context.filePath = QFileInfo(target_file).absoluteFilePath();

	const QFileInfo file_info(target_file);
	if (!file_info.exists() || !file_info.isFile()) {
		if (!silent) {
			QMessageBox::warning(this, tr("Warning"), tr("The selected QJP file does not exist."));
		}
		return false;
	}

	if (file_info.suffix().compare(QStringLiteral("qjp"), Qt::CaseInsensitive) != 0) {
		if (!silent) {
			QMessageBox::warning(this, tr("Warning"), tr("Only .qjp files can be opened."));
		}
		return false;
	}

	const bool opened = LoadTextFileIntoWorkspace(target_file, silent);
	if (!opened) {
		return false;
	}

	ApplyOpenedFileState(open_context);

	const bool recent_list_updated = open_context.ShouldAddToLocalRecent()
		&& AddRecentlyOpenedFile(open_context.filePath);
#ifdef ENABLE_CLOUD_SERVER_MODULE
	if (recent_list_updated && _cloudController) {
		_cloudController->NotifyRecentFilesChanged();
	}
#endif
	return true;
}

bool NMainWindow::SaveFile(bool silent)
{
	if (!_homeWorkspace) {
		return false;
	}

	const QString current_local_path = _currentFileState.LocalFilePath();
	if (current_local_path.isEmpty()) {
		return SaveAsFile(QString(), silent);
	}

	return SaveWorkspaceToPath(current_local_path, silent);
}

bool NMainWindow::SaveAsFile(const QString& file_path, bool silent)
{
	QString initial_path = file_path.trimmed();
	if (initial_path.isEmpty()) {
		initial_path = _currentFileState.LocalFilePath();
	}
	if (initial_path.isEmpty()) {
		initial_path = QDir::homePath() + QDir::separator() + QStringLiteral("Untitled.qjp");
	}

	QString target_file = QFileDialog::getSaveFileName(
		this,
		tr("Save QJP File"),
		initial_path,
		tr("QJP Files (*.qjp)"));
	if (target_file.trimmed().isEmpty()) {
		return false;
	}

	QFileInfo target_info(target_file);
	if (target_info.suffix().compare(QStringLiteral("qjp"), Qt::CaseInsensitive) != 0) {
		target_file += QStringLiteral(".qjp");
	}

	const QString absolute_file_path = QFileInfo(target_file).absoluteFilePath().trimmed();
	if (absolute_file_path.isEmpty()) {
		if (!silent) {
			QMessageBox::warning(this, tr("Warning"), tr("The selected save path is invalid."));
		}
		return false;
	}

	if (!SaveWorkspaceToPath(absolute_file_path, silent)) {
		return false;
	}

	_currentFileState.AssignLocalFile(absolute_file_path);
	if (_homeWorkspace) {
		_homeWorkspace->SetViewportFilePath(absolute_file_path);
	}

	const bool recent_list_updated = AddRecentlyOpenedFile(absolute_file_path);
#ifdef ENABLE_CLOUD_SERVER_MODULE
	if (recent_list_updated && _cloudController) {
		_cloudController->NotifyRecentFilesChanged();
	}
#endif
	return true;
}

bool NMainWindow::SaveWorkspaceToPath(const QString& file_path, bool silent)
{
	const QString absolute_file_path = QFileInfo(file_path).absoluteFilePath().trimmed();
	if (absolute_file_path.isEmpty()) {
		if (!silent) {
			QMessageBox::warning(this, tr("Warning"), tr("There is no file to save."));
		}
		return false;
	}

	QFile file(absolute_file_path);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
		if (!silent) {
			QMessageBox::warning(this, tr("Warning"), tr("Failed to save file: %1").arg(absolute_file_path));
		}
		return false;
	}

	QTextStream stream(&file);
	stream.setCodec("UTF-8");
	stream << _homeWorkspace->ViewportText();
	stream.flush();
	file.close();

	if (stream.status() != QTextStream::Ok) {
		if (!silent) {
			QMessageBox::warning(this, tr("Warning"), tr("Failed to write file: %1").arg(absolute_file_path));
		}
		return false;
	}

	if (!silent && statusBar()) {
		statusBar()->showMessage(tr("Saved file: %1").arg(QFileInfo(absolute_file_path).fileName()), 3000);
	}
	return true;
}

bool NMainWindow::AddRecentlyOpenedFile(const QString& file_path)
{
	if (!qianjizn::project::CAMOptsPtr) {
		return false;
	}

	const QString absolute_file_path = QDir::toNativeSeparators(QFileInfo(file_path).absoluteFilePath()).trimmed();
	if (absolute_file_path.isEmpty()) {
		return false;
	}

	QStringList files = QString::fromStdString(qianjizn::project::CAMOptsPtr->GetRecentFileList())
		.split(';', QString::SkipEmptyParts);
	for (QString& file : files) {
		file = QDir::toNativeSeparators(QFileInfo(file.trimmed()).absoluteFilePath()).trimmed();
	}
	files.removeAll(QString());
	files.removeAll(absolute_file_path);
	files.prepend(absolute_file_path);
	while (files.size() > 9) {
		files.removeLast();
	}

	qianjizn::project::CAMOptsPtr->SetRecentFileList(files.join(';').toStdString());
	return true;
}

bool NMainWindow::LoadTextFileIntoWorkspace(const QString& file_path, bool silent)
{
	if (!_homeWorkspace) {
		return false;
	}

	const QString absolute_file_path = QFileInfo(file_path).absoluteFilePath();
	QFile file(absolute_file_path);
	if (!file.open(QIODevice::ReadOnly)) {
		if (!silent) {
			QMessageBox::warning(this, tr("Warning"), tr("Failed to open file: %1").arg(absolute_file_path));
		}
		return false;
	}

	const QByteArray raw = file.readAll();
	QTextCodec* utf8_codec = QTextCodec::codecForName("UTF-8");
	if (!utf8_codec) {
		if (!silent) {
			QMessageBox::warning(this, tr("Warning"), tr("UTF-8 codec is unavailable."));
		}
		return false;
	}

	QTextCodec::ConverterState state;
	const QString text = utf8_codec->toUnicode(raw.constData(), raw.size(), &state);
	if (state.invalidChars > 0) {
		if (!silent) {
			QMessageBox::warning(this, tr("Warning"), tr("Only UTF-8 encoded files are supported."));
		}
		return false;
	}

	_homeWorkspace->SetViewportText(text);
	_homeWorkspace->SetViewportFilePath(absolute_file_path);

	if (!silent && statusBar()) {
		statusBar()->showMessage(tr("Opened file: %1").arg(QFileInfo(absolute_file_path).fileName()), 3000);
	}

	return true;
}

void NMainWindow::ApplyOpenedFileState(const qianjizn::cloudserver::OpenRequestContext& open_context)
{
	const QString requested_path = open_context.filePath.trimmed();
	const QString absolute_file_path = requested_path.isEmpty()
		? QString()
		: QFileInfo(requested_path).absoluteFilePath().trimmed();
	if (open_context.IsCloud()) {
		_currentFileState.AssignCloudFile(absolute_file_path, open_context.fileUuid);
		return;
	}

	if (absolute_file_path.isEmpty()) {
		_currentFileState.ClearToDraft();
		return;
	}

	_currentFileState.AssignLocalFile(absolute_file_path);
}

void NMainWindow::SetPendingOpenRequest(const qianjizn::cloudserver::OpenRequestContext& context)
{
	_pendingOpenRequest = context;
}

bool NMainWindow::event(QEvent* event)
{
#ifdef ENABLE_CLOUD_SERVER_MODULE
	if (_cloudController) {
		_cloudController->HandleMainWindowEvent(event);
	}
#endif
	return SARibbonMainWindow::event(event);
}

void NMainWindow::InitializeMainWindowShell()
{
	InitRibbonBar();
	InitCentralWorkspace();
#ifdef ENABLE_CLOUD_SERVER_MODULE
	if (_cloudController) {
		_cloudController->Initialize();
	}
#endif
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

	// ribbon最小化按钮
	ribbon_bar_widget->showMinimumModeButton(true);

	if (SARibbonQuickAccessBar* quick_access_bar = ribbon_bar_widget->quickAccessBar()) {
#ifdef ENABLE_CLOUD_SERVER_MODULE
		quick_access_bar->addAction(_actionDocument);
#endif
		quick_access_bar->addAction(_actionNew);
		quick_access_bar->addAction(_actionOpen);
		quick_access_bar->addAction(_actionSave);
		quick_access_bar->addAction(_actionSaveAs);
#ifdef ENABLE_CLOUD_SERVER_MODULE
		quick_access_bar->addAction(_actionUpload);
#endif
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
#ifdef ENABLE_CLOUD_SERVER_MODULE
	fillPanel(category_file->addPanel(QStringLiteral("Project")),
		{
			_actionDocument,
			_actionNew,
			_actionOpen,
			_actionSave,
			_actionSaveAs,
			_actionUpload,
		});
#else
	fillPanel(category_file->addPanel(QStringLiteral("Project")),
		{
			_actionNew,
			_actionOpen,
			_actionSave,
			_actionSaveAs,
		});
#endif

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
#ifdef ENABLE_CLOUD_SERVER_MODULE
	if (_cloudController) {
		_cloudController->PrepareForLocalOpen();
	}
#endif

	if (!OpenFile(QString(), QString(), false)) {
		return;
	}
}

void NMainWindow::OnSave()
{
	if (!SaveFile(false)) {
		return;
	}

#ifdef ENABLE_CLOUD_SERVER_MODULE
	if (_cloudController) {
		_cloudController->SaveCloudFileIfNeeded();
	}
#endif
}

void NMainWindow::OnSaveAs()
{
	if (!SaveAsFile(QString(), false)) {
		return;
	}

#ifdef ENABLE_CLOUD_SERVER_MODULE
	if (_cloudController) {
		_cloudController->SaveCloudFileIfNeeded();
	}
#endif
}

#ifdef ENABLE_CLOUD_SERVER_MODULE
void NMainWindow::OnUpload()
{
	if (!SaveFile(false)) {
		return;
	}

	if (_cloudController) {
		_cloudController->ShowUploadTargetPicker();
	}
}
#endif

void NMainWindow::OnNewProject()
{
#ifdef ENABLE_CLOUD_SERVER_MODULE
	if (_cloudController) {
		_cloudController->HideFileManagerView();
	}
#endif
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

void NMainWindow::OnHelpDoc()
{
	qDebug() << "HelpDoc";
	return;
}

void NMainWindow::OnAbout()
{
	qDebug() << "About";
	return;
}

void NMainWindow::OnLicense()
{
	qDebug() << "License";
	return;
}
QJ_NAMESPACE_ULTRACAM_ULTRAMILL_END
