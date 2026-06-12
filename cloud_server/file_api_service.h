#pragma once

#include "cloud_server_global.h"
#include "api_response.h"

#include <QObject>
#include <QString>

QJ_NAMESPACE_FIT_CLOUD_SERVER_BEGIN

class UserAuthService;

/** 封装云端文件相关的 HTTP API 调用。 */
class CLOUD_SERVER_EXPORT FileApiService final : public QObject
{
	Q_OBJECT

public:
	using ResponseCallback = std::function<void(const ApiResponse&)>;
	using DownloadCallback = std::function<void(const ApiDownloadResponse&)>;

	/** 构造文件 API 服务。 */
	explicit FileApiService(UserAuthService* auth_service, QObject* parent = nullptr);
	/** 析构文件 API 服务。 */
	~FileApiService() override;

	/** 上传文件到指定文件夹。 */
	bool UploadFile(const QString& local_file_path,
		const QString& folder_uuid,
		const QString& team_uuid,
		ResponseCallback callback,
		QString* error_message = nullptr);

	/** 下载云端文件到本地缓存。 */
	bool DownloadFile(const QString& file_uuid,
		const QString& suggested_name,
		const QString& cache_file_path,
		DownloadCallback callback,
		QString* error_message = nullptr);

	/** 更新文件最近打开时间（fire-and-forget）。 */
	bool UpdateLastOpened(const QString& file_uuid,
		const QString& team_uuid = QString());

	/** 保存（覆盖上传）云端文件。 */
	bool SaveFile(const QString& local_file_path,
		const QString& file_uuid,
		ResponseCallback callback,
		QString* error_message = nullptr);

private:
	/** 获取当前会话 Token。 */
	QString CurrentToken() const;
	/** 确保 HTTP 客户端已初始化。 */
	AuthHttpClient* EnsureHttpClient();

	UserAuthService* _authService;
	AuthHttpClient* _httpClient;
};

QJ_NAMESPACE_FIT_CLOUD_SERVER_END
