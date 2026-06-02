#include "cloud_file_service.h"

#include "auth_http_client.h"
#include "user_auth_service.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QUrl>
#include <QVariantMap>

#include <utility>

namespace
{
	constexpr int s_recentFileDownloadTimeoutSec = 60;

	QString ApiBaseStringForClient(const QUrl& url)
	{
		QString base_url = url.toString(QUrl::RemoveQuery | QUrl::RemoveFragment);
		while (base_url.endsWith(QLatin1Char('/'))) {
			base_url.chop(1);
		}
		return base_url;
	}

	QString SanitizeFileSegment(QString value)
	{
		if (value.isEmpty()) {
			return QStringLiteral("recent-file");
		}

		return value;
	}

	QString CloudCacheDirectoryPath()
	{
		QString base_dir = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
		if (base_dir.trimmed().isEmpty()) {
			base_dir = QDir::tempPath();
		}

		return QDir(base_dir).filePath(QStringLiteral("QIANJIZN/QJCAM/cache-path"));
	}
}

QJ_NAMESPACE_FIT_CLOUD_SERVER_BEGIN

CloudFileService::CloudFileService(UserAuthService* auth_service, QObject* parent)
	: QObject(parent)
	, _authService(auth_service)
	, _httpClient(nullptr)
	, _currentOpenedFilePath()
	, _currentOpenedCloudFileUuid()
	, _saveInFlight(false)
{
}

CloudFileService::~CloudFileService() = default;

void CloudFileService::ClearCurrentFile()
{
	_currentOpenedFilePath.clear();
	_currentOpenedCloudFileUuid.clear();
}

void CloudFileService::TrackOpenedLocalFile(const QString& file_path)
{
	_currentOpenedFilePath = file_path;
	_currentOpenedCloudFileUuid.clear();
}

void CloudFileService::TrackOpenedCloudFile(const QString& file_path, const QString& file_uuid)
{
	_currentOpenedFilePath = file_path;
	_currentOpenedCloudFileUuid = file_uuid.trimmed();
}

bool CloudFileService::IsCurrentFileCloud() const
{
	return !_currentOpenedCloudFileUuid.isEmpty();
}

bool CloudFileService::SaveInFlight() const
{
	return _saveInFlight;
}

bool CloudFileService::OpenCloudFile(const QString& file_uuid,
	const QString& suggested_file_name,
	OpenCallback callback,
	QString* error_message)
{
	const QString normalized_file_uuid = file_uuid.trimmed();
	if (normalized_file_uuid.isEmpty()) {
		if (error_message) {
			*error_message = QStringLiteral("Cloud recent file request is missing fileUuid.");
		}
		return false;
	}

	const QString token = _authService && _authService->Session()
		? _authService->Session()->AuthToken().trimmed()
		: QString();
	if (token.isEmpty()) {
		if (error_message) {
			*error_message = QStringLiteral("Current session is missing token.");
		}
		return false;
	}

	const QString cache_file_path = BuildCacheFilePath(normalized_file_uuid, suggested_file_name);
	const QFileInfo cache_info(cache_file_path);
	QDir cache_dir(cache_info.dir());
	if (!cache_dir.exists() && !cache_dir.mkpath(QStringLiteral("."))) {
		if (error_message) {
			*error_message = QStringLiteral("Failed to create cache directory.");
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

	QJsonObject request_body{
		{ QStringLiteral("fileUuid"), normalized_file_uuid },
	};

	http_client->PostJsonToFile(
		QStringLiteral("/api/file/getFile"),
		token,
		QJsonDocument(request_body).toJson(QJsonDocument::Compact),
		cache_file_path,
		s_recentFileDownloadTimeoutSec,
		[this, normalized_file_uuid, cb = std::move(callback)](const AuthHttpClient::DownloadResponse& response) mutable {
			if (!response.networkOk) {
				QFile::remove(response.targetFilePath);
				if (cb) {
					cb(QString(), normalized_file_uuid, QStringLiteral("Cloud file download failed: %1").arg(response.errorMessage));
				}
				return;
			}

			if (!response.writeOk) {
				QFile::remove(response.targetFilePath);
				if (cb) {
					cb(QString(),
						normalized_file_uuid,
						QStringLiteral("Cloud file cache write failed: %1").arg(response.errorMessage));
				}
				return;
			}

			if (cb) {
				cb(response.targetFilePath, normalized_file_uuid, QString());
			}
		});

	return true;
}

bool CloudFileService::SaveCurrentCloudFile(SaveCallback callback, QString* error_message)
{
	if (_saveInFlight) {
		if (error_message) {
			*error_message = QStringLiteral("Cloud save is already running.");
		}
		return false;
	}

	if (_currentOpenedCloudFileUuid.isEmpty()) {
		if (error_message) {
			*error_message = QStringLiteral("Current file is not a cloud file.");
		}
		return false;
	}

	const QFileInfo source_file(_currentOpenedFilePath);
	if (!source_file.exists() || !source_file.isFile()) {
		if (error_message) {
			*error_message = QStringLiteral("Current cloud cache file is missing and cannot be uploaded.");
		}
		return false;
	}

	const QString token = _authService && _authService->Session()
		? _authService->Session()->AuthToken().trimmed()
		: QString();
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

	_saveInFlight = true;

	const QVariantMap form_fields{
		{ QStringLiteral("fileUuid"), _currentOpenedCloudFileUuid },
		{ QStringLiteral("fileVersion"), QStringLiteral("666") },
		{ QStringLiteral("override"), QStringLiteral("true") },
	};

	http_client->PostMultipartFile(
		QStringLiteral("/api/file/updateFile"),
		token,
		QStringLiteral("file"),
		_currentOpenedFilePath,
		form_fields,
		60,
		[this, cb = std::move(callback)](const AuthHttpClient::Response& response) mutable {
			_saveInFlight = false;
			if (cb) {
				cb(response);
			}
		});

	return true;
}

AuthHttpClient* CloudFileService::EnsureHttpClient()
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

QString CloudFileService::BuildCacheFilePath(const QString& file_uuid, const QString& suggested_file_name) const
{
	QString candidate = SanitizeFileSegment(suggested_file_name);
	if (!candidate.endsWith(QStringLiteral(".qjp"), Qt::CaseInsensitive)) {
		candidate = SanitizeFileSegment(file_uuid);
		if (!candidate.endsWith(QStringLiteral(".qjp"), Qt::CaseInsensitive)) {
			candidate += QStringLiteral(".qjp");
		}
	}

	return QDir::fromNativeSeparators(CloudCacheDirectoryPath()) + QLatin1Char('/') + candidate;
}

QJ_NAMESPACE_FIT_CLOUD_SERVER_END
