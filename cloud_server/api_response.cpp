#include "api_response.h"

QJ_NAMESPACE_FIT_CLOUD_SERVER_BEGIN

ApiResponse ParseApiResponse(const AuthHttpClient::Response& response)
{
	ApiResponse result;
	result.raw = response;

	if (!response.networkOk) {
		result.errorMessage = response.bizMsg.isEmpty()
			? QStringLiteral("Network request failed.")
			: response.bizMsg;
		return result;
	}

	if (response.httpStatus < 200 || response.httpStatus >= 300) {
		result.errorMessage = response.bizMsg.isEmpty()
			? QStringLiteral("HTTP error %1.").arg(response.httpStatus)
			: response.bizMsg;
		return result;
	}

	result.bizCode = response.bizCode;
	if (response.bizCode != 200) {
		result.errorMessage = response.bizMsg.isEmpty()
			? QStringLiteral("Server returned code %1.").arg(response.bizCode)
			: response.bizMsg;
		return result;
	}

	result.success = true;
	result.data = response.data;
	result.dataValue = response.dataValue;
	return result;
}

ApiDownloadResponse ParseApiDownloadResponse(const AuthHttpClient::DownloadResponse& response)
{
	ApiDownloadResponse result;
	result.localFilePath = response.targetFilePath;

	if (!response.networkOk) {
		result.errorMessage = response.errorMessage.isEmpty()
			? QStringLiteral("Download failed.")
			: response.errorMessage;
		return result;
	}

	if (response.httpStatus < 200 || response.httpStatus >= 300) {
		result.errorMessage = QStringLiteral("HTTP error %1.").arg(response.httpStatus);
		return result;
	}

	if (!response.writeOk) {
		result.errorMessage = response.errorMessage.isEmpty()
			? QStringLiteral("Failed to save downloaded file.")
			: response.errorMessage;
		return result;
	}

	result.success = true;
	return result;
}

QJ_NAMESPACE_FIT_CLOUD_SERVER_END
