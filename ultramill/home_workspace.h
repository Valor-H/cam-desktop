#pragma once

#include "ultramill_global.h"

#include <QWidget>

QJ_NAMESPACE_ULTRACAM_ULTRAMILL_BEGIN

class HomeWorkspace : public QWidget
{
    Q_OBJECT

public:
    explicit HomeWorkspace(QWidget* parent = nullptr);
    ~HomeWorkspace() override;

signals:
    void ToolLibRequested();
};

QJ_NAMESPACE_ULTRACAM_ULTRAMILL_END