#pragma once

#include "cloud_server_global.h"
#include <QCefConfig.h>
#include <QPointer>

class QCefContext;

QJ_NAMESPACE_FIT_CLOUD_SERVER_BEGIN

class CLOUD_SERVER_EXPORT QCefRuntime
{
public:
	/** 返回 CEF 运行时单例。 */
	static QCefRuntime& Instance();
	/** 初始化 CEF 运行时环境。 */
	void Initialize();

private:
	/** 构造 CEF 运行时对象。 */
	QCefRuntime() = default;
	/** 析构 CEF 运行时对象。 */
	~QCefRuntime() = default;
	/** 禁止拷贝构造。 */
	QCefRuntime(const QCefRuntime&) = delete;
	/** 禁止拷贝赋值。 */
	QCefRuntime& operator=(const QCefRuntime&) = delete;
	/** 初始化 CEF 配置参数。 */
	void InitConfig();
	/** 保存 CEF 配置对象。 */
	QCefConfig m_config;
	/** 保存 CEF 上下文对象。 */
	QPointer<QCefContext> m_context;
};

QJ_NAMESPACE_FIT_CLOUD_SERVER_END
