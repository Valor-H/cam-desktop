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
		constexpr int s_qjpFileType = 11;

		QString ToNativePath(const QString& path)
		{
			return QDir::toNativeSeparators(QDir(path).absolutePath());
		}

		QString NormalizePath(const QString& path)
		{
			return QDir::fromNativeSeparators(path.trimmed());
		}

		QString ToNativeFilePath(const QFileInfo& info)
		{
			return QDir::toNativeSeparators(info.absoluteFilePath());
		}

		QStringList RecentFilePaths()
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
				file = NormalizePath(file);
			}
			files.removeAll(QString());
			return files;
		}

		QJsonObject ToFileInfoObject(const QFileInfo& info)
		{
			return QJsonObject{
				{ QStringLiteral("children"), QJsonArray {} },
				{ QStringLiteral("fileName"), info.fileName() },
				{ QStringLiteral("fileType"), s_qjpFileType },
				{ QStringLiteral("fileUuid"), QStringLiteral("local::") + ToNativeFilePath(info) },
				{ QStringLiteral("lastModified"), info.lastModified().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss")) },
				{ QStringLiteral("link"), QJsonValue::Null },
				{ QStringLiteral("localPath"), ToNativeFilePath(info) },
				{ QStringLiteral("owner"), QJsonValue::Null },
				{ QStringLiteral("startTime"), QJsonValue::Null },
				{ QStringLiteral("endTime"), QJsonValue::Null },
				{ QStringLiteral("permission"), QJsonValue::Null },
				{ QStringLiteral("sharePermissionList"), QJsonValue::Null },
				{ QStringLiteral("size"), static_cast<qint64>(info.size()) },
				{ QStringLiteral("source"), QStringLiteral("local") },
				{ QStringLiteral("thumbnailImage"), QString() },
			};
		}
	}

	Result ScanRoot()
	{
		Result result;

		result.rootPath.clear();

		const QStringList recent_files = RecentFilePaths();
		if (recent_files.isEmpty()) {
			result.ok = true;
			return result;
		}

		QJsonArray files;
		for (const QString& path : recent_files) {
			const QFileInfo entry(path);
			if (!entry.exists() || !entry.isReadable() || !entry.isFile()
				|| entry.suffix().compare(QStringLiteral("qjp"), Qt::CaseInsensitive) != 0) {
				continue;
			}
			files.append(ToFileInfoObject(entry));
		}

		result.ok = true;
		result.files = files;
		return result;
	}
}
