#include "cloud_file_service.h"

#include "auth_http_client.h"
#include "user_auth_service.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QUrl>
#include <QVariantMap>

#include <utility>

namespace
{
constexpr int kRecentFileDownloadTimeoutSec = 60;

QString apiBaseStringForClient(const QUrl& url)
{
    QString baseUrl = url.toString(QUrl::RemoveQuery | QUrl::RemoveFragment);
    while (baseUrl.endsWith(QLatin1Char('/'))) {
        baseUrl.chop(1);
    }
    return baseUrl;
}

QString sanitizeFileSegment(QString value)
{
    if (value.isEmpty()) {
        return QStringLiteral("recent-file");
    }

    return value;
}

QString cloudCacheDirectoryPath()
{
    QString baseDir = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    if (baseDir.trimmed().isEmpty()) {
        baseDir = QDir::tempPath();
    }

    return QDir(baseDir).filePath(QStringLiteral("QIANJIZN/QJCAM/cache-path"));
}
}

QJ_NAMESPACE_FIT_CLOUD_SERVER_BEGIN

CloudFileService::CloudFileService(UserAuthService* authService, QObject* parent)
    : QObject(parent)
    , _authService(authService)
{}

CloudFileService::~CloudFileService() = default;

void CloudFileService::ClearCurrentFile()
{
    _currentOpenedFilePath.clear();
    _currentOpenedCloudFileUuid.clear();
}

void CloudFileService::TrackOpenedLocalFile(const QString& filePath)
{
    _currentOpenedFilePath = filePath;
    _currentOpenedCloudFileUuid.clear();
}

void CloudFileService::TrackOpenedCloudFile(const QString& filePath, const QString& fileUuid)
{
    _currentOpenedFilePath = filePath;
    _currentOpenedCloudFileUuid = fileUuid.trimmed();
}

bool CloudFileService::IsCurrentFileCloud() const
{
    return !_currentOpenedCloudFileUuid.isEmpty();
}

bool CloudFileService::SaveInFlight() const
{
    return _saveInFlight;
}

bool CloudFileService::OpenCloudFile(const QString& fileUuid,
                                     const QString& suggestedFileName,
                                     OpenCallback callback,
                                     QString* errorMessage)
{
    const QString normalizedFileUuid = fileUuid.trimmed();
    if (normalizedFileUuid.isEmpty()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Cloud recent file request is missing fileUuid.");
        }
        return false;
    }

    const QString token = _authService && _authService->Session()
        ? _authService->Session()->AuthToken().trimmed()
        : QString();
    if (token.isEmpty()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Current session is missing token.");
        }
        return false;
    }

    const QString cacheFilePath = BuildCacheFilePath(normalizedFileUuid, suggestedFileName);
    const QFileInfo cacheInfo(cacheFilePath);
    QDir cacheDir(cacheInfo.dir());
    if (!cacheDir.exists() && !cacheDir.mkpath(QStringLiteral("."))) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Failed to create cache directory.");
        }
        return false;
    }

    AuthHttpClient* httpClient = EnsureHttpClient();
    if (!httpClient) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Desktop download client initialization failed.");
        }
        return false;
    }

    QJsonObject requestBody {
        { QStringLiteral("fileUuid"), normalizedFileUuid },
    };

    httpClient->PostJsonToFile(
        QStringLiteral("/api/file/getFile"),
        token,
        QJsonDocument(requestBody).toJson(QJsonDocument::Compact),
        cacheFilePath,
        kRecentFileDownloadTimeoutSec,
        [this, normalizedFileUuid, cb = std::move(callback)](const AuthHttpClient::DownloadResponse& response) mutable {
            if (!response.networkOk) {
                QFile::remove(response.targetFilePath);
                if (cb) {
                    cb(QString(), normalizedFileUuid, QStringLiteral("Cloud file download failed: %1").arg(response.errorMessage));
                }
                return;
            }

            if (!response.writeOk) {
                QFile::remove(response.targetFilePath);
                if (cb) {
                    cb(QString(), normalizedFileUuid, QStringLiteral("Cloud file cache write failed: %1").arg(response.errorMessage));
                }
                return;
            }

            if (cb) {
                cb(response.targetFilePath, normalizedFileUuid, QString());
            }
        });

    return true;
}

bool CloudFileService::SaveCurrentCloudFile(SaveCallback callback, QString* errorMessage)
{
    if (_saveInFlight) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Cloud save is already running.");
        }
        return false;
    }

    if (_currentOpenedCloudFileUuid.isEmpty()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Current file is not a cloud file.");
        }
        return false;
    }

    const QFileInfo sourceFile(_currentOpenedFilePath);
    if (!sourceFile.exists() || !sourceFile.isFile()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Current cloud cache file is missing and cannot be uploaded.");
        }
        return false;
    }

    const QString token = _authService && _authService->Session()
        ? _authService->Session()->AuthToken().trimmed()
        : QString();
    if (token.isEmpty()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Current session is missing token.");
        }
        return false;
    }

    AuthHttpClient* httpClient = EnsureHttpClient();
    if (!httpClient) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Cloud save client initialization failed.");
        }
        return false;
    }

    _saveInFlight = true;

    const QVariantMap formFields {
        { QStringLiteral("fileUuid"), _currentOpenedCloudFileUuid },
        { QStringLiteral("fileVersion"), QStringLiteral("666") },
        { QStringLiteral("override"), QStringLiteral("true") },
    };

    httpClient->PostMultipartFile(
        QStringLiteral("/api/file/updateFile"),
        token,
        QStringLiteral("file"),
        _currentOpenedFilePath,
        formFields,
        60,
        [this, cb = std::move(callback)](const AuthHttpClient::Response& response) mutable {
            _saveInFlight = false;
            if (cb) {
                cb(response);
            }
        });

    return true;
}

AuthHttpClient* CloudFileService::EnsureHttpClient()
{
    if (_httpClient) {
        return _httpClient;
    }

    if (!_authService) {
        return nullptr;
    }

    _httpClient = new AuthHttpClient(apiBaseStringForClient(_authService->ApiBaseUrl()), this);
    return _httpClient;
}

QString CloudFileService::BuildCacheFilePath(const QString& fileUuid, const QString& suggestedFileName) const
{
    QString candidate = sanitizeFileSegment(suggestedFileName);
    if (!candidate.endsWith(QStringLiteral(".qjp"), Qt::CaseInsensitive)) {
        candidate = sanitizeFileSegment(fileUuid);
        if (!candidate.endsWith(QStringLiteral(".qjp"), Qt::CaseInsensitive)) {
            candidate += QStringLiteral(".qjp");
        }
    }

    return QDir::fromNativeSeparators(cloudCacheDirectoryPath()) + QLatin1Char('/') + candidate;
}

QJ_NAMESPACE_FIT_CLOUD_SERVER_END