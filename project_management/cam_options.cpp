#include "cam_options.h"

QJ_NAMESPACE_CAM_PROJECT_MANAGEMENT_BEGIN

namespace
{
	CamOptions g_camOptions;
}

CamOptions* g_camOptsPtr = &g_camOptions;

std::string CamOptions::GetRecentFileList() const
{
	return "D:\\Temps\\新建 112.qjp;D:\\Temps\\12345 测试.qjp;D:\\Temps\\车削123.qjp";
}

QJ_NAMESPACE_CAM_PROJECT_MANAGEMENT_END
