#pragma once

#include "user_global.h"
#include <QCefConfig.h>
#include <QPointer>

class QCefContext;

QJ_NAMESPACE_FIT_USER_BEGIN

class USER_EXPORT QCefRuntime
{
public:
    static QCefRuntime& Instance();

    void Initialize();

private:
    QCefRuntime() = default;
    ~QCefRuntime() = default;

    QCefRuntime(const QCefRuntime&) = delete;
    QCefRuntime& operator=(const QCefRuntime&) = delete;

    void InitConfig();

    QCefConfig m_config;
    QPointer<QCefContext> m_context;
};

QJ_NAMESPACE_FIT_USER_END
