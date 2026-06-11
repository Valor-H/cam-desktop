#pragma once

#include "cloud_server_global.h"

#include <QByteArray>
#include <QString>
#include <QStringList>

QJ_NAMESPACE_FIT_CLOUD_SERVER_BEGIN

enum class OpenRequestSource
{
	Local,
	Cloud,
};

class OpenRequestSourcePayload
{
	public:
	static QString Build(
		OpenRequestSource source,
		bool from_recent,
		const QString& file_uuid)
	{
		QStringList payload_parts;
		payload_parts.push_back(source == OpenRequestSource::Cloud ? QStringLiteral("c") : QStringLiteral("l"));
		payload_parts.push_back(from_recent ? QStringLiteral("1") : QStringLiteral("0"));
		payload_parts.push_back(QString::fromLatin1(file_uuid.trimmed().toUtf8().toBase64()));
		return payload_parts.join(QLatin1Char('|'));
	}

	static bool TryParse(
		const QString& payload,
		OpenRequestSource* source,
		bool* from_recent,
		QString* file_uuid)
	{
		if (!source || !from_recent || !file_uuid) {
			return false;
		}

		const QStringList payload_parts = payload.trimmed().split(QLatin1Char('|'), Qt::KeepEmptyParts);
		if (payload_parts.size() >= 3) {
			*source = payload_parts.at(0) == QStringLiteral("c")
				? OpenRequestSource::Cloud
				: OpenRequestSource::Local;
			*from_recent = payload_parts.at(1) == QStringLiteral("1");
			*file_uuid = QString::fromUtf8(QByteArray::fromBase64(payload_parts.at(2).toLatin1())).trimmed();
			return true;
		}

		return false;
	}
};

class OpenRequestContext
{
	public:
	QString filePath;
	QString fileUuid;
	OpenRequestSource source = OpenRequestSource::Local;
	bool fromRecent = false;

	QStringList ToLaunchArguments() const
	{
		QStringList arguments;
		const QString normalized_path = filePath.trimmed();
		if (normalized_path.isEmpty()) {
			return arguments;
		}

		arguments.push_back(QStringLiteral("-o"));
		arguments.push_back(normalized_path);
		arguments.push_back(QStringLiteral("-or"));
		arguments.push_back(OpenRequestSourcePayload::Build(source, fromRecent, fileUuid));
		return arguments;
	}

	static bool TryParseLaunchArguments(const QStringList& arguments, OpenRequestContext* context)
	{
		if (!context) {
			return false;
		}

		OpenRequestContext parsed_context;
		bool has_open_path = false;
		for (int index = 0; index < arguments.size(); ++index) {
			const QString& argument = arguments.at(index);
			if (argument == QStringLiteral("-or")) {
				if (index + 1 >= arguments.size()) {
					continue;
				}

				const QString payload = arguments.at(index + 1).trimmed();
				if (!OpenRequestSourcePayload::TryParse(
					payload,
					&parsed_context.source,
					&parsed_context.fromRecent,
					&parsed_context.fileUuid)) {
					const QStringList payload_parts = payload.split(QLatin1Char('|'), Qt::KeepEmptyParts);
					if (payload_parts.size() >= 4) {
						parsed_context.source = payload_parts.at(0) == QStringLiteral("c")
							? OpenRequestSource::Cloud
							: OpenRequestSource::Local;
						parsed_context.fromRecent = payload_parts.at(1) == QStringLiteral("1");
						parsed_context.filePath = QString::fromUtf8(QByteArray::fromBase64(payload_parts.at(2).toLatin1())).trimmed();
						parsed_context.fileUuid = QString::fromUtf8(QByteArray::fromBase64(payload_parts.at(3).toLatin1())).trimmed();
						has_open_path = !parsed_context.filePath.isEmpty();
					}
				}
				++index;
				continue;
			}

			if (argument == QStringLiteral("-o")) {
				if (index + 1 >= arguments.size()) {
					continue;
				}

				parsed_context.filePath = arguments.at(index + 1).trimmed();
				has_open_path = !parsed_context.filePath.isEmpty();
				++index;
				continue;
			}

			if (argument.startsWith(QStringLiteral("--open-source="))) {
				const QString source_value = argument.mid(QStringLiteral("--open-source=").size()).trimmed().toLower();
				parsed_context.source = source_value == QStringLiteral("cloud")
					? OpenRequestSource::Cloud
					: OpenRequestSource::Local;
				continue;
			}

			if (argument.startsWith(QStringLiteral("--open-path="))) {
				parsed_context.filePath = argument.mid(QStringLiteral("--open-path=").size()).trimmed();
				has_open_path = !parsed_context.filePath.isEmpty();
				continue;
			}

			if (argument.startsWith(QStringLiteral("--open-file-uuid="))) {
				parsed_context.fileUuid = argument.mid(QStringLiteral("--open-file-uuid=").size()).trimmed();
				continue;
			}

			if (argument.startsWith(QStringLiteral("--open-from-recent="))) {
				const QString recent_value = argument.mid(QStringLiteral("--open-from-recent=").size()).trimmed();
				parsed_context.fromRecent = recent_value == QStringLiteral("1")
					|| recent_value.compare(QStringLiteral("true"), Qt::CaseInsensitive) == 0;
			}
		}

		if (!has_open_path) {
			return false;
		}

		*context = parsed_context;
		return true;
	}

	bool IsCloud() const
	{
		return source == OpenRequestSource::Cloud;
	}

	bool ShouldAddToLocalRecent() const
	{
		return source == OpenRequestSource::Local;
	}
};

QJ_NAMESPACE_FIT_CLOUD_SERVER_END
