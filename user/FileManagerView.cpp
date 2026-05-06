#include "FileManagerView.h"

#include "AuthHttpClient.h"
#include "LocalFilesBridge.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPalette>
#include <QPointer>
#include <QRegularExpression>
#include <QSizePolicy>
#include <QVariantMap>
#include <QVariantList>
#include <QVBoxLayout>
#include <QtMath>

#include <QtConcurrent/QtConcurrentRun>

#include <QCefSetting.h>
#include <QCefView.h>

namespace
{
constexpr int kQjpFileType = 11;
constexpr int kRecentFileDownloadTimeoutSec = 60;
constexpr double kEmbeddedPageScale = 0.90;

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
    , m_pageUrl(pageUrl)
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

    m_view = new QCefView(pageUrl.toString(), &setting, this);
    m_view->setAutoFillBackground(true);
    m_view->setPalette(palette());
    layout->addWidget(m_view);

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
            &QCefView::invokeMethod,
            this,
            [this](const QCefBrowserId&, const QCefFrameId&, const QString& method, const QVariantList& arguments) {
                OnInvokeMethod(method, arguments);
            });
    connect(m_snapshotWatcher, &QFutureWatcher<LocalFilesSnapshot::Result>::finished, this, [this]() {
        PushLocalFilesSnapshot(m_snapshotWatcher->result());
    });
    if (m_authService && m_authService->Session()) {
        connect(m_authService->Session(), &qianjizn::user::UserSession::AuthStateChanged, this, [this](bool) {
            SyncAuthStateToWeb();
        });
        connect(m_authService->Session(), &qianjizn::user::UserSession::UserProfileChanged, this, [this]() {
            SyncAuthStateToWeb();
        });
    }
}

FileManagerView::~FileManagerView() = default;

void FileManagerView::NavigateTo(const QUrl& pageUrl)
{
    m_pageUrl = pageUrl;
    if (!m_view) {
        return;
    }

    m_view->navigateToUrl(m_pageUrl.toString());
}

void FileManagerView::ApplyEmbeddedScale()
{
    if (!m_view) {
        return;
    }

    m_view->setZoomLevel(zoomFactorToLevel(kEmbeddedPageScale));
}

void FileManagerView::InjectDesktopBridgeScript()
{
    if (!m_view) {
        return;
    }

    m_view->executeJavascript(QCefView::MainFrameID, LocalFilesBridge::BridgeInjectScript(), m_pageUrl.toString());
}

