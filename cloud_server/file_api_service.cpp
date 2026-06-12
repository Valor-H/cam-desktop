#include "file_api_service.h"

#include "user_auth_service.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>

namespace
{
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

FileApiService::FileApiService(UserAuthService* auth_service, QObject* parent)
	: QObject(parent)
	, _authService(auth_service)
	, _httpClient(nullptr)
{
}

FileApiService::~FileApiService() = default;

QString FileApiService::CurrentToken() const
{
	if (!_authService || !_authService->Session()) {
		return QString();
	}
	return _authService->Session()->AuthToken().trimmed();
}

AuthHttpClient* FileApiService::EnsureHttpClient()
{
	if (_httpClient) {
		return _httpClient;
	}
	if (!_authService) {
		return nullptr;
	}
	_httpClient = new AuthHttpClient(ApiBaseStringForClient(_authService->ApiBaseUrl()), this);
	return _httpClient;
}

bool FileApiService::UploadFile(const QString& local_file_path,
	const QString& folder_uuid,
	const QString& team_uuid,
	ResponseCallback callback,
	QString* error_message)
{
	const QString token = CurrentToken();
	if (token.isEmpty()) {
		if (error_message) {
			*error_message = QStringLiteral("Current session is missing token.");
		}
		return false;
	}

	AuthHttpClient* http_client = EnsureHttpClient();
	if (!http_client) {
		if (error_message) {
			*error_message = QStringLiteral("Cloud upload client initialization failed.");
		}
		return false;
	}

	const QString normalized_folder_uuid = folder_uuid.trimmed();
	if (normalized_folder_uuid.isEmpty()) {
		if (error_message) {
			*error_message = QStringLiteral("Upload target folder is missing.");
		}
		return false;
	}

	QVariantMap form_fields{
		{ QStringLiteral("parentUuid"), normalized_folder_uuid },
	};

	const QString normalized_team_uuid = team_uuid.trimmed();
	QString api_path = QStringLiteral("/api/file/addFile");
	if (!normalized_team_uuid.isEmpty()) {
		form_fields.insert(QStringLiteral("teamUuid"), normalized_team_uuid);
		api_path = QStringLiteral("/api/file/addTeamFile");
	}

	http_client->PostMultipartFile(
		api_path,
		token,
		QStringLiteral("files"),
		local_file_path.trimmed(),
		form_fields,
		60,
		[cb = std::move(callback)](const AuthHttpClient::Response& response) {
			if (cb) {
				cb(ParseApiResponse(response));
			}
		});

	return true;
}

bool FileApiService::DownloadFile(const QString& file_uuid,
	const QString& /*suggested_name*/,
	const QString& cache_file_path,
	DownloadCallback callback,
	QString* error_message)
{
	const QString token = CurrentToken();
	if (token.isEmpty()) {
		if (error_message) {
			*error_message = QStringLiteral("Current session is missing token.");
		}
		return false;
	}

	AuthHttpClient* http_client = EnsureHttpClient();
	if (!http_client) {
		if (error_message) {
			*error_message = QStringLiteral("Desktop download client initialization failed.");
		}
		return false;
	}

	const QString normalized_file_uuid = file_uuid.trimmed();
	if (normalized_file_uuid.isEmpty()) {
		if (error_message) {
			*error_message = QStringLiteral("Cloud file request is missing fileUuid.");
		}
		return false;
	}

	QJsonObject request_body{
		{ QStringLiteral("fileUuid"), normalized_file_uuid },
	};

	http_client->PostJsonToFile(
		QStringLiteral("/api/file/getFile"),
		token,
		QJsonDocument(request_body).toJson(QJsonDocument::Compact),
		cache_file_path,
		60,
		[cb = std::move(callback)](const AuthHttpClient::DownloadResponse& response) {
			if (cb) {
				cb(ParseApiDownloadResponse(response));
			}
		});

	return true;
}

bool FileApiService::UpdateLastOpened(const QString& file_uuid, const QString& team_uuid)
{
	const QString token = CurrentToken();
	if (token.isEmpty()) {
		return false;
	}

	AuthHttpClient* http_client = EnsureHttpClient();
	if (!http_client) {
		return false;
	}

	const QString normalized_file_uuid = file_uuid.trimmed();
	if (normalized_file_uuid.isEmpty()) {
		return false;
	}

	QJsonObject request_body{
		{ QStringLiteral("fileUuid"), normalized_file_uuid },
	};

	const QString normalized_team_uuid = team_uuid.trimmed();
	const QString api_path = normalized_team_uuid.isEmpty()
		? QStringLiteral("/api/file/updateLastOpened")
		: QStringLiteral("/api/file/updateTeamLastOpened");

	if (!normalized_team_uuid.isEmpty()) {
		request_body.insert(QStringLiteral("teamUuid"), normalized_team_uuid);
	}

	// Fire-and-forget: 静默失败不影响主流程
	http_client->PostJson(
		api_path,
		token,
		QJsonDocument(request_body).toJson(QJsonDocument::Compact),
		10,
		[](const AuthHttpClient::Response& /*response*/) {
			// 静默忽略
		});

	return true;
}

bool FileApiService::SaveFile(const QString& local_file_path,
	const QString& file_uuid,
	ResponseCallback callback,
	QString* error_message)
{
	const QString token = CurrentToken();
	if (token.isEmpty()) {
		if (error_message) {
			*error_message = QStringLiteral("Current session is missing token.");
		}
		return false;
	}

	AuthHttpClient* http_client = EnsureHttpClient();
	if (!http_client) {
		if (error_message) {
			*error_message = QStringLiteral("Cloud save client initialization failed.");
		}
		return false;
	}

	const QString normalized_file_uuid = file_uuid.trimmed();
	if (normalized_file_uuid.isEmpty()) {
		if (error_message) {
			*error_message = QStringLiteral("Current file is not a cloud file.");
		}
		return false;
	}

	const QVariantMap form_fields{
		{ QStringLiteral("fileUuid"), normalized_file_uuid },
		{ QStringLiteral("fileVersion"), QStringLiteral("666") },
		{ QStringLiteral("override"), QStringLiteral("true") },
	};

	http_client->PostMultipartFile(
		QStringLiteral("/api/file/updateFile"),
		token,
		QStringLiteral("file"),
		local_file_path.trimmed(),
		form_fields,
		60,
		[cb = std::move(callback)](const AuthHttpClient::Response& response) {
			if (cb) {
				cb(ParseApiResponse(response));
			}
		});

	return true;
}

QJ_NAMESPACE_FIT_CLOUD_SERVER_END
