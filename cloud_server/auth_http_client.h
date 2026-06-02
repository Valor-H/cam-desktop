#pragma once

#include "cloud_server_global.h"

#include <QByteArray>
#include <QObject>
#include <QString>
#include <QVariantMap>

#include <atomic>
#include <functional>
#include <memory>

class CLOUD_SERVER_EXPORT AuthHttpClient : public QObject
{
public:
	/** 封装通用请求结果。 */
	struct Response
	{
		bool        networkOk{ false };    /** 标识网络请求是否成功完成。 */
		int         httpStatus{ 0 };       /** 记录 HTTP 响应状态码。 */
		int         bizCode{ -1 };         /** 记录业务层返回码。 */
		QString     bizMsg;                 /** 记录业务层返回消息。 */
		QVariantMap data;                   /** 保存业务层返回数据。 */
	};
	/** 封装文件下载请求结果。 */
	struct DownloadResponse
	{
		bool    networkOk{ false };    /** 标识网络请求是否成功完成。 */
		int     httpStatus{ 0 };       /** 记录 HTTP 响应状态码。 */
		bool    writeOk{ false };      /** 标识文件写入是否成功。 */
		QString errorMessage;           /** 记录下载或写入失败原因。 */
		QString targetFilePath;         /** 保存目标文件路径。 */
	};
	/** 定义通用请求完成回调类型。 */
	using Callback = std::function<void(const Response&)>;
	/** 定义文件下载完成回调类型。 */
	using DownloadCallback = std::function<void(const DownloadResponse&)>;
	/** 构造认证 HTTP 客户端。 */
	explicit AuthHttpClient(const QString& baseUrl, QObject* parent = nullptr);
	/** 析构认证 HTTP 客户端。 */
	~AuthHttpClient() override;
	/** 发送不带请求体的 POST 请求。 */
	void Post(const QString& path,
		const QString& bearerToken,
		int            timeoutSec,
		Callback       callback);
	/** 发送 JSON POST 请求并将响应写入文件。 */
	void PostJsonToFile(const QString& path,
		const QString& bearerToken,
		const QByteArray& jsonBody,
		const QString& targetFilePath,
		int timeoutSec,
		DownloadCallback callback);
	/** 发送 multipart/form-data POST 请求上传文件。 */
	void PostMultipartFile(const QString& path,
		const QString& bearerToken,
		const QString& fileFieldName,
		const QString& filePath,
		const QVariantMap& formFields,
		int timeoutSec,
		Callback callback);
	/** 取消当前全部未完成请求。 */
	void CancelAll();

private:
	/** 保存服务端基础地址。 */
	QString                                _baseUrl;
	/** 维护 Qt 网络请求管理器。 */
	class QNetworkAccessManager* _networkManager{ nullptr };
	/** 维护请求取消代次计数。 */
	std::shared_ptr<std::atomic<quint64>>  _cancelEpoch;
};
