#pragma once

#include "cloud_server_global.h"

#include <QString>

QJ_NAMESPACE_FIT_CLOUD_SERVER_BEGIN

enum class CloudFileSource
{
	Draft,
	Local,
	Cloud,
};

struct CloudFileState
{
	QString localFilePath;
	QString cloudFileUuid;
	CloudFileSource source = CloudFileSource::Draft;

	bool HasLocalFile() const
	{
		return !localFilePath.trimmed().isEmpty();
	}

	bool IsCloud() const
	{
		return source == CloudFileSource::Cloud && !cloudFileUuid.trimmed().isEmpty();
	}

	bool IsDraft() const
	{
		return source == CloudFileSource::Draft;
	}

	void ClearToDraft()
	{
		localFilePath.clear();
		cloudFileUuid.clear();
		source = CloudFileSource::Draft;
	}

	void AssignLocalFile(const QString& file_path)
	{
		localFilePath = file_path.trimmed();
		cloudFileUuid.clear();
		source = localFilePath.isEmpty() ? CloudFileSource::Draft : CloudFileSource::Local;
	}

	void AssignCloudFile(const QString& file_path, const QString& file_uuid)
	{
		localFilePath = file_path.trimmed();
		cloudFileUuid = file_uuid.trimmed();
		source = cloudFileUuid.isEmpty() ? CloudFileSource::Local : CloudFileSource::Cloud;
	}

	QString LocalFilePath() const
	{
		return localFilePath.trimmed();
	}
};

QJ_NAMESPACE_FIT_CLOUD_SERVER_END
