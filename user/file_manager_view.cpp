#include "file_manager_view.h"

#include "auth_http_client.h"
#include "local_files_snapshot.h"
#include "user_auth_service.h"
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
#include <QRegularExpression>
#include <QResizeEvent>
#include <QShowEvent>
#include <QSizePolicy>
#include <QTimer>
#include <QUrlQuery>
#include <QVariantMap>
#include <QVariantList>
#include <QVBoxLayout>
#include <QWindow>
#include <QtMath>

#include <QtConcurrent/QtConcurrentRun>

#include <QCefSetting.h>
#include <QCefEvent.h>
#include <QCefView.h>

namespace
{
constexpr int kQjpFileType = 11;
constexpr int kRecentFileDownloadTimeoutSec = 60;
constexpr double kEmbeddedPageScale = 0.90;
constexpr int kDesktopQueryErrorBusy = 409;
constexpr int kDesktopQueryErrorBadRequest = 400;
constexpr int kDesktopQueryErrorInternal = 500;
const QString kMethodRequestLocalFiles = QStringLiteral("Desktop.RequestLocalFiles");
const QString kMethodRequestLogin = QStringLiteral("Desktop.RequestLogin");
const QString kMethodRequestOpenRecentFile = QStringLiteral("Desktop.RequestOpenRecentFile");
const QString kMethodRequestOpen = QStringLiteral("Desktop.RequestOpen");
const QString kMethodRequestNewProject = QStringLiteral("Desktop.RequestNewProject");
const QString kEventDesktopOnResume = QStringLiteral("Desktop.OnResume");

#ifndef CAMDEMO_RECENT_FILE_CACHE_DIR
#define CAMDEMO_RECENT_FILE_CACHE_DIR "D:/cachePath/"
#endif

QString apiBaseStringForClient(const QUrl& url)
{
    QString baseUrl = url.toString(QUrl::RemoveQuery | QUrl::RemoveFragment);
    while (baseUrl.endsWith(QLatin1Char('/'))) {
        baseUrl.chop(1);
    }
    return baseUrl;
}

QUrl ensureDesktopSource(const QUrl& input)
{
    QUrl url = input;
    QUrlQuery query(url);
    query.removeAllQueryItems(QStringLiteral("source"));
    query.addQueryItem(QStringLiteral("source"), QStringLiteral("desktop"));
    url.setQuery(query);
    return url;
}

QString sanitizeFileSegment(QString value)
{
    value = value.trimmed();
    if (value.isEmpty()) {
        return QStringLiteral("recent-file");
    }

    value.replace(QRegularExpression(QStringLiteral("[^A-Za-z0-9._-]")), QStringLiteral("_"));
    return value;
}

double zoomFactorToLevel(double factor)
{
    if (factor <= 0.0 || qFuzzyCompare(factor, 1.0)) {
        return 0.0;
    }

    return qLn(factor) / qLn(1.2);
}
}

