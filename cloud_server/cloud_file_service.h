#pragma once

#include "cloud_server_global.h"
#include "api_response.h"

#include <QString>
#include <QObject>

#include <functional>

QJ_NAMESPACE_FIT_CLOUD_SERVER_BEGIN

class UserAuthService;
class FileApiService;

class CLOUD_SERVER_EXPORT CloudFileService final : public QObject
{
	Q_OBJECT

public:
	/** 定义云端文件打开完成回调类型。 */
	using OpenCallback = std::function<void(const QString& local_file_path,
		const QString& file_uuid,
		const QString& error_message)>;
	/** 定义云端保存/上传完成回调类型。 */
	using SaveCallback = std::function<void(const ApiResponse&)>;

	/** 构造云文件服务。 */
	explicit CloudFileService(UserAuthService* auth_service, QObject* parent = nullptr);
	/** 析构云文件服务。 */
	~CloudFileService() override;

	/** 检查是否有保存/上传操作正在进行。 */
	bool SaveInFlight() const;
	/** 上传文件到指定目标。 */
	bool UploadFileToTarget(const QString& local_file_path,
		const QString& folder_uuid,
		const QString& team_uuid,
		SaveCallback callback,
		QString* error_message = nullptr);
	/** 打开云端文件。 */
	bool OpenCloudFile(const QString& file_uuid,
		const QString& suggested_file_name,
		OpenCallback callback,
		QString* error_message = nullptr);
	/** 更新文件最近打开时间。 */
	bool UpdateFileLastOpened(const QString& file_uuid,
		const QString& team_uuid = QString());
	/** 保存云端文件。 */
	bool SaveCloudFile(const QString& local_file_path,
		const QString& file_uuid,
		SaveCallback callback,
		QString* error_message = nullptr);

private:
	/** 构建云端文件缓存路径。 */
	QString BuildCacheFilePath(const QString& file_uuid, const QString& suggested_file_name) const;

	/** 保存文件 API 服务实例。 */
	FileApiService* _apiService;
	/** 标识当前是否有保存/上传操作正在进行。 */
	bool _saveInFlight;
};

QJ_NAMESPACE_FIT_CLOUD_SERVER_END
