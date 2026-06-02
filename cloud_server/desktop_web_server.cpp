#include "desktop_web_server.h"

#include "user_auth_service.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>

#include <hv/HttpServer.h>

QJ_NAMESPACE_FIT_CLOUD_SERVER_BEGIN

namespace
{
	constexpr int s_desktopWebServerBasePort = 31870;
	constexpr int s_desktopWebServerMaxPortScan = 200;

	int StartDesktopWebServer(hv::HttpServer& server, int preferred_port)
	{
		const int start_port = preferred_port > 0 ? preferred_port : s_desktopWebServerBasePort;

		for (int offset = 0; offset < s_desktopWebServerMaxPortScan; ++offset) {
			const int candidate_port = start_port + offset;
			server.setPort(candidate_port);
			if (server.start() == 0) {
				return candidate_port;
			}
		}

		return -1;
	}
}

struct DesktopWebServer::Private
{
	hv::HttpService service;
	hv::HttpServer server;
};

DesktopWebServer::DesktopWebServer(UserAuthService* auth_service, QObject* parent)
	: QObject(parent)
	, d(new Private)
	, _authService(auth_service)
	, _serviceRegistered(false)
	, _started(false)
	, _port(31870)
{
	ConfigureRoutes();
}

DesktopWebServer::~DesktopWebServer()
{
	Stop();
}

bool DesktopWebServer::Start()
{
	if (_started) {
		return true;
	}

	const QString index_path = QDir(WebRootPath()).filePath(QStringLiteral("index.html"));
	if (!QFileInfo::exists(index_path)) {
		return false;
	}

	if (!_serviceRegistered) {
		d->server.registerHttpService(&d->service);
		_serviceRegistered = true;
	}

	d->server.setHost("127.0.0.1");
	d->server.setThreadNum(1);

	const int port = StartDesktopWebServer(d->server, _port);
	if (port <= 0) {
		return false;
	}

	_port = port;
	_started = true;
	return true;
}

void DesktopWebServer::Stop()
{
	if (!_started) {
		return;
	}

	d->server.stop();
	_started = false;
}

QUrl DesktopWebServer::BaseUrl() const
{
	return QUrl(QStringLiteral("http://127.0.0.1:%1/").arg(_port));
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

		const QString file_path = ResolveStaticFilePath(path);
		if (!file_path.isEmpty()) {
			return resp->File(file_path.toStdString().c_str());
		}

		const QString index_path = QDir(WebRootPath()).filePath(QStringLiteral("index.html"));
		return resp->File(index_path.toStdString().c_str());
		});
}

QString DesktopWebServer::BuildRuntimeConfigJson() const
{
	QJsonObject payload{
		{ QStringLiteral("runtimeMode"), QStringLiteral("desktop") },
		{ QStringLiteral("backendUrl"), _authService ? _authService->ApiBaseUrl().toString().trimmed() : QString() },
		{ QStringLiteral("websocketUrl"),
			_authService ? _authService->Config().websocketUrl.toString().trimmed() : QString() },
		{ QStringLiteral("helpDocUrl"), _authService ? _authService->Config().helpDocUrl.toString().trimmed() : QString() },
		{ QStringLiteral("mockServiceUrl"),
			_authService ? _authService->Config().mockServiceUrl.toString().trimmed() : QString() },
	};

	return QString::fromUtf8(QJsonDocument(payload).toJson(QJsonDocument::Compact));
}

QString DesktopWebServer::ResolveStaticFilePath(const QString& request_path) const
{
	const QString root_path = WebRootPath();
	const QString relative_path = request_path == QStringLiteral("/") ? QStringLiteral("index.html") : request_path.mid(1);
	const QString decoded_path = QUrl::fromPercentEncoding(relative_path.toUtf8());
	const QString clean_path = QDir::cleanPath(decoded_path);
	if (clean_path.isEmpty() || clean_path.startsWith(QStringLiteral(".."))) {
		return QString();
	}

	const QString absolute_path = QDir(root_path).absoluteFilePath(clean_path);
	const QFileInfo info(absolute_path);
	if (!info.exists() || !info.isFile()) {
		return QString();
	}

	const QString canonical_root = QFileInfo(root_path).canonicalFilePath();
	const QString canonical_file = info.canonicalFilePath();
	if (!canonical_root.isEmpty() && !canonical_file.startsWith(canonical_root, Qt::CaseInsensitive)) {
		return QString();
	}

	return absolute_path;
}

QString DesktopWebServer::WebRootPath() const
{
	const QDir app_dir(QCoreApplication::applicationDirPath());

	const QString local_web_root = app_dir.absoluteFilePath(QStringLiteral("web"));
	if (QFileInfo::exists(QDir(local_web_root).filePath(QStringLiteral("index.html")))) {
		return local_web_root;
	}

	QDir runtime_root(app_dir);
	if (runtime_root.cdUp()) {
		const QString sibling_web_root = runtime_root.absoluteFilePath(QStringLiteral("web"));
		if (QFileInfo::exists(QDir(sibling_web_root).filePath(QStringLiteral("index.html")))) {
			return sibling_web_root;
		}
	}

	return local_web_root;
}

QJ_NAMESPACE_FIT_CLOUD_SERVER_END