FileManagerView::FileManagerView(QWidget* parent, qianjizn::user::UserAuthService* authService, const QUrl& pageUrl)
    : QWidget(parent)
    , m_pageUrl(ensureDesktopSource(pageUrl))
    , m_authService(authService)
{
    setObjectName(QStringLiteral("embeddedFileManagerOverlay"));
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setAutoFillBackground(true);
    setContentsMargins(0, 0, 0, 0);

    QPalette pal = palette();
    pal.setColor(QPalette::Window, Qt::white);
    setPalette(pal);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    QCefSetting setting;
    setting.setBackgroundColor(QColor(Qt::white));

    m_view = new QCefView(m_pageUrl.toString(), &setting, this);
    m_view->setAutoFillBackground(true);
    m_view->setPalette(palette());
    layout->addWidget(m_view);

    connect(m_view, &QCefView::nativeBrowserCreated, this, [this](QWindow* window) {
        m_nativeBrowserWindow = window;
        ScheduleNativeBrowserWindowSync();
    });

    m_snapshotWatcher = new QFutureWatcher<LocalFilesSnapshot::Result>(this);

    connect(m_view,
            &QCefView::loadEnd,
            this,
            [this](const QCefBrowserId&, const QCefFrameId&, bool isMainFrame, int) {
                if (isMainFrame) {
                    OnLoadEnd();
                }
            });
    connect(m_view,
            &QCefView::addressChanged,
            this,
            [this](const QCefFrameId&, const QString& url) { m_pageUrl = ensureDesktopSource(QUrl(url)); });
    connect(m_view,
            &QCefView::cefQueryRequest,
            this,
            [this](const QCefBrowserId&, const QCefFrameId&, const QCefQuery& query) { OnCefQueryRequest(query); });
    connect(m_snapshotWatcher, &QFutureWatcher<LocalFilesSnapshot::Result>::finished, this, [this]() {
        PushLocalFilesSnapshot(m_snapshotWatcher->result());
    });
    if (m_authService && m_authService->Session()) {
        connect(m_authService->Session(), &qianjizn::user::UserSession::AuthStateChanged, this, [this](bool) {
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
    if (!m_view) {
        return;
    }

    const QJsonObject payload {
        { QStringLiteral("reason"), QStringLiteral("resume") },
        { QStringLiteral("ts"), QString::number(QDateTime::currentMSecsSinceEpoch()) },
    };
    const QString json = QString::fromUtf8(QJsonDocument(payload).toJson(QJsonDocument::Compact));
    QCefEvent event(kEventDesktopOnResume);
    event.setArguments(QVariantList { json });
    m_view->triggerEvent(event, QCefView::MainFrameID);
}

void FileManagerView::SyncViewportGeometryNow()
{
    if (!m_view) {
        return;
    }

    if (QLayout* rootLayout = layout()) {
        rootLayout->setGeometry(rect());
        rootLayout->activate();
    }

    m_view->setGeometry(rect());
    m_view->resize(size());
    m_view->updateGeometry();
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
    ScheduleNativeBrowserWindowSync();
}

void FileManagerView::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    ScheduleNativeBrowserWindowSync();
}

void FileManagerView::SyncNativeBrowserWindowNow()
{
    if (!m_view || !m_nativeBrowserWindow) {
        return;
    }

    m_nativeBrowserWindow->setVisible(isVisible() && m_view->isVisible());
    m_nativeBrowserWindow->setPosition(0, 0);
    m_nativeBrowserWindow->resize(m_view->size());
}

void FileManagerView::ScheduleNativeBrowserWindowSync()
{
    if (m_nativeBrowserSyncPending) {
        return;
    }

    m_nativeBrowserSyncPending = true;
    QTimer::singleShot(0, this, [this]() {
        m_nativeBrowserSyncPending = false;
        SyncViewportGeometryNow();
    });
}

void FileManagerView::ApplyEmbeddedScale()
{
    if (!m_view) {
        return;
    }

    m_view->setZoomLevel(zoomFactorToLevel(kEmbeddedPageScale));
}

void FileManagerView::SyncAuthStateToWeb()
{
    if (!m_authService || !m_authService->Session()) {
        return;
    }
    PushCurrentAuthStateToWeb();
}

void FileManagerView::PushCurrentAuthStateToWeb()
{
    if (!m_view || !m_authService || !m_authService->Session()) {
        return;
    }

    const QString token = m_authService->Session()->AuthToken().trimmed();
    const QVariantMap currentUser = m_authService->Session()->CurrentUser();
    const bool loggedIn = m_authService->Session()->IsAuthenticated() && !token.isEmpty() && !currentUser.isEmpty();

    const QJsonObject payload {
        { QStringLiteral("loggedIn"), loggedIn },
        { QStringLiteral("token"), loggedIn ? token : QString() },
        { QStringLiteral("user"), loggedIn ? QJsonObject::fromVariantMap(currentUser) : QJsonObject {} },
    };
    const QString json = QString::fromUtf8(QJsonDocument(payload).toJson(QJsonDocument::Compact));

    QCefEvent event(QStringLiteral("Desktop.AuthSync"));
    event.setArguments(QVariantList { json });
    m_view->triggerEvent(event, QCefView::MainFrameID);
}

void FileManagerView::OnLoadEnd()
{
    ApplyEmbeddedScale();
    SyncAuthStateToWeb();
}

void FileManagerView::OnCefQueryRequest(const QCefQuery& query)
{
    if (!m_view) {
        return;
    }

    QString method;
    QVariantMap payload;
    const QByteArray rawRequest = query.request().toUtf8();
    QJsonParseError parseError {};
    const QJsonDocument document = QJsonDocument::fromJson(rawRequest, &parseError);
    if (parseError.error == QJsonParseError::NoError && document.isObject()) {
        const QJsonObject requestObject = document.object();
        method = requestObject.value(QStringLiteral("method")).toString().trimmed();
        payload = requestObject.value(QStringLiteral("payload")).toObject().toVariantMap();
    } else {
        method = QString::fromUtf8(rawRequest).trimmed();
    }

    if (method == kMethodRequestLocalFiles) {
        RequestLocalFilesSnapshot(&query);
        return;
    }

    if (method == kMethodRequestLogin) {
        HandleDesktopLoginRequest(&query);
        return;
    }

    if (method == kMethodRequestOpenRecentFile) {
        HandleOpenRecentFileRequest(payload, &query);
        return;
    }

    if (method == kMethodRequestOpen) {
        emit OpenRequested();
        QCefQuery successQuery = query;
        successQuery.reply(true, QStringLiteral("{}"));
        m_view->responseQCefQuery(successQuery);
        return;
    }

    if (method == kMethodRequestNewProject) {
        emit NewProjectRequested();
        QCefQuery successQuery = query;
        successQuery.reply(true, QStringLiteral("{}"));
        m_view->responseQCefQuery(successQuery);
        return;
    }

    QCefQuery unsupportedQuery = query;
    unsupportedQuery.reply(false, QStringLiteral("Unsupported desktop query method."), kDesktopQueryErrorBadRequest);
    m_view->responseQCefQuery(unsupportedQuery);
}

void FileManagerView::HandleDesktopLoginRequest(const QCefQuery* query)
{
    if (!m_authService) {
        if (query) {
            QCefQuery errorQuery = *query;
            errorQuery.reply(false,
                             QStringLiteral("Desktop authentication service is unavailable."),
                             kDesktopQueryErrorInternal);
            m_view->responseQCefQuery(errorQuery);
        }
        return;
    }

    m_authService->ShowAccountAuthDialog(this);
    if (m_authService->Session()->IsAuthenticated()) {
        PushCurrentAuthStateToWeb();
    }

    if (query && m_view) {
        QCefQuery successQuery = *query;
        successQuery.reply(true, QStringLiteral("{}"));
        m_view->responseQCefQuery(successQuery);
    }
}

void FileManagerView::RequestLocalFilesSnapshot(const QCefQuery* query)
{
    if (!m_snapshotWatcher) {
        return;
    }

    if (m_snapshotWatcher->isRunning()) {
        if (query && m_view) {
            QCefQuery busyQuery = *query;
            busyQuery.reply(false,
                            QStringLiteral("A desktop local files request is already running."),
                            kDesktopQueryErrorBusy);
            m_view->responseQCefQuery(busyQuery);
        }
        return;
    }

    if (query) {
        m_pendingLocalFilesQuery = *query;
        m_hasPendingLocalFilesQuery = true;
    }

    m_snapshotWatcher->setFuture(QtConcurrent::run([]() {
        return LocalFilesSnapshot::ScanRoot();
    }));
}

void FileManagerView::PushLocalFilesSnapshot(const LocalFilesSnapshot::Result& result)
{
    if (!m_view) {
        m_hasPendingLocalFilesQuery = false;
        return;
    }

    if (m_hasPendingLocalFilesQuery) {
        QCefQuery query = m_pendingLocalFilesQuery;
        m_hasPendingLocalFilesQuery = false;

        if (result.ok) {
            const QJsonObject payload {
                { QStringLiteral("rootPath"), result.rootPath },
                { QStringLiteral("files"), result.files },
            };
            const QString json = QString::fromUtf8(QJsonDocument(payload).toJson(QJsonDocument::Compact));
            query.reply(true, json);
        } else {
            query.reply(false,
                        result.errorMessage.isEmpty() ? QStringLiteral("Failed to read desktop local files.")
                                                      : result.errorMessage,
                        kDesktopQueryErrorInternal);
        }

        m_view->responseQCefQuery(query);
        return;
    }
}

void FileManagerView::HandleOpenRecentFileRequest(const QVariantMap& payload, const QCefQuery* query)
{
    if (!IsDesktopRecentRequest(payload)) {
        if (query) {
            ReplyOpenRecentFileError(*query,
                                     QStringLiteral("Only recent desktop QJP files can be opened here."),
                                     kDesktopQueryErrorBadRequest);
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
                                     kDesktopQueryErrorBadRequest);
        }
        return;
    }

    if (source == QStringLiteral("cloud")) {
        OpenRecentCloudFile(payload, query);
        return;
    }

    if (query) {
        ReplyOpenRecentFileError(*query, QStringLiteral("Unknown recent file source."), kDesktopQueryErrorBadRequest);
    }
}

bool FileManagerView::OpenRecentLocalFile(const QVariantMap& payload)
{
    const QString filePath = ResolveRecentLocalPath(payload);
    const QFileInfo fileInfo(filePath);
    if (!fileInfo.exists() || !fileInfo.isFile()
        || fileInfo.suffix().compare(QStringLiteral("qjp"), Qt::CaseInsensitive) != 0) {
        return false;
    }

    emit OpenFileRequested(fileInfo.absoluteFilePath());
    return true;
}

void FileManagerView::OpenRecentCloudFile(const QVariantMap& payload, const QCefQuery* query)
{
    if (!m_authService) {
        if (query) {
            ReplyOpenRecentFileError(*query,
                                     QStringLiteral("Desktop authentication service is unavailable."),
                                     kDesktopQueryErrorInternal);
        }
        return;
    }

    const QVariantMap currentUser = m_authService->Session()->CurrentUser();
    const QString userUuid = currentUser.value(QStringLiteral("uuid")).toString().trimmed();
    const QString token = m_authService->Session()->AuthToken().trimmed();
    const QString fileUuid = payload.value(QStringLiteral("fileUuid")).toString().trimmed();

    if (token.isEmpty() || userUuid.isEmpty()) {
        if (query) {
            ReplyOpenRecentFileError(*query,
                                     QStringLiteral("Current session is missing token or user UUID."),
                                     kDesktopQueryErrorBadRequest);
        }
        return;
    }
    if (fileUuid.isEmpty()) {
        if (query) {
            ReplyOpenRecentFileError(*query,
                                     QStringLiteral("Cloud recent file request is missing fileUuid."),
                                     kDesktopQueryErrorBadRequest);
        }
        return;
    }

    const QString cacheFilePath = BuildCacheFilePath(payload);
    const QFileInfo cacheInfo(cacheFilePath);
    QDir cacheDir(cacheInfo.dir());
    if (!cacheDir.exists() && !cacheDir.mkpath(QStringLiteral("."))) {
        if (query) {
            ReplyOpenRecentFileError(*query, QStringLiteral("Failed to create cache directory."), kDesktopQueryErrorInternal);
        }
        return;
    }

    AuthHttpClient* httpClient = EnsureFileHttpClient();
    if (!httpClient) {
        if (query) {
            ReplyOpenRecentFileError(*query,
                                     QStringLiteral("Desktop download client initialization failed."),
                                     kDesktopQueryErrorInternal);
        }
        return;
    }

    if (query) {
        if (m_hasPendingOpenRecentFileQuery) {
            ReplyOpenRecentFileError(*query,
                                     QStringLiteral("A desktop open recent file request is already running."),
                                     kDesktopQueryErrorBusy);
            return;
        }

        m_pendingOpenRecentFileQuery = *query;
        m_hasPendingOpenRecentFileQuery = true;
    }

    QJsonObject requestBody {
        { QStringLiteral("fileUuid"), fileUuid },
        { QStringLiteral("UserUuid"), userUuid },
    };

    QPointer<FileManagerView> self(this);
    httpClient->PostJsonToFile(
        QStringLiteral("/api/file/getFile"),
        token,
        QJsonDocument(requestBody).toJson(QJsonDocument::Compact),
        cacheFilePath,
        kRecentFileDownloadTimeoutSec,
        [self](const AuthHttpClient::DownloadResponse& response) {
            if (!self) {
                return;
            }

            if (!response.networkOk) {
                QFile::remove(response.targetFilePath);
                if (self->m_hasPendingOpenRecentFileQuery) {
                    const QCefQuery query = self->m_pendingOpenRecentFileQuery;
                    self->m_hasPendingOpenRecentFileQuery = false;
                    self->ReplyOpenRecentFileError(
                        query,
                        QStringLiteral("Cloud file download failed: %1").arg(response.errorMessage),
                        kDesktopQueryErrorInternal);
                }
                return;
            }

            if (!response.writeOk) {
                QFile::remove(response.targetFilePath);
                if (self->m_hasPendingOpenRecentFileQuery) {
                    const QCefQuery query = self->m_pendingOpenRecentFileQuery;
                    self->m_hasPendingOpenRecentFileQuery = false;
                    self->ReplyOpenRecentFileError(
                        query,
                        QStringLiteral("Cloud file cache write failed: %1").arg(response.errorMessage),
                        kDesktopQueryErrorInternal);
                }
                return;
            }

            emit self->OpenFileRequested(response.targetFilePath);
            if (self->m_hasPendingOpenRecentFileQuery) {
                const QCefQuery query = self->m_pendingOpenRecentFileQuery;
                self->m_hasPendingOpenRecentFileQuery = false;
                self->ReplyOpenRecentFileSuccess(query, response.targetFilePath);
            }
        });
}

QString FileManagerView::ResolveRecentLocalPath(const QVariantMap& payload) const
{
    const QString path = payload.value(QStringLiteral("path")).toString().trimmed();
    if (!path.isEmpty()) {
        return QDir::fromNativeSeparators(path);
    }

    const QString fileUuid = payload.value(QStringLiteral("fileUuid")).toString().trimmed();
    if (fileUuid.startsWith(QStringLiteral("local::"))) {
        return QDir::fromNativeSeparators(fileUuid.mid(QStringLiteral("local::").size()));
    }

    return QString();
}

QString FileManagerView::BuildCacheFilePath(const QVariantMap& payload) const
{
    const QString fileName = payload.value(QStringLiteral("fileName")).toString().trimmed();
    QString candidate = sanitizeFileSegment(fileName);
    if (!candidate.endsWith(QStringLiteral(".qjp"), Qt::CaseInsensitive)) {
        const QString fileUuid = sanitizeFileSegment(payload.value(QStringLiteral("fileUuid")).toString());
        candidate = fileUuid.isEmpty() ? candidate : fileUuid;
        if (!candidate.endsWith(QStringLiteral(".qjp"), Qt::CaseInsensitive)) {
            candidate += QStringLiteral(".qjp");
        }
    }

    return QDir::fromNativeSeparators(QStringLiteral(CAMDEMO_RECENT_FILE_CACHE_DIR))
        + QLatin1Char('/') + candidate;
}

bool FileManagerView::IsDesktopRecentRequest(const QVariantMap& payload) const
{
    const QString path = m_pageUrl.path().trimmed();
    const bool isRecentRoute = path == QStringLiteral("/local-files") || path == QStringLiteral("/recent-files");
    const bool fromRecent = payload.value(QStringLiteral("fromRecent")).toBool();
    const int fileType = payload.value(QStringLiteral("fileType")).toInt();
    const QString source = payload.value(QStringLiteral("source")).toString().trimmed();

    return isRecentRoute
        && fromRecent
        && fileType == kQjpFileType
        && (source == QStringLiteral("local") || source == QStringLiteral("cloud"));
}

void FileManagerView::ReplyOpenRecentFileError(const QCefQuery& query, const QString& message, int errorCode)
{
    if (!m_view) {
        return;
    }

    QCefQuery errorQuery = query;
    errorQuery.reply(false, message, errorCode);
    m_view->responseQCefQuery(errorQuery);
}

void FileManagerView::ReplyOpenRecentFileSuccess(const QCefQuery& query, const QString& openedPath)
{
    if (!m_view) {
        return;
    }

    const QJsonObject payload {
        { QStringLiteral("openedPath"), openedPath },
    };
    const QString json = QString::fromUtf8(QJsonDocument(payload).toJson(QJsonDocument::Compact));

    QCefQuery successQuery = query;
    successQuery.reply(true, json);
    m_view->responseQCefQuery(successQuery);
}

AuthHttpClient* FileManagerView::EnsureFileHttpClient()
{
    if (m_fileHttpClient) {
        return m_fileHttpClient;
    }

    if (!m_authService) {
        return nullptr;
    }

    m_fileHttpClient = new AuthHttpClient(apiBaseStringForClient(m_authService->ApiBaseUrl()), this);
    return m_fileHttpClient;
}
