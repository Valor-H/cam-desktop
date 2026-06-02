#pragma once

#include <QJsonArray>
#include <QString>

namespace LocalFilesSnapshot
{
	/** 封装本地文件扫描结果。 */
	struct Result {
		/** 标识扫描是否成功。 */
		bool ok{ false };
		/** 保存扫描根目录路径。 */
		QString rootPath;
		/** 保存扫描到的文件列表。 */
		QJsonArray files;
		/** 保存错误代码。 */
		QString errorCode;
		/** 保存错误描述。 */
		QString errorMessage;
	};

	/** 扫描本地根目录并返回快照结果。 */
	Result ScanRoot();
}
