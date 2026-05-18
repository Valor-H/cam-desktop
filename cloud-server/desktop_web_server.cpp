#include "desktop_web_server.h"

#include "user_auth_service.h"

#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>

#include <hv/HttpServer.h>

QJ_NAMESPACE_FIT_CLOUD_SERVER_BEGIN

struct DesktopWebServer::Private
{
    hv::HttpService service;
    hv::HttpServer server;
};

DesktopWebServer::DesktopWebServer(UserAuthService* authService, QObject* parent)
    : QObject(parent)
    , m_authService(authService)
    , d(new Private)
{
    ConfigureRoutes();
}

DesktopWebServer::~DesktopWebServer()
{
    Stop();
}

bool DesktopWebServer::Start()
{
    if (m_started) {
        return true;
    }

    const QString indexPath = QDir(WebRootPath()).filePath(QStringLiteral("index.html"));
    if (!QFileInfo::exists(indexPath)) {
        return false;
    }

    d->server.registerHttpService(&d->service);
    d->server.setHost("127.0.0.1");
    d->server.setPort(m_port);
    d->server.setThreadNum(1);
    if (d->server.start() != 0) {
        return false;
    }

    m_started = true;
    return true;
}

void DesktopWebServer::Stop()
{
    if (!m_started) {
        return;
    }

    d->server.stop();
    m_started = false;
}

QUrl DesktopWebServer::BaseUrl() const
{
    return QUrl(QStringLiteral("http://127.0.0.1:%1/").arg(m_port));
}

void DesktopWebServer::ConfigureRoutes()
{
    d->service.AllowCORS();

    d->service.Use([this](HttpRequest* req, HttpResponse* resp) -> int {
        if (req->method != HTTP_GET && req->method != HTTP_HEAD) {
            return HTTP_STATUS_NEXT;
        }

        const QString path =
            (!req || req->Path().empty()) ? QStringLiteral("/") : QString::fromStdString(req->Path());
        if (path.compare(QStringLiteral("/config.json"), Qt::CaseInsensitive) == 0) {
            const QString json = BuildRuntimeConfigJson();
            if (json.isEmpty()) {
                return HTTP_STATUS_INTERNAL_SERVER_ERROR;
            }

            resp->SetContentType(APPLICATION_JSON);
            resp->SetBody(json.toStdString());
            return HTTP_STATUS_OK;
        }

        const QString filePath = ResolveStaticFilePath(path);
        if (!filePath.isEmpty()) {
            return resp->File(filePath.toStdString().c_str());
        }

        const QString indexPath = QDir(WebRootPath()).filePath(QStringLiteral("index.html"));
        return resp->File(indexPath.toStdString().c_str());
    });
}

QString DesktopWebServer::BuildRuntimeConfigJson() const
{
    QJsonObject payload {
        { QStringLiteral("runtimeMode"), QStringLiteral("desktop") },
        { QStringLiteral("backendUrl"), m_authService ? m_authService->ApiBaseUrl().toString().trimmed() : QString() },
        { QStringLiteral("websocketUrl"), m_authService ? m_authService->Config().websocketUrl.toString().trimmed() : QString() },
        { QStringLiteral("helpDocUrl"), m_authService ? m_authService->Config().helpDocUrl.toString().trimmed() : QString() },
        { QStringLiteral("mockServiceUrl"), m_authService ? m_authService->Config().mockServiceUrl.toString().trimmed() : QString() },
    };

    return QString::fromUtf8(QJsonDocument(payload).toJson(QJsonDocument::Compact));
}

QString DesktopWebServer::ResolveStaticFilePath(const QString& requestPath) const
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

QString DesktopWebServer::WebRootPath() const
{
    return QStringLiteral("D:/Codes/cloud-cam-front/dist");
}

QJ_NAMESPACE_FIT_CLOUD_SERVER_END
