#include "local_files_snapshot.h"

#include <project_management/cam_options.h>

#include <QDir>
#include <QFileInfo>
#include <QJsonObject>

#include <QStringList>

using namespace qianjizn::project;

namespace LocalFilesSnapshot
{
namespace
{
constexpr int kQjpFileType = 11;

QString toNativePath(const QString& path)
{
    return QDir::toNativeSeparators(QDir(path).absolutePath());
}

QString normalizePath(const QString& path)
{
    return QDir::fromNativeSeparators(path.trimmed());
}

QString toNativeFilePath(const QFileInfo& info)
{
    return QDir::toNativeSeparators(info.absoluteFilePath());
}

QStringList recentFilePaths()
{
    if (!CAMOptsPtr) {
        return {};
    }

    const QString raw = QString::fromStdString(CAMOptsPtr->GetRecentFileList()).trimmed();
    if (raw.isEmpty()) {
        return {};
    }

    QStringList files = raw.split(';', QString::SkipEmptyParts);
    for (QString& file : files) {
        file = normalizePath(file);
    }
    files.removeAll(QString());
    return files;
}

QJsonObject toFileInfoObject(const QFileInfo& info)
{
    return QJsonObject {
        { QStringLiteral("children"), QJsonArray {} },
        { QStringLiteral("fileName"), info.fileName() },
        { QStringLiteral("fileType"), kQjpFileType },
        { QStringLiteral("fileUuid"), QStringLiteral("local::") + toNativeFilePath(info) },
        { QStringLiteral("lastModified"), info.lastModified().toString(Qt::ISODate) },
        { QStringLiteral("link"), QJsonValue::Null },
        { QStringLiteral("owner"), QJsonValue::Null },
        { QStringLiteral("startTime"), QJsonValue::Null },
        { QStringLiteral("endTime"), QJsonValue::Null },
        { QStringLiteral("path"), toNativeFilePath(info) },
        { QStringLiteral("permission"), QJsonValue::Null },
        { QStringLiteral("sharePermissionList"), QJsonValue::Null },
        { QStringLiteral("size"), static_cast<qint64>(info.size()) },
        { QStringLiteral("thumbnailImage"), QString() },
    };
}
}

Result ScanRoot()
{
    Result result;

    result.rootPath.clear();

    const QStringList recentFiles = recentFilePaths();
    if (recentFiles.isEmpty()) {
        result.ok = true;
        return result;
    }

    QJsonArray files;
    for (const QString& path : recentFiles) {
        const QFileInfo entry(path);
        if (!entry.exists() || !entry.isReadable() || !entry.isFile()
            || entry.suffix().compare(QStringLiteral("qjp"), Qt::CaseInsensitive) != 0) {
            continue;
        }
        files.append(toFileInfoObject(entry));
    }

    result.ok = true;
    result.files = files;
    return result;
}
}
