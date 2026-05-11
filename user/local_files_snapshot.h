#pragma once

#include <QJsonArray>
#include <QString>

namespace LocalFilesSnapshot
{
struct Result {
    bool ok { false };
    QString rootPath;
    QJsonArray files;
    QString errorCode;
    QString errorMessage;
};

Result ScanRoot();
}