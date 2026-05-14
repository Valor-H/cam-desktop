#include "napplication.h"
#include <user/q_cef_runtime.h>

// Q_INIT_RESOURCE cannot be called inside a namespace; use a file-scope helper.
static void initUltramillResources()
{
    Q_INIT_RESOURCE(ultramill);
}

QJ_NAMESPACE_ULTRACAM_ULTRAMILL_BEGIN

NApplication::NApplication(int& argc, char** argv)
    : QApplication(argc, argv)
{}

NApplication::~NApplication() = default;

void NApplication::Initialize()
{
    initUltramillResources();
    InitCefRuntime();
}

void NApplication::InitCefRuntime()
{
    qianjizn::user::QCefRuntime::Instance().Initialize();
}

QJ_NAMESPACE_ULTRACAM_ULTRAMILL_END
