#pragma once

#include "ultramill_global.h"
#include <QFrame>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QWidget>

QJ_NAMESPACE_ULTRACAM_ULTRAMILL_BEGIN

class MockMainWorkspace final : public QWidget
{
    Q_OBJECT

public:
    explicit MockMainWorkspace(QWidget* parent = nullptr)
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
};

QJ_NAMESPACE_ULTRACAM_ULTRAMILL_END
