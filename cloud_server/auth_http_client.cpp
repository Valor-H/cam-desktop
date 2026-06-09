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

AuthHttpClient::AuthHttpClient(const QString& base_url, QObject* parent)
	: QObject(parent)
	, _baseUrl(base_url)
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
	const QString& bearer_token,
	int            timeout_sec,
	Callback       callback)
{
	const quint64 my_epoch = _cancelEpoch->load(std::memory_order_relaxed);
	std::shared_ptr<std::atomic<quint64>> epoch_ref = _cancelEpoch;

	std::shared_ptr<HttpRequest> req = std::make_shared<HttpRequest>();
	req->method = HTTP_POST;
	req->url = (_baseUrl + path).toStdString();
	req->timeout = timeout_sec;
	req->headers["Authorization"] = "Bearer " + bearer_token.toStdString();
	req->headers["Content-Type"] = "application/json";
	req->headers["X-Client-Type"] = "desktop";
	req->body = "{}";

	requests::async(req,
		[epoch_ref, my_epoch, cb = std::move(callback)](const HttpResponsePtr& resp) {
			QMetaObject::invokeMethod(
				QCoreApplication::instance(),
				[epoch_ref, my_epoch, resp, cb]() {
					if (epoch_ref->load(std::memory_order_relaxed) != my_epoch) {
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
					const QJsonDocument document = QJsonDocument::fromJson(body);
					if (document.isObject()) {
						const QJsonObject root = document.object();
						result.bizCode = root.value(QStringLiteral("code")).toInt(-1);
						result.bizMsg = root.value(QStringLiteral("msg")).toString();
						const QJsonValue data_value = root.value(QStringLiteral("data"));
						result.dataValue = data_value.toVariant();
						if (data_value.isObject()) {
							const QJsonObject data_obj = data_value.toObject();
							if (!data_obj.isEmpty()) {
								result.data = data_obj.toVariantMap();
							}
						}
					}

					cb(result);
				},
				Qt::QueuedConnection);
		});
}

void AuthHttpClient::PostJsonToFile(
	const QString& path,
	const QString& bearer_token,
	const QByteArray& json_body,
	const QString& target_file_path,
	int timeout_sec,
	DownloadCallback callback)
{
	const quint64 my_epoch = _cancelEpoch->load(std::memory_order_relaxed);
	std::shared_ptr<std::atomic<quint64>> epoch_ref = _cancelEpoch;

	std::shared_ptr<HttpRequest> req = std::make_shared<HttpRequest>();
	req->method = HTTP_POST;
	req->url = (_baseUrl + path).toStdString();
	req->timeout = timeout_sec;
	req->headers["Authorization"] = "Bearer " + bearer_token.toStdString();
	req->headers["Content-Type"] = "application/json";
	req->headers["X-Client-Type"] = "desktop";
	req->body = json_body.toStdString();

	requests::async(req,
		[epoch_ref, my_epoch, cb = std::move(callback), target_file_path](const HttpResponsePtr& resp) {
			QMetaObject::invokeMethod(
				QCoreApplication::instance(),
				[epoch_ref, my_epoch, resp, cb, target_file_path]() {
					if (epoch_ref->load(std::memory_order_relaxed) != my_epoch) {
						return;
					}

					DownloadResponse result;
					result.targetFilePath = target_file_path;

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

					QFile target(target_file_path);
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
	const QString& bearer_token,
	const QString& file_field_name,
	const QString& file_path,
	const QVariantMap& form_fields,
	int timeout_sec,
	Callback callback)
{
	const quint64 my_epoch = _cancelEpoch->load(std::memory_order_relaxed);
	std::shared_ptr<std::atomic<quint64>> epoch_ref = _cancelEpoch;

	QFileInfo file_info(file_path);
	if (!file_info.exists() || !file_info.isFile()) {
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

	QHttpMultiPart* multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
	for (auto it = form_fields.constBegin(); it != form_fields.constEnd(); ++it) {
		QHttpPart textPart;
		textPart.setHeader(
			QNetworkRequest::ContentDispositionHeader,
			QVariant(QStringLiteral("form-data; name=\"%1\"").arg(it.key())));
		textPart.setBody(it.value().toString().toUtf8());
		multiPart->append(textPart);
	}

	QFile* file = new QFile(file_path, multiPart);
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
			.arg(file_field_name, file_info.fileName())));
	filePart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant(QStringLiteral("application/octet-stream")));
	filePart.setBodyDevice(file);
	multiPart->append(filePart);

	QNetworkRequest request(QUrl(_baseUrl + path));
	request.setRawHeader("Authorization", QStringLiteral("Bearer %1").arg(bearer_token).toUtf8());
	request.setRawHeader("X-Client-Type", QByteArrayLiteral("desktop"));

	QNetworkReply* reply = _networkManager->post(request, multiPart);
	multiPart->setParent(reply);

	QTimer* timeoutTimer = new QTimer(reply);
	timeoutTimer->setSingleShot(true);
	connect(timeoutTimer, &QTimer::timeout, reply, [reply]() {
		if (reply->isRunning()) {
			reply->abort();
		}
		});
	timeoutTimer->start(timeout_sec * 1000);

	connect(reply, &QNetworkReply::finished, this, [epoch_ref, my_epoch, reply, cb = std::move(callback)]() mutable {
		const QByteArray body = reply->readAll();
		const QVariant status_variant = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);

		Response result;
		result.httpStatus = status_variant.isValid() ? status_variant.toInt() : 0;
		result.networkOk = reply->error() == QNetworkReply::NoError;
		if (!result.networkOk) {
			result.bizMsg = reply->errorString();
		}

		const QJsonDocument document = QJsonDocument::fromJson(body);
		if (document.isObject()) {
			const QJsonObject root = document.object();
			result.bizCode = root.value(QStringLiteral("code")).toInt(-1);
			result.bizMsg = root.value(QStringLiteral("msg")).toString(result.bizMsg);
			const QJsonValue data_value = root.value(QStringLiteral("data"));
			result.dataValue = data_value.toVariant();
			if (data_value.isObject()) {
				const QJsonObject data_obj = data_value.toObject();
				if (!data_obj.isEmpty()) {
					result.data = data_obj.toVariantMap();
				}
			}
		}

		reply->deleteLater();
		if (epoch_ref->load(std::memory_order_relaxed) != my_epoch) {
			return;
		}

		cb(result);
		});
}

void AuthHttpClient::CancelAll()
{
	_cancelEpoch->fetch_add(1, std::memory_order_relaxed);
}
