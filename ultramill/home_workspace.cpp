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

    auto* rootLayout = new QHBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    auto* leftSidebar = new QFrame(this);
    leftSidebar->setObjectName(QStringLiteral("mockSidebar"));
    leftSidebar->setMinimumWidth(280);
    leftSidebar->setMaximumWidth(320);
    leftSidebar->setStyleSheet(QStringLiteral(
        "#mockSidebar {"
        "background: #f5f6f8;"
        "border-right: 1px solid #d7dce3;"
        "}"));

    auto* leftLayout = new QVBoxLayout(leftSidebar);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(0);

    auto* leftHeader = new QFrame(leftSidebar);
    leftHeader->setFixedHeight(42);
    leftHeader->setStyleSheet(QStringLiteral("background: #ffffff; border-bottom: 1px solid #d7dce3;"));

    auto* leftHeaderLayout = new QHBoxLayout(leftHeader);
    leftHeaderLayout->setContentsMargins(12, 6, 12, 6);
    leftHeaderLayout->setSpacing(8);

    auto* newDemoButton = new QPushButton(tr("New Demo"), leftHeader);
    newDemoButton->setCursor(Qt::PointingHandCursor);
    newDemoButton->setStyleSheet(QStringLiteral(
        "QPushButton {"
        "padding: 6px 12px;"
        "background: #ffffff;"
        "color: #1558b0;"
        "border: 1px solid #b9c7d8;"
        "border-radius: 4px;"
        "font-weight: 600;"
        "}"
        "QPushButton:hover { background: #eef4fb; }"));
    leftHeaderLayout->addWidget(newDemoButton, 0, Qt::AlignLeft | Qt::AlignVCenter);
    connect(newDemoButton, &QPushButton::clicked, this, &HomeWorkspace::NewDemoRequested);

    auto* toolLibButton = new QPushButton(tr("Tool Library"), leftHeader);
    toolLibButton->setCursor(Qt::PointingHandCursor);
    toolLibButton->setStyleSheet(QStringLiteral(
        "QPushButton {"
        "padding: 6px 12px;"
        "background: #1f6feb;"
        "color: #ffffff;"
        "border: none;"
        "border-radius: 4px;"
        "font-weight: 600;"
        "}"
        "QPushButton:hover { background: #1558b0; }"));
    leftHeaderLayout->addWidget(toolLibButton, 0, Qt::AlignLeft | Qt::AlignVCenter);
    leftHeaderLayout->addStretch(1);
    connect(toolLibButton, &QPushButton::clicked, this, &HomeWorkspace::ToolLibRequested);

    leftLayout->addWidget(leftHeader);

    auto* leftBody = new QFrame(leftSidebar);
    leftBody->setStyleSheet(QStringLiteral("background: #fafbfc;"));
    leftLayout->addWidget(leftBody, 1);

    auto* rightViewport = new QFrame(this);
    rightViewport->setObjectName(QStringLiteral("mockViewport"));
    rightViewport->setStyleSheet(QStringLiteral(
        "#mockViewport {"
        "background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #c6d0da, stop:1 #eef2f6);"
        "}"));

    rootLayout->addWidget(leftSidebar);
    rootLayout->addWidget(rightViewport, 1);
}

HomeWorkspace::~HomeWorkspace() = default;

QJ_NAMESPACE_ULTRACAM_ULTRAMILL_END