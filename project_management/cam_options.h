#pragma once

#include "project_global.h"
#include <string>

QJ_NAMESPACE_CAM_PROJECT_MANAGEMENT_BEGIN

class CamOptions
{
public:
	bool Read();
	std::string GetRecentFileList() const;
	void SetRecentFileList(const std::string& path);

private:
	bool Write() const;
	void EnsureLoaded() const;

private:
	mutable bool _loaded{ false };
	std::string _recentFileList;
};

extern CamOptions* CAMOptsPtr;

QJ_NAMESPACE_CAM_PROJECT_MANAGEMENT_END
