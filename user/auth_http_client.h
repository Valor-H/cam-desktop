#pragma once

#include <QByteArray>
#include <QObject>
#include <QString>
#include <QVariantMap>

#include <atomic>
#include <functional>
#include <memory>

class AuthHttpClient : public QObject
{
public:
    struct Response
    {
        bool        networkOk { false };
        int         httpStatus { 0 };
        int         bizCode { -1 };
        QString     bizMsg;
        QVariantMap data;
    };

    struct DownloadResponse
    {
        bool    networkOk { false };
        int     httpStatus { 0 };
        bool    writeOk { false };
        QString errorMessage;
        QString targetFilePath;
    };

    using Callback = std::function<void(const Response&)>;
    using DownloadCallback = std::function<void(const DownloadResponse&)>;

    explicit AuthHttpClient(const QString& baseUrl, QObject* parent = nullptr);
    ~AuthHttpClient() override;

    void Post(const QString& path,
              const QString& bearerToken,
              int            timeoutSec,
              Callback       callback);

    void PostJsonToFile(const QString& path,
                       const QString& bearerToken,
                       const QByteArray& jsonBody,
                       const QString& targetFilePath,
                       int timeoutSec,
                       DownloadCallback callback);

    void CancelAll();

private:
    QString                                _baseUrl;
    std::shared_ptr<std::atomic<quint64>>  _cancelEpoch;
};
