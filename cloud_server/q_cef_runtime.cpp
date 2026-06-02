#include "q_cef_runtime.h"

#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QColor>
#include <QFileInfo>
#include <QStandardPaths>

#include <QCefContext.h>

QJ_NAMESPACE_FIT_CLOUD_SERVER_BEGIN

QCefRuntime& QCefRuntime::Instance()
{
	static QCefRuntime s_instance;
	return s_instance;
}

void QCefRuntime::Initialize()
{
	if (_context) {
		return;
	}

	InitConfig();
	// QCefContext is a QObject; when constructed with app it should follow Qt object ownership.
	// Keep only a non-owning observer here to avoid double deletion at shutdown.
	int argc = 0;
	_context = new QCefContext(qobject_cast<QApplication*>(qApp), argc, nullptr, &_config);
}

void QCefRuntime::InitConfig()
{
	_config.setLogLevel(QCefConfig::LOGSEVERITY_DEFAULT);
	// Keep the bridge object so the web layer can subscribe to QCefEvent via CallBridge.addEventListener.
	_config.setBridgeObjectName(QStringLiteral("CallBridge"));
	_config.setBuiltinSchemeName(QStringLiteral("CefView"));
	_config.setRemoteDebuggingPort(0);
	_config.setWindowlessRenderingEnabled(true);
	_config.setStandaloneMessageLoopEnabled(true);
	_config.setSandboxDisabled(true);
	_config.setBackgroundColor(QColor(Qt::white));

	const QString appDir = QCoreApplication::applicationDirPath();
	const QString cefBundle = QDir(appDir).filePath(QStringLiteral("CefView"));
	const QString appLocalData = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
	const QString exeName = QFileInfo(QCoreApplication::applicationFilePath()).completeBaseName();
	const QString instanceCachePath = QDir(appLocalData).filePath(
		QStringLiteral("QJCAM/cef/%1-%2").arg(exeName, QString::number(QCoreApplication::applicationPid())));

	if (!instanceCachePath.isEmpty()) {
		QDir().mkpath(instanceCachePath);
	}

	_config.setResourceDirectoryPath(QDir(cefBundle).filePath(QStringLiteral("Resources")));
	_config.setLocalesDirectoryPath(QDir(cefBundle).filePath(QStringLiteral("locales")));
	_config.setCachePath(instanceCachePath);
}

QJ_NAMESPACE_FIT_CLOUD_SERVER_END
