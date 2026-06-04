#pragma once

#include "ultramill_global.h"

#include <QDialog>

QJ_NAMESPACE_ULTRACAM_ULTRAMILL_BEGIN

class ToolLibDialog : public QDialog
{
public:
	explicit ToolLibDialog(QWidget* parent = nullptr);
	~ToolLibDialog() override;
};

QJ_NAMESPACE_ULTRACAM_ULTRAMILL_END
