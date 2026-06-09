#pragma once

#include "cloud_server_global.h"
#include "auth_http_client.h"

#include <QString>
#include <QObject>

#include <functional>

QJ_NAMESPACE_FIT_CLOUD_SERVER_BEGIN

class UserAuthService;

class CLOUD_SERVER_EXPORT CloudFileService final : public QObject
{
	Q_OBJECT

public:
	using OpenCallback = std::function<void(const QString& local_file_path,
		const QString& file_uuid,
		const QString& error_message)>;
	using SaveCallback = std::function<void(const AuthHttpClient::Response&)>;

	explicit CloudFileService(UserAuthService* auth_service, QObject* parent = nullptr);
	~CloudFileService() override;

	bool SaveInFlight() const;
	bool UploadFileToTarget(const QString& local_file_path,
		const QString& folder_uuid,
		const QString& team_uuid,
		SaveCallback callback,
		QString* error_message = nullptr);
	bool OpenCloudFile(const QString& file_uuid,
		const QString& suggested_file_name,
		OpenCallback callback,
		QString* error_message = nullptr);
	bool SaveCloudFile(const QString& local_file_path,
		const QString& file_uuid,
		SaveCallback callback,
		QString* error_message = nullptr);

private:
	AuthHttpClient* EnsureHttpClient();
	QString BuildCacheFilePath(const QString& file_uuid, const QString& suggested_file_name) const;

	UserAuthService* _authService;
	AuthHttpClient* _httpClient;
	bool _saveInFlight;
};

QJ_NAMESPACE_FIT_CLOUD_SERVER_END
