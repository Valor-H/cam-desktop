#include "file_manager_view.h"

#include "desktop_runtime_injection.h"
#include "local_files_snapshot.h"
#include "user_auth_service.h"
#include <project_management/cam_options.h>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QEvent>
#include <QLayout>
#include <QPalette>
#include <QPointer>
#include <QResizeEvent>
#include <QShowEvent>
#include <QSizePolicy>
#include <QTimer>
#include <QVariantMap>
#include <QVariantList>
#include <QVBoxLayout>
#include <QWindow>

#include <QtConcurrent/QtConcurrentRun>

#include <QCefSetting.h>
#include <QCefEvent.h>
#include <QCefView.h>

namespace
{
	constexpr int s_qjpFileType = 11;
	constexpr int s_desktopQueryErrorBusy = 409;
	constexpr int s_desktopQueryErrorBadRequest = 400;
	constexpr int s_desktopQueryErrorInternal = 500;
	const QString s_methodRequestLocalFiles = QStringLiteral("Desktop.RequestLocalFiles");
	const QString s_methodRequestLogin = QStringLiteral("Desktop.RequestLogin");
	const QString s_methodRequestOpenRecentFile = QStringLiteral("Desktop.RequestOpenRecentFile");
	const QString s_methodRequestOpenFile = QStringLiteral("Desktop.RequestOpenFile");
	const QString s_methodRequestOpen = QStringLiteral("Desktop.RequestOpen");
	const QString s_methodRequestNewProject = QStringLiteral("Desktop.RequestNewProject");
	const QString s_methodRequestReturnToWorkspace = QStringLiteral("Desktop.RequestReturnToWorkspace");
	const QString s_methodRequestHelpDoc = QStringLiteral("Desktop.RequestHelpDoc");
	const QString s_methodRequestLicense = QStringLiteral("Desktop.RequestLicense");
	const QString s_methodRequestAbout = QStringLiteral("Desktop.RequestAbout");
	const QString s_eventDesktopOnResume = QStringLiteral("Desktop.OnResume");
	const QString s_eventDesktopLocalRecentFilesChanged = QStringLiteral("Desktop.LocalRecentFilesChanged");

	QString ApiBaseStringForClient(const QUrl& url)
	{
		QString base_url = url.toString(QUrl::RemoveQuery | QUrl::RemoveFragment);
		while (base_url.endsWith(QLatin1Char('/'))) {
			base_url.chop(1);
		}
		return base_url;
	}
}

QJ_NAMESPACE_FIT_CLOUD_SERVER_BEGIN

FileManagerView::FileManagerView(QWidget* parent,
	UserAuthService* auth_service,
	CloudFileService* cloud_file_service,
	const QUrl& page_url)
	: QWidget(parent)
	, _pageUrl(page_url)
	, _view(nullptr)
	, _nativeBrowserWindow(nullptr)
	, _snapshotWatcher(nullptr)
	, _authService(auth_service)
	, _cloudFileService(cloud_file_service)
	, _pendingLocalFilesQuery()
	, _hasPendingLocalFilesQuery(false)
	, _pendingOpenRecentFileQuery()
	, _hasPendingOpenRecentFileQuery(false)
	, _nativeBrowserSyncPending(false)
{
	setWindowFlags(Qt::Tool | Qt::FramelessWindowHint);
	setObjectName(QStringLiteral("embeddedFileManagerOverlay"));
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	setAutoFillBackground(true);
	setContentsMargins(0, 0, 0, 0);

	QPalette pal = palette();
	pal.setColor(QPalette::Window, Qt::white);
	setPalette(pal);

	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);

	QCefSetting setting;
	setting.setBackgroundColor(QColor(Qt::white));

	_view = new QCefView(_pageUrl.toString(), &setting, this);
	_view->setAutoFillBackground(true);
	_view->setPalette(palette());
	layout->addWidget(_view);

	connect(_view, &QCefView::nativeBrowserCreated, this, [this](QWindow* window) {
		_nativeBrowserWindow = window;
		ScheduleNativeBrowserWindowSync();
		});

	_snapshotWatcher = new QFutureWatcher<LocalFilesSnapshot::Result>(this);

	connect(_view,
		&QCefView::loadStart,
		this,
		[this](const QCefBrowserId&, const QCefFrameId&, bool is_main_frame, int) {
			if (is_main_frame) {
				OnLoadStart();
			}
		});
	connect(_view,
		&QCefView::loadEnd,
		this,
		[this](const QCefBrowserId&, const QCefFrameId&, bool is_main_frame, int) {
			if (is_main_frame) {
				OnLoadEnd();
			}
		});
	connect(_view,
		&QCefView::addressChanged,
		this,
		[this](const QCefFrameId&, const QString& url) { _pageUrl = QUrl(url); });
	connect(_view,
		&QCefView::cefQueryRequest,
		this,
		[this](const QCefBrowserId&, const QCefFrameId&, const QCefQuery& query) { OnCefQueryRequest(query); });
	connect(_snapshotWatcher, &QFutureWatcher<LocalFilesSnapshot::Result>::finished, this, [this]() {
		PushLocalFilesSnapshot(_snapshotWatcher->result());
		});
	if (_authService && _authService->Session()) {
		connect(_authService->Session(), &qianjizn::cloudserver::UserSession::AuthStateChanged, this, [this](bool) {
			SyncAuthStateToWeb();
			if (isVisible()) {
				RefreshCurrentPage();
			}
			});
		connect(_authService->Session(), &qianjizn::cloudserver::UserSession::UserProfileChanged, this, [this]() {
			SyncAuthStateToWeb();
			if (isVisible()) {
				RefreshCurrentPage();
			}
			});
	}
}

