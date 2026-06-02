#pragma once

#include "project_global.h"
#include <string>

QJ_NAMESPACE_CAM_PROJECT_MANAGEMENT_BEGIN

class CamOptions
{
public:
	std::string GetRecentFileList() const;
};

extern CamOptions* CAMOptsPtr;

QJ_NAMESPACE_CAM_PROJECT_MANAGEMENT_END
