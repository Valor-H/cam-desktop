#include "home_workspace.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QPushButton>
#include <QVBoxLayout>

QJ_NAMESPACE_ULTRACAM_ULTRAMILL_BEGIN

HomeWorkspace::HomeWorkspace(QWidget* parent)
	: QWidget(parent)
{
	setObjectName(QStringLiteral("mockMainWorkspace"));

	QHBoxLayout* root_layout = new QHBoxLayout(this);
	root_layout->setContentsMargins(0, 0, 0, 0);
	root_layout->setSpacing(0);

	QFrame* left_sidebar = new QFrame(this);
	left_sidebar->setObjectName(QStringLiteral("mockSidebar"));
	left_sidebar->setMinimumWidth(280);
	left_sidebar->setMaximumWidth(320);
	left_sidebar->setStyleSheet(QStringLiteral(
		"#mockSidebar {"
		"background: #f5f6f8;"
		"border-right: 1px solid #d7dce3;"
		"}"));

	QVBoxLayout* left_layout = new QVBoxLayout(left_sidebar);
	left_layout->setContentsMargins(0, 0, 0, 0);
	left_layout->setSpacing(0);

	QFrame* left_header = new QFrame(left_sidebar);
	left_header->setFixedHeight(42);
	left_header->setStyleSheet(QStringLiteral("background: #ffffff; border-bottom: 1px solid #d7dce3;"));

	QHBoxLayout* left_header_layout = new QHBoxLayout(left_header);
	left_header_layout->setContentsMargins(12, 6, 12, 6);
	left_header_layout->setSpacing(8);

	QPushButton* new_project_button = new QPushButton(tr("New Project"), left_header);
	new_project_button->setCursor(Qt::PointingHandCursor);
	new_project_button->setStyleSheet(QStringLiteral(
		"QPushButton {"
		"padding: 6px 12px;"
		"background: #ffffff;"
		"color: #1558b0;"
		"border: 1px solid #b9c7d8;"
		"border-radius: 4px;"
		"font-weight: 600;"
		"}"
		"QPushButton:hover { background: #eef4fb; }"));
	left_header_layout->addWidget(new_project_button, 0, Qt::AlignLeft | Qt::AlignVCenter);
	connect(new_project_button, &QPushButton::clicked, this, &HomeWorkspace::NewProjectRequested);

	QPushButton* tool_lib_button = new QPushButton(tr("Tool Library"), left_header);
	tool_lib_button->setCursor(Qt::PointingHandCursor);
	tool_lib_button->setStyleSheet(QStringLiteral(
		"QPushButton {"
		"padding: 6px 12px;"
		"background: #1f6feb;"
		"color: #ffffff;"
		"border: none;"
		"border-radius: 4px;"
		"font-weight: 600;"
		"}"
		"QPushButton:hover { background: #1558b0; }"));
	left_header_layout->addWidget(tool_lib_button, 0, Qt::AlignLeft | Qt::AlignVCenter);
	left_header_layout->addStretch(1);
	connect(tool_lib_button, &QPushButton::clicked, this, &HomeWorkspace::ToolLibRequested);

	left_layout->addWidget(left_header);

	QFrame* left_body = new QFrame(left_sidebar);
	left_body->setStyleSheet(QStringLiteral("background: #fafbfc;"));
	left_layout->addWidget(left_body, 1);

	QFrame* right_viewport = new QFrame(this);
	right_viewport->setObjectName(QStringLiteral("mockViewport"));
	right_viewport->setStyleSheet(QStringLiteral(
		"#mockViewport {"
		"background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #c6d0da, stop:1 #eef2f6);"
		"}"));

	root_layout->addWidget(left_sidebar);
	root_layout->addWidget(right_viewport, 1);
}

HomeWorkspace::~HomeWorkspace() = default;

QJ_NAMESPACE_ULTRACAM_ULTRAMILL_END