FileManagerView::~FileManagerView() = default;

void FileManagerView::RefreshCurrentPage()
{
	if (!_view) {
		return;
	}

	const QJsonObject payload{
		{ QStringLiteral("reason"), QStringLiteral("resume") },
		{ QStringLiteral("ts"), QString::number(QDateTime::currentMSecsSinceEpoch()) },
	};
	const QString json = QString::fromUtf8(QJsonDocument(payload).toJson(QJsonDocument::Compact));
	QCefEvent event(s_eventDesktopOnResume);
	event.setArguments(QVariantList{ json });
	_view->triggerEvent(event, QCefView::MainFrameID);
}

void FileManagerView::InjectDesktopRuntime()
{
	InjectDesktopRuntimeIntoView(_view, _authService, _pageUrl.toString());
}

void FileManagerView::SyncViewportGeometryNow()
{
	if (!_view) {
		return;
	}

	if (QLayout* rootLayout = layout()) {
		rootLayout->setGeometry(rect());
		rootLayout->activate();
	}

	_view->setGeometry(rect());
	_view->resize(size());
	_view->updateGeometry();
	SyncNativeBrowserWindowNow();
}

bool FileManagerView::event(QEvent* event)
{
	if (event && (event->type() == QEvent::ShowToParent || event->type() == QEvent::LayoutRequest)) {
		ScheduleNativeBrowserWindowSync();
	}

	return QWidget::event(event);
}

void FileManagerView::showEvent(QShowEvent* event)
{
	QWidget::showEvent(event);
	raise();
	ScheduleNativeBrowserWindowSync();
}

void FileManagerView::resizeEvent(QResizeEvent* event)
{
	QWidget::resizeEvent(event);
	ScheduleNativeBrowserWindowSync();
}

void FileManagerView::SyncNativeBrowserWindowNow()
{
	if (!_view || !_nativeBrowserWindow) {
		return;
	}

	_nativeBrowserWindow->setVisible(isVisible() && _view->isVisible());
	_nativeBrowserWindow->setPosition(0, 0);
	_nativeBrowserWindow->resize(_view->size());
}

void FileManagerView::ScheduleNativeBrowserWindowSync()
{
	if (_nativeBrowserSyncPending) {
		return;
	}

	_nativeBrowserSyncPending = true;
	QTimer::singleShot(0, this, [this]() {
		_nativeBrowserSyncPending = false;
		SyncViewportGeometryNow();
		});
}

void FileManagerView::OnLoadStart()
{
	InjectDesktopRuntime();
}

void FileManagerView::SyncAuthStateToWeb()
{
	if (!_authService || !_authService->Session()) {
		return;
	}
	PushCurrentAuthStateToWeb();
}

void FileManagerView::PushCurrentAuthStateToWeb()
{
	if (!_view || !_authService || !_authService->Session()) {
		return;
	}

	InjectDesktopRuntime();

	const QString token = _authService->Session()->AuthToken().trimmed();
	const QVariantMap current_user = _authService->Session()->CurrentUser();
	const bool logged_in = _authService->Session()->IsAuthenticated() && !token.isEmpty() && !current_user.isEmpty();

	const QJsonObject payload{
		{ QStringLiteral("loggedIn"), logged_in },
		{ QStringLiteral("token"), logged_in ? token : QString() },
		{ QStringLiteral("user"), logged_in ? QJsonObject::fromVariantMap(current_user) : QJsonObject {} },
	};
	const QString json = QString::fromUtf8(QJsonDocument(payload).toJson(QJsonDocument::Compact));

	QCefEvent event(QStringLiteral("Desktop.AuthSync"));
	event.setArguments(QVariantList{ json });
	_view->triggerEvent(event, QCefView::MainFrameID);
}

