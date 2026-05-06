#include "DesktopFrontendServer.h"

#include "UserAuthService.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMetaObject>
#include <QTcpSocket>
#include <QThread>

using qianjizn::user::UserAuthService;

DesktopFrontendServer::DesktopFrontendServer(UserAuthService* authService, QObject* parent)
    : QObject(parent)
    , m_authService(authService)
{
    ConfigureRoutes();
}

DesktopFrontendServer::~DesktopFrontendServer()
{
    Stop();
}

bool DesktopFrontendServer::Start()
{
    if (m_started) {
        return true;
    }

    const QString indexPath = QDir(WebRootPath()).filePath(QStringLiteral("index.html"));
    if (!QFileInfo::exists(indexPath)) {
        return false;
    }

    m_server.registerHttpService(&m_service);
    m_server.setHost("127.0.0.1");
    m_server.setPort(m_port);
    m_server.setThreadNum(1);
    if (m_server.start() != 0) {
        return false;
    }

    m_started = true;
    return true;
}

void DesktopFrontendServer::Stop()
{
    if (!m_started) {
        return;
    }

    m_server.stop();
    m_started = false;
}

QUrl DesktopFrontendServer::BaseUrl() const
{
    return QUrl(QStringLiteral("http://127.0.0.1:%1/").arg(m_port));
}

void DesktopFrontendServer::ConfigureRoutes()
{
    m_service.AllowCORS();

    m_service.GET("/desktop-bootstrap.json", [this](HttpRequest*, HttpResponse* resp) {
        resp->headers["Content-Type"] = "application/json; charset=utf-8";
        resp->body = BuildBootstrapJson().toStdString();
        return 200;
    });

    m_service.Use([this](HttpRequest* req, HttpResponse* resp) {
        if (req->method != HTTP_GET && req->method != HTTP_HEAD) {
            return HTTP_STATUS_NEXT;
        }

        const QString path = NormalizeRequestPath(req);
        if (path == QStringLiteral("/desktop-bootstrap.json")) {
            return HTTP_STATUS_NEXT;
        }

        const QString filePath = ResolveStaticFilePath(path);
        if (!filePath.isEmpty()) {
            return resp->File(filePath.toStdString().c_str());
        }

        const QString indexPath = QDir(WebRootPath()).filePath(QStringLiteral("index.html"));
        return resp->File(indexPath.toStdString().c_str());
    });
}

QString DesktopFrontendServer::NormalizeRequestPath(HttpRequest* req) const
{
    if (!req) {
        return QStringLiteral("/");
    }

    const std::string rawPath = req->Path();
    if (rawPath.empty()) {
        return QStringLiteral("/");
    }

    return QString::fromStdString(rawPath);
}

QString DesktopFrontendServer::BuildBootstrapJson() const
{
    const BootstrapSnapshot snapshot = CaptureSnapshotSync();
    QJsonObject payload {
        { QStringLiteral("loggedIn"), snapshot.loggedIn },
        { QStringLiteral("offline"), snapshot.offline },
        { QStringLiteral("backendUrl"), snapshot.backendUrl },
        { QStringLiteral("websocketUrl"), snapshot.websocketUrl },
        { QStringLiteral("helpDocUrl"), snapshot.helpDocUrl },
        { QStringLiteral("WebsocketUrl"), snapshot.websocketUrl },
        { QStringLiteral("HelpDocUrl"), snapshot.helpDocUrl },
        { QStringLiteral("token"), snapshot.token },
    };
    if (!snapshot.user.isEmpty()) {
        payload.insert(QStringLiteral("user"), QJsonObject::fromVariantMap(snapshot.user));
    }
    return QString::fromUtf8(QJsonDocument(payload).toJson(QJsonDocument::Compact));
}

DesktopFrontendServer::BootstrapSnapshot DesktopFrontendServer::CaptureSnapshot() const
{
    BootstrapSnapshot snapshot;
    if (!m_authService) {
        return snapshot;
    }

    snapshot.backendUrl = m_authService->ApiBaseUrl().toString().trimmed();
    snapshot.websocketUrl = m_authService->Config().websocketUrl.toString().trimmed();
    snapshot.helpDocUrl = m_authService->Config().helpDocUrl.toString().trimmed();
    snapshot.token = m_authService->Session()->AuthToken().trimmed();
    snapshot.user = m_authService->Session()->CurrentUser();
    snapshot.loggedIn = m_authService->Session()->IsAuthenticated() && !snapshot.token.isEmpty();
    snapshot.offline = !IsApiReachable();
    return snapshot;
}

DesktopFrontendServer::BootstrapSnapshot DesktopFrontendServer::CaptureSnapshotSync() const
{
    if (thread() == QThread::currentThread()) {
        return CaptureSnapshot();
    }

    BootstrapSnapshot snapshot;
    QMetaObject::invokeMethod(const_cast<DesktopFrontendServer*>(this), [&]() { snapshot = CaptureSnapshot(); },
                              Qt::BlockingQueuedConnection);
    return snapshot;
}

bool DesktopFrontendServer::IsApiReachable() const
{
    if (!m_authService) {
        return false;
    }

    const QUrl apiBaseUrl = m_authService->ApiBaseUrl();
    const QString host = apiBaseUrl.host().trimmed();
    if (host.isEmpty()) {
        return false;
    }

    int port = apiBaseUrl.port();
    if (port <= 0) {
        port = apiBaseUrl.scheme().compare(QStringLiteral("https"), Qt::CaseInsensitive) == 0 ? 443 : 80;
    }

    QTcpSocket socket;
    socket.connectToHost(host, static_cast<quint16>(port));
    const bool connected = socket.waitForConnected(250);
    if (connected) {
        socket.disconnectFromHost();
    }
    return connected;
}

QString DesktopFrontendServer::ResolveStaticFilePath(const QString& requestPath) const
{
    const QString rootPath = WebRootPath();
    const QString relative = requestPath == QStringLiteral("/") ? QStringLiteral("index.html") : requestPath.mid(1);
    const QString decoded = QUrl::fromPercentEncoding(relative.toUtf8());
    const QString cleanPath = QDir::cleanPath(decoded);
    if (cleanPath.isEmpty() || cleanPath.startsWith(QStringLiteral(".."))) {
        return QString();
    }

    const QString absolutePath = QDir(rootPath).absoluteFilePath(cleanPath);
    const QFileInfo info(absolutePath);
    if (!info.exists() || !info.isFile()) {
        return QString();
    }

    const QString canonicalRoot = QFileInfo(rootPath).canonicalFilePath();
    const QString canonicalFile = info.canonicalFilePath();
    if (!canonicalRoot.isEmpty() && !canonicalFile.startsWith(canonicalRoot, Qt::CaseInsensitive)) {
        return QString();
    }

    return absolutePath;
}

QString DesktopFrontendServer::WebRootPath() const
{
    return QStringLiteral("D:/Codes/cloud-cam-front/dist");
}

