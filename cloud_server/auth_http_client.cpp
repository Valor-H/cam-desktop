#include "auth_http_client.h"

#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMetaObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QHttpMultiPart>
#include <QHttpPart>
#include <QTimer>
#include <QUrl>

#include <hv/requests.h>

AuthHttpClient::AuthHttpClient(const QString& baseUrl, QObject* parent)
	: QObject(parent)
	, _baseUrl(baseUrl)
	, _networkManager(new QNetworkAccessManager(this))
	, _cancelEpoch(std::make_shared<std::atomic<quint64>>(0))
{
}

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
	const quint64 myEpoch = _cancelEpoch->load(std::memory_order_relaxed);
	auto          epochRef = _cancelEpoch;

	auto req = std::make_shared<HttpRequest>();
	req->method = HTTP_POST;
	req->url = (_baseUrl + path).toStdString();
	req->timeout = timeoutSec;
	req->headers["Authorization"] = "Bearer " + bearerToken.toStdString();
	req->headers["Content-Type"] = "application/json";
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

					result.networkOk = true;
					result.httpStatus = resp->status_code;

					const QByteArray body = QByteArray::fromStdString(resp->body);
					const QJsonDocument doc = QJsonDocument::fromJson(body);
					if (doc.isObject()) {
						const QJsonObject root = doc.object();
						result.bizCode = root.value(QStringLiteral("code")).toInt(-1);
						result.bizMsg = root.value(QStringLiteral("msg")).toString();
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

void AuthHttpClient::PostMultipartFile(
	const QString& path,
	const QString& bearerToken,
	const QString& fileFieldName,
	const QString& filePath,
	const QVariantMap& formFields,
	int timeoutSec,
	Callback callback)
{
	const quint64 myEpoch = _cancelEpoch->load(std::memory_order_relaxed);
	auto epochRef = _cancelEpoch;

	QFileInfo fileInfo(filePath);
	if (!fileInfo.exists() || !fileInfo.isFile()) {
		QMetaObject::invokeMethod(
			QCoreApplication::instance(),
			[cb = std::move(callback)]() {
				Response result;
				result.bizMsg = QStringLiteral("upload source file is missing");
				cb(result);
			},
			Qt::QueuedConnection);
		return;
	}

	auto* multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
	for (auto it = formFields.constBegin(); it != formFields.constEnd(); ++it) {
		QHttpPart textPart;
		textPart.setHeader(
			QNetworkRequest::ContentDispositionHeader,
			QVariant(QStringLiteral("form-data; name=\"%1\"").arg(it.key())));
		textPart.setBody(it.value().toString().toUtf8());
		multiPart->append(textPart);
	}

	auto* file = new QFile(filePath, multiPart);
	if (!file->open(QIODevice::ReadOnly)) {
		multiPart->deleteLater();
		QMetaObject::invokeMethod(
			QCoreApplication::instance(),
			[cb = std::move(callback)]() {
				Response result;
				result.bizMsg = QStringLiteral("failed to open upload source file");
				cb(result);
			},
			Qt::QueuedConnection);
		return;
	}

	QHttpPart filePart;
	filePart.setHeader(
		QNetworkRequest::ContentDispositionHeader,
		QVariant(QStringLiteral("form-data; name=\"%1\"; filename=\"%2\"")
			.arg(fileFieldName, fileInfo.fileName())));
	filePart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant(QStringLiteral("application/octet-stream")));
	filePart.setBodyDevice(file);
	multiPart->append(filePart);

	QNetworkRequest request(QUrl(_baseUrl + path));
	request.setRawHeader("Authorization", QStringLiteral("Bearer %1").arg(bearerToken).toUtf8());
	request.setRawHeader("X-Client-Type", QByteArrayLiteral("desktop"));

	QNetworkReply* reply = _networkManager->post(request, multiPart);
	multiPart->setParent(reply);

	auto* timeoutTimer = new QTimer(reply);
	timeoutTimer->setSingleShot(true);
	connect(timeoutTimer, &QTimer::timeout, reply, [reply]() {
		if (reply->isRunning()) {
			reply->abort();
		}
		});
	timeoutTimer->start(timeoutSec * 1000);

	connect(reply, &QNetworkReply::finished, this, [epochRef, myEpoch, reply, cb = std::move(callback)]() mutable {
		const QByteArray body = reply->readAll();
		const QVariant statusVariant = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);

		Response result;
		result.httpStatus = statusVariant.isValid() ? statusVariant.toInt() : 0;
		result.networkOk = reply->error() == QNetworkReply::NoError;
		if (!result.networkOk) {
			result.bizMsg = reply->errorString();
		}

		const QJsonDocument doc = QJsonDocument::fromJson(body);
		if (doc.isObject()) {
			const QJsonObject root = doc.object();
			result.bizCode = root.value(QStringLiteral("code")).toInt(-1);
			result.bizMsg = root.value(QStringLiteral("msg")).toString(result.bizMsg);
			const QJsonObject dataObj = root.value(QStringLiteral("data")).toObject();
			if (!dataObj.isEmpty()) {
				result.data = dataObj.toVariantMap();
			}
		}

		reply->deleteLater();
		if (epochRef->load(std::memory_order_relaxed) != myEpoch) {
			return;
		}

		cb(result);
		});
}

void AuthHttpClient::CancelAll()
{
	_cancelEpoch->fetch_add(1, std::memory_order_relaxed);
}
