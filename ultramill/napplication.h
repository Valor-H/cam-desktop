#pragma once

#include "ultramill_global.h"
#include <QApplication>

class QTranslator;

QJ_NAMESPACE_ULTRACAM_ULTRAMILL_BEGIN

class NApplication : public QApplication
{
public:
	NApplication(int& argc, char** argv);
	~NApplication() override;

	void Initialize();

private:
	void LoadTranslations();
#ifdef ENABLE_CLOUD_SERVER_MODULE
	static void InitializeCefRuntime();
#endif

	QTranslator* _cloudServerTranslator;
	QTranslator* _ultramillTranslator;
};

QJ_NAMESPACE_ULTRACAM_ULTRAMILL_END
