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
    using OpenCallback = std::function<void(const QString& localFilePath, const QString& fileUuid, const QString& errorMessage)>;
    using SaveCallback = std::function<void(const AuthHttpClient::Response&)>;

    explicit CloudFileService(UserAuthService* authService, QObject* parent = nullptr);
    ~CloudFileService() override;

    void ClearCurrentFile();
    void TrackOpenedLocalFile(const QString& filePath);
    void TrackOpenedCloudFile(const QString& filePath, const QString& fileUuid);
    bool IsCurrentFileCloud() const;
    bool SaveInFlight() const;
    bool OpenCloudFile(const QString& fileUuid,
                       const QString& suggestedFileName,
                       OpenCallback callback,
                       QString* errorMessage = nullptr);
    bool SaveCurrentCloudFile(SaveCallback callback, QString* errorMessage = nullptr);

private:
    AuthHttpClient* EnsureHttpClient();
    QString BuildCacheFilePath(const QString& fileUuid, const QString& suggestedFileName) const;

    UserAuthService* _authService { nullptr };
    AuthHttpClient* _httpClient { nullptr };
    QString _currentOpenedFilePath;
    QString _currentOpenedCloudFileUuid;
    bool _saveInFlight { false };
};

QJ_NAMESPACE_FIT_CLOUD_SERVER_END