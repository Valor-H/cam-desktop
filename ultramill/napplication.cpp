#include "napplication.h"
#include <cloud_server/q_cef_runtime.h>

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QTranslator>

// Q_INIT_RESOURCE cannot be called inside a namespace; use a file-scope helper.
static void InitUltramillResources()
{
	Q_INIT_RESOURCE(ultramill);
}

QJ_NAMESPACE_ULTRACAM_ULTRAMILL_BEGIN

NApplication::NApplication(int& argc, char** argv)
	: QApplication(argc, argv)
	, _cloudServerTranslator(nullptr)
	, _ultramillTranslator(nullptr)
{
}

NApplication::~NApplication()
{
	delete _cloudServerTranslator;
	delete _ultramillTranslator;
}

void NApplication::Initialize()
{
	InitUltramillResources();
	LoadTranslations();
	InitCefRuntime();
}

void NApplication::LoadTranslations()
{
	if (_cloudServerTranslator) {
		removeTranslator(_cloudServerTranslator);
		delete _cloudServerTranslator;
		_cloudServerTranslator = nullptr;
	}

	if (_ultramillTranslator) {
		removeTranslator(_ultramillTranslator);
		delete _ultramillTranslator;
		_ultramillTranslator = nullptr;
	}

	const QString app_dir = QCoreApplication::applicationDirPath();

	const QString cloud_server_qm = QDir(app_dir).filePath(QStringLiteral("cloud_server_zh_CN.qm"));
	if (QFileInfo::exists(cloud_server_qm)) {
		QTranslator* translator = new QTranslator(this);
		if (translator->load(cloud_server_qm)) {
			installTranslator(translator);
			_cloudServerTranslator = translator;
		}
		else {
			delete translator;
		}
	}

	const QString ultramill_qm = QDir(app_dir).filePath(QStringLiteral("ultramill_zh_CN.qm"));
	if (QFileInfo::exists(ultramill_qm)) {
		QTranslator* translator = new QTranslator(this);
		if (translator->load(ultramill_qm)) {
			installTranslator(translator);
			_ultramillTranslator = translator;
		}
		else {
			delete translator;
		}
	}
}

void NApplication::InitCefRuntime()
{
	qianjizn::cloudserver::QCefRuntime::Instance().Initialize();
}

QJ_NAMESPACE_ULTRACAM_ULTRAMILL_END
