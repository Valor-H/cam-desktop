#pragma once

#include "cloud_server_global.h"

#include <QString>

QJ_NAMESPACE_FIT_CLOUD_SERVER_BEGIN

enum class OpenRequestSource
{
	Local,
	Cloud,
};

struct OpenRequestContext
{
	QString filePath;
	QString fileUuid;
	OpenRequestSource source = OpenRequestSource::Local;
	bool fromRecent = false;

	bool IsCloud() const
	{
		return source == OpenRequestSource::Cloud;
	}

	bool ShouldAddToLocalRecent() const
	{
		return source == OpenRequestSource::Local;
	}
};

QJ_NAMESPACE_FIT_CLOUD_SERVER_END
