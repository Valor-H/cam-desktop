#pragma once

#include "ultramill_global.h"

#include <QString>

QJ_NAMESPACE_ULTRACAM_ULTRAMILL_BEGIN

enum class WorkspaceFileSource
{
	Local,
	Cloud,
};

struct WorkspaceFileContext
{
	QString filePath;
	QString fileUuid;
	WorkspaceFileSource source = WorkspaceFileSource::Local;
	bool fromRecent = false;

	bool IsCloud() const
	{
		return source == WorkspaceFileSource::Cloud;
	}

	bool ShouldAddToLocalRecent() const
	{
		return source == WorkspaceFileSource::Local;
	}
};

QJ_NAMESPACE_ULTRACAM_ULTRAMILL_END