void FileManagerView::OnLoadEnd()
{
	InjectDesktopRuntime();
	SyncAuthStateToWeb();
}

void FileManagerView::OnCefQueryRequest(const QCefQuery& query)
{
	if (!_view) {
		return;
	}

	QString method;
	QVariantMap payload;
	const QByteArray raw_request = query.request().toUtf8();
	QJsonParseError parse_error{};
	const QJsonDocument document = QJsonDocument::fromJson(raw_request, &parse_error);
	if (parse_error.error == QJsonParseError::NoError && document.isObject()) {
		const QJsonObject request_object = document.object();
		method = request_object.value(QStringLiteral("method")).toString().trimmed();
		payload = request_object.value(QStringLiteral("payload")).toObject().toVariantMap();
	}
	else {
		method = QString::fromUtf8(raw_request).trimmed();
	}

	if (method == s_methodRequestLocalFiles) {
		RequestLocalFilesSnapshot(&query);
		return;
	}

	if (method == s_methodRequestLogin) {
		HandleDesktopLoginRequest(&query);
		return;
	}

	if (method == s_methodRequestOpenRecentFile) {
		HandleOpenRecentFileRequest(payload, &query);
		return;
	}

	if (method == s_methodRequestOpenFile) {
		const int file_type = payload.value(QStringLiteral("fileType")).toInt();
		const QString source = payload.value(QStringLiteral("source")).toString().trimmed();
		if (file_type != s_qjpFileType || (source != QStringLiteral("local") && source != QStringLiteral("cloud"))) {
			QCefQuery invalidQuery = query;
			invalidQuery.reply(false, QStringLiteral("Only desktop QJP files can be opened."), s_desktopQueryErrorBadRequest);
			_view->responseQCefQuery(invalidQuery);
			return;
		}

		if (source == QStringLiteral("local")) {
			if (!OpenRecentLocalFile(payload)) {
				QCefQuery invalidLocalQuery = query;
				invalidLocalQuery.reply(
					false,
					QStringLiteral("The local QJP file is invalid or cannot be opened."),
					s_desktopQueryErrorBadRequest);
				_view->responseQCefQuery(invalidLocalQuery);
				return;
			}

			QCefQuery successQuery = query;
			successQuery.reply(
				true,
				QString::fromUtf8(QJsonDocument(QJsonObject{
													{ QStringLiteral("openedPath"), ResolveRecentLocalPath(payload) },
					}).toJson(QJsonDocument::Compact)));
			_view->responseQCefQuery(successQuery);
			return;
		}

		OpenRecentCloudFile(payload, &query);
		return;
	}

	if (method == s_methodRequestOpen) {
		emit OpenRequested();
		QCefQuery successQuery = query;
		successQuery.reply(true, QStringLiteral("{}"));
		_view->responseQCefQuery(successQuery);
		return;
	}

	if (method == s_methodRequestNewProject) {
		emit NewProjectRequested();
		QCefQuery successQuery = query;
		successQuery.reply(true, QStringLiteral("{}"));
		_view->responseQCefQuery(successQuery);
		return;
	}

	if (method == s_methodRequestReturnToWorkspace) {
		emit ReturnToWorkspaceRequested();
		QCefQuery successQuery = query;
		successQuery.reply(true, QStringLiteral("{}"));
		_view->responseQCefQuery(successQuery);
		return;
	}

	if (method == s_methodRequestHelpDoc) {
		emit HelpDocRequested();
		QCefQuery successQuery = query;
		successQuery.reply(true, QStringLiteral("{}"));
		_view->responseQCefQuery(successQuery);
		return;
	}

	if (method == s_methodRequestLicense) {
		emit LicenseRequested();
		QCefQuery successQuery = query;
		successQuery.reply(true, QStringLiteral("{}"));
		_view->responseQCefQuery(successQuery);
		return;
	}

	if (method == s_methodRequestAbout) {
		emit AboutRequested();
		QCefQuery successQuery = query;
		successQuery.reply(true, QStringLiteral("{}"));
		_view->responseQCefQuery(successQuery);
		return;
	}

	QCefQuery unsupportedQuery = query;
	unsupportedQuery.reply(false, QStringLiteral("Unsupported desktop query method."), s_desktopQueryErrorBadRequest);
	_view->responseQCefQuery(unsupportedQuery);
}

