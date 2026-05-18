#include "desktop_web_server.h"

#include "desktop_runtime_injection.h"
#include "user_auth_service.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>

#include <hv/HttpServer.h>

QJ_NAMESPACE_FIT_CLOUD_SERVER_BEGIN

struct DesktopWebServer::Private
{
    hv::HttpService service;
    hv::HttpServer server;
};

namespace
{
QString InjectBootstrapScriptIntoHtml(QString html, QString script)
{
    if (html.isEmpty() || script.isEmpty()) {
        return html;
    }

    script.replace(QStringLiteral("</script"), QStringLiteral("<\\/script"), Qt::CaseInsensitive);
    const QString injection = QStringLiteral("<script>%1</script>").arg(script);

    const qsizetype headIndex = html.indexOf(QStringLiteral("</head>"), 0, Qt::CaseInsensitive);
    if (headIndex >= 0) {
        html.insert(headIndex, injection);
        return html;
    }

    const qsizetype bodyIndex = html.indexOf(QStringLiteral("</body>"), 0, Qt::CaseInsensitive);
    if (bodyIndex >= 0) {
        html.insert(bodyIndex, injection);
        return html;
    }

    html.prepend(injection);
    return html;
}
}

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
        const QString filePath = ResolveStaticFilePath(path);
        if (!filePath.isEmpty()) {
            if (QFileInfo(filePath).fileName().compare(QStringLiteral("index.html"), Qt::CaseInsensitive) == 0) {
                const QString html = BuildIndexHtmlResponse();
                if (html.isEmpty()) {
                    return HTTP_STATUS_NOT_FOUND;
                }
                resp->SetContentType(TEXT_HTML);
                resp->SetBody(html.toStdString());
                return HTTP_STATUS_OK;
            }
            return resp->File(filePath.toStdString().c_str());
        }

        const QString html = BuildIndexHtmlResponse();
        if (html.isEmpty()) {
            return HTTP_STATUS_NOT_FOUND;
        }

        resp->SetContentType(TEXT_HTML);
        resp->SetBody(html.toStdString());
        return HTTP_STATUS_OK;
    });
}

QString DesktopWebServer::BuildIndexHtmlResponse() const
{
    const QString indexPath = QDir(WebRootPath()).filePath(QStringLiteral("index.html"));
    QFile indexFile(indexPath);
    if (!indexFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString();
    }

    const QString html = QString::fromUtf8(indexFile.readAll());
    const QString bootstrapScript = BuildDesktopRuntimeInjectionScript(m_authService);
    return InjectBootstrapScriptIntoHtml(html, bootstrapScript);
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
