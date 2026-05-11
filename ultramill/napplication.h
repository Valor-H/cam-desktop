#pragma once

#include "ultramill_global.h"
#include <QApplication>

QJ_NAMESPACE_ULTRACAM_ULTRAMILL_BEGIN

class NApplication : public QApplication
{
public:
    NApplication(int& argc, char** argv);
    ~NApplication() override;

    void Initialize();

private:
    static void InitCefRuntime();
};

QJ_NAMESPACE_ULTRACAM_ULTRAMILL_END