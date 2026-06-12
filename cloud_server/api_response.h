#pragma once

#include "cloud_server_global.h"
#include "auth_http_client.h"

#include <QVariantMap>

QJ_NAMESPACE_FIT_CLOUD_SERVER_BEGIN

/** 封装 API 层统一响应结果。 */
struct ApiResponse
{
	bool success{ false };        /** 标识请求是否成功（网络 + HTTP + 业务码全部通过）。 */
	QString errorMessage;          /** 人类可读的错误描述，失败时填充。 */
	int bizCode{ -1 };            /** 原始业务层返回码。 */
	QVariantMap data;            /** 解析后的业务数据对象。 */
	QVariant dataValue;          /** 原始业务层返回 data 值。 */
	AuthHttpClient::Response raw; /** 原始 HTTP 响应（用于高级场景）。 */
};

/** 封装 API 层下载响应结果。 */
struct ApiDownloadResponse
{
	bool success{ false };         /** 标识下载是否成功（网络 + HTTP + 写入）。 */
	QString errorMessage;          /** 人类可读的错误描述。 */
	QString localFilePath;         /** 下载后的本地文件路径。 */
};

/** 解析 HTTP 响应为统一 API 响应。
 *  自动检查 networkOk / HTTP 状态码 / bizCode，并填充错误信息。 */
CLOUD_SERVER_EXPORT ApiResponse ParseApiResponse(const AuthHttpClient::Response& response);

/** 解析 HTTP 下载响应为统一下载响应。
 *  自动检查网络、HTTP 状态码和文件写入。 */
CLOUD_SERVER_EXPORT ApiDownloadResponse ParseApiDownloadResponse(const AuthHttpClient::DownloadResponse& response);

QJ_NAMESPACE_FIT_CLOUD_SERVER_END
