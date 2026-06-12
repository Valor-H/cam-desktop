#include "cloud_file_service.h"

#include "file_api_service.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>

namespace
{
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
	, _apiService(new FileApiService(auth_service, this))
	, _saveInFlight(false)
{
}

CloudFileService::~CloudFileService() = default;

bool CloudFileService::SaveInFlight() const
{
	return _saveInFlight;
}

bool CloudFileService::UploadFileToTarget(const QString& local_file_path,
	const QString& folder_uuid,
	const QString& team_uuid,
	SaveCallback callback,
	QString* error_message)
{
	if (_saveInFlight) {
		if (error_message) {
			*error_message = QStringLiteral("Cloud upload is already running.");
		}
		return false;
	}

	const QString normalized_local_file_path = local_file_path.trimmed();
	const QFileInfo source_file(normalized_local_file_path);
	if (!source_file.exists() || !source_file.isFile()) {
		if (error_message) {
			*error_message = QStringLiteral("Current file is missing and cannot be uploaded.");
		}
		return false;
	}

	_saveInFlight = true;

	const bool started = _apiService->UploadFile(
		normalized_local_file_path,
		folder_uuid,
		team_uuid,
		[this, cb = std::move(callback)](const ApiResponse& response) {
			_saveInFlight = false;
			if (cb) {
				cb(response);
			}
		},
		error_message);

	if (!started) {
		_saveInFlight = false;
	}
	return started;
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

	const QString cache_file_path = BuildCacheFilePath(normalized_file_uuid, suggested_file_name);
	const QFileInfo cache_info(cache_file_path);
	QDir cache_dir(cache_info.dir());
	if (!cache_dir.exists() && !cache_dir.mkpath(QStringLiteral("."))) {
		if (error_message) {
			*error_message = QStringLiteral("Failed to create cache directory.");
		}
		return false;
	}

	return _apiService->DownloadFile(
		normalized_file_uuid,
		suggested_file_name,
		cache_file_path,
		[normalized_file_uuid, cb = std::move(callback)](const ApiDownloadResponse& response) {
			if (!cb) {
				return;
			}
			if (!response.success) {
				QFile::remove(response.localFilePath);
				cb(QString(), normalized_file_uuid, response.errorMessage);
				return;
			}
			cb(response.localFilePath, normalized_file_uuid, QString());
		},
		error_message);
}

bool CloudFileService::UpdateFileLastOpened(const QString& file_uuid, const QString& team_uuid)
{
	return _apiService->UpdateLastOpened(file_uuid, team_uuid);
}

bool CloudFileService::SaveCloudFile(const QString& local_file_path,
	const QString& file_uuid,
	SaveCallback callback,
	QString* error_message)
{
	if (_saveInFlight) {
		if (error_message) {
			*error_message = QStringLiteral("Cloud save is already running.");
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

	const QString normalized_local_file_path = local_file_path.trimmed();
	const QFileInfo source_file(normalized_local_file_path);
	if (!source_file.exists() || !source_file.isFile()) {
		if (error_message) {
			*error_message = QStringLiteral("Current cloud cache file is missing and cannot be uploaded.");
		}
		return false;
	}

	_saveInFlight = true;

	const bool started = _apiService->SaveFile(
		normalized_local_file_path,
		normalized_file_uuid,
		[this, cb = std::move(callback)](const ApiResponse& response) {
			_saveInFlight = false;
			if (cb) {
				cb(response);
			}
		},
		error_message);

	if (!started) {
		_saveInFlight = false;
	}
	return started;
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