void FileManagerView::SyncAuthStateToWeb()
{
    if (!m_authService || !m_authService->Session() || !m_authService->Session()->IsAuthenticated()) {
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
    if (token.isEmpty() || currentUser.isEmpty()) {
        return;
    }

    const QJsonObject payload {
        { QStringLiteral("token"), token },
        { QStringLiteral("user"), QJsonObject::fromVariantMap(currentUser) },
    };
    const QString json = QString::fromUtf8(QJsonDocument(payload).toJson(QJsonDocument::Compact));
    const QString script = QStringLiteral(
        "(function(){"
        "if(typeof window.__DESKTOP_WEB_onLoginSuccess__==='function'){"
        "window.__DESKTOP_WEB_onLoginSuccess__(%1);"
        "}"
        "})();")
                               .arg(json);
    m_view->executeJavascript(QCefView::MainFrameID, script, m_pageUrl.toString());
}

void FileManagerView::OnLoadEnd()
{
    ApplyEmbeddedScale();
    InjectDesktopBridgeScript();
    SyncAuthStateToWeb();
}

void FileManagerView::OnInvokeMethod(const QString& method, const QVariantList& arguments)
{
    if (method == LocalFilesBridge::MethodRequestLogin()) {
        if (!m_authService) {
            return;
        }

        m_authService->ShowAccountAuthDialog(this);
        if (m_authService->Session()->IsAuthenticated()) {
            PushCurrentAuthStateToWeb();
        }
        return;
    }

    if (method == LocalFilesBridge::MethodRequestLocalFiles()) {
        RequestLocalFilesSnapshot();
        return;
    }

    if (method == LocalFilesBridge::MethodRequestOpenRecentFile()) {
        const QVariantMap payload = arguments.isEmpty() ? QVariantMap {} : arguments.first().toMap();
        HandleOpenRecentFileRequest(payload);
    }
}

void FileManagerView::RequestLocalFilesSnapshot()
{
    if (!m_snapshotWatcher || m_snapshotWatcher->isRunning()) {
        return;
    }

    m_snapshotWatcher->setFuture(QtConcurrent::run([]() {
        return LocalFilesSnapshot::ScanRoot();
    }));
}

void FileManagerView::PushLocalFilesSnapshot(const LocalFilesSnapshot::Result& result)
{
    if (!m_view) {
        return;
    }

    QJsonObject payload {
        { QStringLiteral("rootPath"), result.rootPath },
    };

    QString script;
    if (result.ok) {
        payload.insert(QStringLiteral("files"), result.files);
        const QString json = QString::fromUtf8(QJsonDocument(payload).toJson(QJsonDocument::Compact));
        script = QStringLiteral(
                     "(function(){"
                     "if(window.__DESKTOP_QT__&&typeof window.__DESKTOP_QT__.onLocalFilesSnapshot==='function'){"
                     "window.__DESKTOP_QT__.onLocalFilesSnapshot(%1);"
                     "}"
                     "})();")
                     .arg(json);
    } else {
        payload.insert(QStringLiteral("code"), result.errorCode);
        payload.insert(QStringLiteral("message"), result.errorMessage);
        const QString json = QString::fromUtf8(QJsonDocument(payload).toJson(QJsonDocument::Compact));
        script = QStringLiteral(
                     "(function(){"
                     "if(window.__DESKTOP_QT__&&typeof window.__DESKTOP_QT__.onLocalFilesError==='function'){"
                     "window.__DESKTOP_QT__.onLocalFilesError(%1);"
                     "}"
                     "})();")
                     .arg(json);
    }

    m_view->executeJavascript(QCefView::MainFrameID, script, m_pageUrl.toString());
}

void FileManagerView::HandleOpenRecentFileRequest(const QVariantMap& payload)
{
    if (!IsDesktopRecentRequest(payload)) {
        NotifyOpenRecentFileError(QStringLiteral("Only recent desktop QJP files can be opened here."),
                                  QStringLiteral("recent_open_rejected"));
        return;
    }

    const QString source = payload.value(QStringLiteral("source")).toString().trimmed();
    if (source == QStringLiteral("local")) {
        if (OpenRecentLocalFile(payload)) {
            return;
        }
        NotifyOpenRecentFileError(QStringLiteral("The local QJP file is invalid or cannot be opened."),
                                  QStringLiteral("recent_local_invalid"));
        return;
    }

    if (source == QStringLiteral("cloud")) {
        OpenRecentCloudFile(payload);
        return;
    }

    NotifyOpenRecentFileError(QStringLiteral("Unknown recent file source."), QStringLiteral("recent_source_invalid"));
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
    NotifyOpenRecentFileSuccess(fileInfo.absoluteFilePath());
    return true;
}

void FileManagerView::OpenRecentCloudFile(const QVariantMap& payload)
{
    if (!m_authService) {
        NotifyOpenRecentFileError(QStringLiteral("Desktop authentication service is unavailable."),
                                  QStringLiteral("recent_open_no_auth_service"));
        return;
    }

    const QVariantMap currentUser = m_authService->Session()->CurrentUser();
    const QString userUuid = currentUser.value(QStringLiteral("uuid")).toString().trimmed();
    const QString token = m_authService->Session()->AuthToken().trimmed();
    const QString fileUuid = payload.value(QStringLiteral("fileUuid")).toString().trimmed();

    if (token.isEmpty() || userUuid.isEmpty()) {
        NotifyOpenRecentFileError(QStringLiteral("Current session is missing token or user UUID."),
                                  QStringLiteral("recent_open_no_auth"));
        return;
    }
    if (fileUuid.isEmpty()) {
        NotifyOpenRecentFileError(QStringLiteral("Cloud recent file request is missing fileUuid."),
                                  QStringLiteral("recent_open_no_uuid"));
        return;
    }

    const QString cacheFilePath = BuildCacheFilePath(payload);
    const QFileInfo cacheInfo(cacheFilePath);
    QDir cacheDir(cacheInfo.dir());
    if (!cacheDir.exists() && !cacheDir.mkpath(QStringLiteral("."))) {
        NotifyOpenRecentFileError(QStringLiteral("Failed to create cache directory."),
                                  QStringLiteral("recent_open_cache_dir_failed"));
        return;
    }

    AuthHttpClient* httpClient = EnsureFileHttpClient();
    if (!httpClient) {
        NotifyOpenRecentFileError(QStringLiteral("Desktop download client initialization failed."),
                                  QStringLiteral("recent_open_http_unavailable"));
        return;
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
                self->NotifyOpenRecentFileError(QStringLiteral("Cloud file download failed: %1").arg(response.errorMessage),
                                                QStringLiteral("recent_open_cloud_network"));
                return;
            }

            if (!response.writeOk) {
                QFile::remove(response.targetFilePath);
                self->NotifyOpenRecentFileError(QStringLiteral("Cloud file cache write failed: %1").arg(response.errorMessage),
                                                QStringLiteral("recent_open_cloud_cache"));
                return;
            }

            emit self->OpenFileRequested(response.targetFilePath);
            self->NotifyOpenRecentFileSuccess(response.targetFilePath);
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

void FileManagerView::NotifyOpenRecentFileError(const QString& message, const QString& code)
{
    if (!m_view) {
        return;
    }

    const QJsonObject payload {
        { QStringLiteral("code"), code },
        { QStringLiteral("message"), message },
    };
    const QString json = QString::fromUtf8(QJsonDocument(payload).toJson(QJsonDocument::Compact));
    const QString script = QStringLiteral(
        "(function(){"
        "if(window.__DESKTOP_QT__&&typeof window.__DESKTOP_QT__.onOpenRecentFileError==='function'){"
        "window.__DESKTOP_QT__.onOpenRecentFileError(%1);"
        "}"
        "})();")
                               .arg(json);
    m_view->executeJavascript(QCefView::MainFrameID, script, m_pageUrl.toString());
}

void FileManagerView::NotifyOpenRecentFileSuccess(const QString& openedPath)
{
    if (!m_view) {
        return;
    }

    const QJsonObject payload {
        { QStringLiteral("openedPath"), openedPath },
    };
    const QString json = QString::fromUtf8(QJsonDocument(payload).toJson(QJsonDocument::Compact));
    const QString script = QStringLiteral(
        "(function(){"
        "if(window.__DESKTOP_QT__&&typeof window.__DESKTOP_QT__.onOpenRecentFileSuccess==='function'){"
        "window.__DESKTOP_QT__.onOpenRecentFileSuccess(%1);"
        "}"
        "})();")
                               .arg(json);
    m_view->executeJavascript(QCefView::MainFrameID, script, m_pageUrl.toString());
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