void FileManagerView::HandleDesktopLoginRequest(const QCefQuery* query)
{
	if (!_authService) {
		if (query) {
			QCefQuery errorQuery = *query;
			errorQuery.reply(false,
				QStringLiteral("Desktop authentication service is unavailable."),
				s_desktopQueryErrorInternal);
			_view->responseQCefQuery(errorQuery);
		}
		return;
	}

	_authService->ShowAccountAuthDialog(this);
	if (_authService->Session()->IsAuthenticated()) {
		PushCurrentAuthStateToWeb();
	}

	if (query && _view) {
		QCefQuery successQuery = *query;
		successQuery.reply(true, QStringLiteral("{}"));
		_view->responseQCefQuery(successQuery);
	}
}

void FileManagerView::RequestLocalFilesSnapshot(const QCefQuery* query)
{
	if (!_snapshotWatcher) {
		return;
	}

	if (_snapshotWatcher->isRunning()) {
		if (query && _view) {
			QCefQuery busyQuery = *query;
			busyQuery.reply(false,
				QStringLiteral("A desktop local files request is already running."),
				s_desktopQueryErrorBusy);
			_view->responseQCefQuery(busyQuery);
		}
		return;
	}

	if (query) {
		_pendingLocalFilesQuery = *query;
		_hasPendingLocalFilesQuery = true;
	}

	_snapshotWatcher->setFuture(QtConcurrent::run([]() {
		return LocalFilesSnapshot::ScanRoot();
		}));
}

void FileManagerView::PushLocalFilesSnapshot(const LocalFilesSnapshot::Result& result)
{
	if (!_view) {
		_hasPendingLocalFilesQuery = false;
		return;
	}

	if (_hasPendingLocalFilesQuery) {
		QCefQuery query = _pendingLocalFilesQuery;
		_hasPendingLocalFilesQuery = false;

		if (result.ok) {
			const QJsonObject payload{
				{ QStringLiteral("rootPath"), result.rootPath },
				{ QStringLiteral("files"), result.files },
			};
			const QString json = QString::fromUtf8(QJsonDocument(payload).toJson(QJsonDocument::Compact));
			query.reply(true, json);
		}
		else {
			query.reply(false,
				result.errorMessage.isEmpty() ? QStringLiteral("Failed to read desktop local files.")
				: result.errorMessage,
				s_desktopQueryErrorInternal);
		}

		_view->responseQCefQuery(query);
		return;
	}
}

void FileManagerView::HandleOpenRecentFileRequest(const QVariantMap& payload, const QCefQuery* query)
{
	if (!IsDesktopRecentRequest(payload)) {
		if (query) {
			ReplyOpenRecentFileError(*query,
				QStringLiteral("Only recent desktop QJP files can be opened here."),
				s_desktopQueryErrorBadRequest);
		}
		return;
	}

	const QString source = payload.value(QStringLiteral("source")).toString().trimmed();
	if (source == QStringLiteral("local")) {
		if (OpenRecentLocalFile(payload)) {
			if (query) {
				ReplyOpenRecentFileSuccess(*query, ResolveRecentLocalPath(payload));
			}
			return;
		}
		if (query) {
			ReplyOpenRecentFileError(*query,
				QStringLiteral("The local QJP file is invalid or cannot be opened."),
				s_desktopQueryErrorBadRequest);
		}
		return;
	}

	if (source == QStringLiteral("cloud")) {
		OpenRecentCloudFile(payload, query);
		return;
	}

	if (query) {
		ReplyOpenRecentFileError(*query, QStringLiteral("Unknown recent file source."), s_desktopQueryErrorBadRequest);
	}
}

bool FileManagerView::OpenRecentLocalFile(const QVariantMap& payload)
{
	const QString file_path = ResolveRecentLocalPath(payload);
	const QFileInfo file_info(file_path);
	if (!file_info.exists() || !file_info.isFile()
		|| file_info.suffix().compare(QStringLiteral("qjp"), Qt::CaseInsensitive) != 0) {
		return false;
	}

	emit OpenFileRequested(file_info.absoluteFilePath());
	return true;
}

