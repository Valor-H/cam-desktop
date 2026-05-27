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
    static QCefRuntime instance;
    return instance;
}

void QCefRuntime::Initialize()
{
    if (m_context) {
        return;
    }

    InitConfig();
    // QCefContext is a QObject; when constructed with app it should follow Qt object ownership.
    // Keep only a non-owning observer here to avoid double deletion at shutdown.
    int argc = 0;
    m_context = new QCefContext(qobject_cast<QApplication*>(qApp), argc, nullptr, &m_config);
}

void QCefRuntime::InitConfig()
{
    m_config.setLogLevel(QCefConfig::LOGSEVERITY_DEFAULT);
    // Keep the bridge object so the web layer can subscribe to QCefEvent via CallBridge.addEventListener.
    m_config.setBridgeObjectName(QStringLiteral("CallBridge"));
    m_config.setBuiltinSchemeName(QStringLiteral("CefView"));
    m_config.setRemoteDebuggingPort(0);
    m_config.setWindowlessRenderingEnabled(true);
    m_config.setStandaloneMessageLoopEnabled(true);
    m_config.setSandboxDisabled(true);
    m_config.setBackgroundColor(QColor(Qt::white));

    const QString appDir = QCoreApplication::applicationDirPath();
    const QString cefBundle = QDir(appDir).filePath(QStringLiteral("CefView"));
    const QString appLocalData = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    const QString exeName = QFileInfo(QCoreApplication::applicationFilePath()).completeBaseName();
    const QString instanceCachePath = QDir(appLocalData).filePath(
        QStringLiteral("QJCAM/cef/%1-%2").arg(exeName, QString::number(QCoreApplication::applicationPid())));

    if (!instanceCachePath.isEmpty()) {
        QDir().mkpath(instanceCachePath);
    }

    m_config.setResourceDirectoryPath(QDir(cefBundle).filePath(QStringLiteral("Resources")));
    m_config.setLocalesDirectoryPath(QDir(cefBundle).filePath(QStringLiteral("locales")));
    m_config.setCachePath(instanceCachePath);
}

QJ_NAMESPACE_FIT_CLOUD_SERVER_END
