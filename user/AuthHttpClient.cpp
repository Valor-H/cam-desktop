#include "AuthHttpClient.h"

#include <QCoreApplication>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMetaObject>

#include <hv/requests.h>

AuthHttpClient::AuthHttpClient(const QString& baseUrl, QObject* parent)
    : QObject(parent)
    , _baseUrl(baseUrl)
    , _cancelEpoch(std::make_shared<std::atomic<quint64>>(0))
{}

AuthHttpClient::~AuthHttpClient()
{
    CancelAll();
}

void AuthHttpClient::Post(
    const QString& path,
    const QString& bearerToken,
    int            timeoutSec,
    Callback       callback)
{
    const quint64 myEpoch  = _cancelEpoch->load(std::memory_order_relaxed);
    auto          epochRef = _cancelEpoch;

    auto req = std::make_shared<HttpRequest>();
    req->method  = HTTP_POST;
    req->url     = (_baseUrl + path).toStdString();
    req->timeout = timeoutSec;
    req->headers["Authorization"] = "Bearer " + bearerToken.toStdString();
    req->headers["Content-Type"]  = "application/json";
    req->headers["X-Client-Type"] = "desktop";
    req->body = "{}";

    requests::async(req,
        [epochRef, myEpoch, cb = std::move(callback)](const HttpResponsePtr& resp) {
            QMetaObject::invokeMethod(
                QCoreApplication::instance(),
                [epochRef, myEpoch, resp, cb]() {
                    if (epochRef->load(std::memory_order_relaxed) != myEpoch) {
                        return;
                    }

                    Response result;
                    if (!resp) {
                        result.networkOk = false;
                        cb(result);
                        return;
                    }

                    result.networkOk  = true;
                    result.httpStatus = resp->status_code;

                    const QByteArray body = QByteArray::fromStdString(resp->body);
                    const QJsonDocument doc = QJsonDocument::fromJson(body);
                    if (doc.isObject()) {
                        const QJsonObject root = doc.object();
                        result.bizCode = root.value(QStringLiteral("code")).toInt(-1);
                        result.bizMsg  = root.value(QStringLiteral("msg")).toString();
                        const QJsonObject dataObj =
                            root.value(QStringLiteral("data")).toObject();
                        if (!dataObj.isEmpty()) {
                            result.data = dataObj.toVariantMap();
                        }
                    }

                    cb(result);
                },
                Qt::QueuedConnection);
        });
}

void AuthHttpClient::PostJsonToFile(
    const QString& path,
    const QString& bearerToken,
    const QByteArray& jsonBody,
    const QString& targetFilePath,
    int timeoutSec,
    DownloadCallback callback)
{
    const quint64 myEpoch = _cancelEpoch->load(std::memory_order_relaxed);
    auto epochRef = _cancelEpoch;

    auto req = std::make_shared<HttpRequest>();
    req->method = HTTP_POST;
    req->url = (_baseUrl + path).toStdString();
    req->timeout = timeoutSec;
    req->headers["Authorization"] = "Bearer " + bearerToken.toStdString();
    req->headers["Content-Type"] = "application/json";
    req->headers["X-Client-Type"] = "desktop";
    req->body = jsonBody.toStdString();

    requests::async(req,
        [epochRef, myEpoch, cb = std::move(callback), targetFilePath](const HttpResponsePtr& resp) {
            QMetaObject::invokeMethod(
                QCoreApplication::instance(),
                [epochRef, myEpoch, resp, cb, targetFilePath]() {
                    if (epochRef->load(std::memory_order_relaxed) != myEpoch) {
                        return;
                    }

                    DownloadResponse result;
                    result.targetFilePath = targetFilePath;

                    if (!resp) {
                        result.errorMessage = QStringLiteral("network request failed");
                        cb(result);
                        return;
                    }

                    result.networkOk = true;
                    result.httpStatus = resp->status_code;

                    if (resp->status_code < 200 || resp->status_code >= 300) {
                        result.errorMessage = QStringLiteral("http status %1").arg(resp->status_code);
                        cb(result);
                        return;
                    }

                    QFile target(targetFilePath);
                    if (!target.open(QIODevice::WriteOnly)) {
                        result.errorMessage = QStringLiteral("failed to open target file for writing");
                        cb(result);
                        return;
                    }

                    const QByteArray body = QByteArray::fromStdString(resp->body);
                    const qint64 written = target.write(body);
                    target.close();

                    if (written != body.size()) {
                        target.remove();
                        result.errorMessage = QStringLiteral("failed to write response body to cache file");
                        cb(result);
                        return;
                    }

                    result.writeOk = true;
                    cb(result);
                },
                Qt::QueuedConnection);
        });
}

void AuthHttpClient::CancelAll()
{
    _cancelEpoch->fetch_add(1, std::memory_order_relaxed);
}