void FileManagerView::OpenRecentCloudFile(const QVariantMap& payload, const QCefQuery* query)
{
	if (!_cloudFileService) {
		if (query) {
			ReplyOpenRecentFileError(*query,
				QStringLiteral("Desktop cloud file service is unavailable."),
				s_desktopQueryErrorInternal);
		}
		return;
	}

	const QString file_uuid = payload.value(QStringLiteral("fileUuid")).toString().trimmed();
	if (file_uuid.isEmpty()) {
		if (query) {
			ReplyOpenRecentFileError(*query,
				QStringLiteral("Cloud recent file request is missing fileUuid."),
				s_desktopQueryErrorBadRequest);
		}
		return;
	}

	if (query) {
		if (_hasPendingOpenRecentFileQuery) {
			ReplyOpenRecentFileError(*query,
				QStringLiteral("A desktop open recent file request is already running."),
				s_desktopQueryErrorBusy);
			return;
		}

		_pendingOpenRecentFileQuery = *query;
		_hasPendingOpenRecentFileQuery = true;
	}

	QPointer<FileManagerView> self(this);
	QString error_message;
	const bool started = _cloudFileService->OpenCloudFile(
		file_uuid,
		payload.value(QStringLiteral("fileName")).toString(),
		[self](const QString& local_file_path, const QString& opened_file_uuid, const QString& open_error_message) {
			if (!self) {
				return;
			}

			if (!open_error_message.isEmpty()) {
				if (self->_hasPendingOpenRecentFileQuery) {
					const QCefQuery query = self->_pendingOpenRecentFileQuery;
					self->_hasPendingOpenRecentFileQuery = false;
					self->ReplyOpenRecentFileError(
						query,
						open_error_message,
						s_desktopQueryErrorInternal);
				}
				return;
			}

			emit self->OpenCloudFileRequested(local_file_path, opened_file_uuid);
			if (self->_hasPendingOpenRecentFileQuery) {
				const QCefQuery query = self->_pendingOpenRecentFileQuery;
				self->_hasPendingOpenRecentFileQuery = false;
				self->ReplyOpenRecentFileSuccess(query, local_file_path);
			}
		},
		&error_message);

	if (!started && query) {
		_hasPendingOpenRecentFileQuery = false;
		ReplyOpenRecentFileError(*query, error_message, s_desktopQueryErrorBadRequest);
	}
}

QString FileManagerView::ResolveRecentLocalPath(const QVariantMap& payload) const
{
	const QString local_path = payload.value(QStringLiteral("localPath")).toString().trimmed();
	if (!local_path.isEmpty()) {
		return QDir::fromNativeSeparators(local_path);
	}

	const QString file_uuid = payload.value(QStringLiteral("fileUuid")).toString().trimmed();
	if (file_uuid.startsWith(QStringLiteral("local::"))) {
		return QDir::fromNativeSeparators(file_uuid.mid(QStringLiteral("local::").size()));
	}

	return QString();
}

bool FileManagerView::IsDesktopRecentRequest(const QVariantMap& payload) const
{
	const QString path = _pageUrl.path().trimmed();
	const bool is_recent_route = path == QStringLiteral("/local-files") || path == QStringLiteral("/recent-files");
	const bool from_recent = payload.value(QStringLiteral("fromRecent")).toBool();
	const int file_type = payload.value(QStringLiteral("fileType")).toInt();
	const QString source = payload.value(QStringLiteral("source")).toString().trimmed();

	return is_recent_route
		&& from_recent
		&& file_type == s_qjpFileType
		&& (source == QStringLiteral("local") || source == QStringLiteral("cloud"));
}

void FileManagerView::ReplyOpenRecentFileError(const QCefQuery& query, const QString& message, int error_code)
{
	if (!_view) {
		return;
	}

	QCefQuery errorQuery = query;
	errorQuery.reply(false, message, error_code);
	_view->responseQCefQuery(errorQuery);
}

void FileManagerView::ReplyOpenRecentFileSuccess(const QCefQuery& query, const QString& opened_path)
{
	if (!_view) {
		return;
	}

	const QJsonObject payload{
		{ QStringLiteral("openedPath"), opened_path },
	};
	const QString json = QString::fromUtf8(QJsonDocument(payload).toJson(QJsonDocument::Compact));

	QCefQuery successQuery = query;
	successQuery.reply(true, json);
	_view->responseQCefQuery(successQuery);
}

void FileManagerView::NotifyRecentFilesChanged()
{
	if (!_view) {
		return;
	}

	const QJsonObject payload{
		{ QStringLiteral("ts"), QString::number(QDateTime::currentMSecsSinceEpoch()) },
	};
	const QString json = QString::fromUtf8(QJsonDocument(payload).toJson(QJsonDocument::Compact));

	QCefEvent event(s_eventDesktopLocalRecentFilesChanged);
	event.setArguments(QVariantList{ json });
	_view->triggerEvent(event, QCefView::MainFrameID);
}

QJ_NAMESPACE_FIT_CLOUD_SERVER_END
