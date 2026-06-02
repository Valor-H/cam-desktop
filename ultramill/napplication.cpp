#include "napplication.h"
#include <cloud_server/q_cef_runtime.h>

#include <QCoreApplication>
#include <QDir>
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
{
}

NApplication::~NApplication()
{
	delete _cloudServerTranslator;
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

	const QString qm_path = QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("cloud_server_zh_CN.qm"));
	if (!QFileInfo::exists(qm_path)) {
		return;
	}

	QTranslator* translator = new QTranslator(this);
	if (!translator->load(qm_path)) {
		delete translator;
		return;
	}

	installTranslator(translator);
	_cloudServerTranslator = translator;
}

void NApplication::InitCefRuntime()
{
	qianjizn::cloudserver::QCefRuntime::Instance().Initialize();
}

QJ_NAMESPACE_ULTRACAM_ULTRAMILL_END
