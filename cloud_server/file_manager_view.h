#pragma once

#include "cloud_server_global.h"

#include <cloud_server/cloud_file_service.h>

#include <QUrl>
#include <QVariantMap>
#include <QWidget>

#include <QCefQuery.h>

class QString;
class QEvent;
class QShowEvent;
class QResizeEvent;
template <typename T>
class QFutureWatcher;

class QCefView;
class QWindow;

namespace LocalFilesSnapshot
{
	struct Result;
}

QJ_NAMESPACE_FIT_CLOUD_SERVER_BEGIN
class UserAuthService;
class CloudFileService;

class CLOUD_SERVER_EXPORT FileManagerView : public QWidget
{
	Q_OBJECT

public:
	/** 构造文件管理视图。 */
	explicit FileManagerView(QWidget* parent,
		UserAuthService* auth_service,
		CloudFileService* cloud_file_service,
		const QUrl& page_url);
	/** 析构文件管理视图。 */
	~FileManagerView() override;
	/** 刷新当前网页页面。 */
	void RefreshCurrentPage();
	/** 立即同步嵌入视图区域大小。 */
	void SyncViewportGeometryNow();

protected:
	/** 处理部件事件分发。 */
	bool event(QEvent* event) override;
	/** 处理显示事件。 */
	void showEvent(QShowEvent* event) override;
	/** 处理尺寸变化事件。 */
	void resizeEvent(QResizeEvent* event) override;

signals:
	/** 请求打开指定本地文件。 */
	void OpenFileRequested(const QString& file_path);
	/** 请求打开已缓存到本地的云端文件。 */
	void OpenCloudFileRequested(const QString& file_path, const QString& file_uuid);
	/** 请求执行打开操作。 */
	void OpenRequested();
	/** 请求新建项目。 */
	void NewProjectRequested();
	/** 请求返回桌面工作台。 */
	void ReturnToWorkspaceRequested();

private:
	/** 向网页注入桌面运行时对象。 */
	void InjectDesktopRuntime();
	/** 将认证状态同步到网页。 */
	void SyncAuthStateToWeb();
	/** 推送当前认证状态到网页。 */
	void PushCurrentAuthStateToWeb();
	/** 处理页面开始加载事件。 */
	void OnLoadStart();
	/** 处理页面加载结束事件。 */
	void OnLoadEnd();
	/** 处理来自网页端的 CEF 查询请求。 */
	void OnCefQueryRequest(const QCefQuery& query);
	/** 处理桌面端登录请求。 */
	void HandleDesktopLoginRequest(const QCefQuery* query = nullptr);
	/** 请求本地文件快照。 */
	void RequestLocalFilesSnapshot(const QCefQuery* query = nullptr);
	/** 将本地文件快照推送到网页。 */
	void PushLocalFilesSnapshot(const LocalFilesSnapshot::Result& result);
	/** 处理打开最近文件请求。 */
	void HandleOpenRecentFileRequest(const QVariantMap& payload, const QCefQuery* query = nullptr);
	/** 打开最近访问的本地文件。 */
	bool OpenRecentLocalFile(const QVariantMap& payload);
	/** 打开最近访问的云端文件。 */
	void OpenRecentCloudFile(const QVariantMap& payload, const QCefQuery* query = nullptr);
	/** 解析最近访问本地文件路径。 */
	QString ResolveRecentLocalPath(const QVariantMap& payload) const;
	/** 判断是否为桌面端最近文件请求。 */
	bool IsDesktopRecentRequest(const QVariantMap& payload) const;
	/** 回复打开最近文件失败信息。 */
	void ReplyOpenRecentFileError(const QCefQuery& query, const QString& message, int error_code = 500);
	/** 回复打开最近文件成功信息。 */
	void ReplyOpenRecentFileSuccess(const QCefQuery& query, const QString& opened_path);
	/** 立即同步原生浏览器窗口。 */
	void SyncNativeBrowserWindowNow();
	/** 安排一次原生浏览器窗口同步。 */
	void ScheduleNativeBrowserWindowSync();
	/** 保存页面访问地址。 */
	QUrl _pageUrl;
	/** 保存嵌入式浏览器视图。 */
	QCefView* _view;
	/** 保存原生浏览器窗口句柄。 */
	QWindow* _nativeBrowserWindow;
	/** 监听本地文件快照异步结果。 */
	QFutureWatcher<LocalFilesSnapshot::Result>* _snapshotWatcher;
	/** 保存认证服务实例。 */
	UserAuthService* _authService;
	/** 保存云文件服务实例。 */
	CloudFileService* _cloudFileService;
	/** 保存待处理的本地文件查询。 */
	QCefQuery _pendingLocalFilesQuery;
	/** 标识是否存在待处理的本地文件查询。 */
	bool _hasPendingLocalFilesQuery;
	/** 保存待处理的最近文件打开查询。 */
	QCefQuery _pendingOpenRecentFileQuery;
	/** 标识是否存在待处理的最近文件打开查询。 */
	bool _hasPendingOpenRecentFileQuery;
	/** 标识原生浏览器窗口同步是否待执行。 */
	bool _nativeBrowserSyncPending;
};

QJ_NAMESPACE_FIT_CLOUD_SERVER_END
