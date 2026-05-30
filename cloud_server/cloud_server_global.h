#pragma once

#ifndef CLOUD_SERVER_GLOBAL_H
#define CLOUD_SERVER_GLOBAL_H

#include <common/namespace/qj_namespace_def.h>
#include <QtCore/qglobal.h>


#ifdef cloud_server_EXPORTS
#define CLOUD_SERVER_EXPORT Q_DECL_EXPORT
#else
#define CLOUD_SERVER_EXPORT Q_DECL_IMPORT
#endif

#define QJ_NAMESPACE_FIT_CLOUD_SERVER_BEGIN namespace qianjizn { namespace cloudserver {
#define QJ_NAMESPACE_FIT_CLOUD_SERVER_END } }
#define QJ_USING_NAMESPACE_FIT_CLOUD_SERVER using namespace qianjizn::cloudserver;

#endif
