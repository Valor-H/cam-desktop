#pragma once

#include "ultramill_global.h"

#include <QWidget>

class QPlainTextEdit;

QJ_NAMESPACE_ULTRACAM_ULTRAMILL_BEGIN

class HomeWorkspace : public QWidget
{
    Q_OBJECT

public:
    explicit HomeWorkspace(QWidget* parent = nullptr);
    ~HomeWorkspace() override;
    void SetViewportText(const QString& text);
    void SetViewportFilePath(const QString& file_path);
    void ClearViewport();

signals:
    void NewProjectRequested();
    void ToolLibRequested();

private:
    QPlainTextEdit* _viewportEditor;
};

QJ_NAMESPACE_ULTRACAM_ULTRAMILL_END
