#include "tool_lib_dialog.h"

#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

QJ_NAMESPACE_ULTRACAM_ULTRAMILL_BEGIN

ToolLibDialog::ToolLibDialog(QWidget* parent)
	: QDialog(parent)
{
	setWindowTitle(tr("Tool Library"));
	resize(460, 280);

	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->setContentsMargins(20, 20, 20, 20);
	layout->setSpacing(10);

	QLabel* title_label = new QLabel(tr("Tool Library Window"), this);
	title_label->setStyleSheet(QStringLiteral("font-size: 20px; font-weight: 700; color: #1f2328;"));
	layout->addWidget(title_label);

	layout->addWidget(new QLabel(tr("1. This is a mock tool library dialog."), this));
	layout->addWidget(new QLabel(tr("2. Tool categories or tool items can be added here later."), this));
	layout->addWidget(new QLabel(tr("3. Current content is intentionally minimal."), this));
	layout->addStretch(1);

	QPushButton* close_button = new QPushButton(tr("Close"), this);
	close_button->setFixedWidth(96);
	connect(close_button, &QPushButton::clicked, this, &QDialog::accept);
	layout->addWidget(close_button, 0, Qt::AlignRight);
}

ToolLibDialog::~ToolLibDialog() = default;

QJ_NAMESPACE_ULTRACAM_ULTRAMILL_END
