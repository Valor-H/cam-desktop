#pragma once

#include "user_global.h"

#include "LocalFilesSnapshot.h"
#include "UserAuthService.h"

#include <QByteArray>
#include <QFutureWatcher>
#include <QUrl>
#include <QVariant>
#include <QWidget>

#include <QCefQuery.h>

class QString;

class QCefView;
class AuthHttpClient;

class USER_EXPORT FileManagerView : public QWidget
{
    Q_OBJECT

public:
    explicit FileManagerView(QWidget* parent, qianjizn::user::UserAuthService* authService, const QUrl& pageUrl);
    ~FileManagerView() override;
    void NavigateTo(const QUrl& pageUrl);

signals:
    void OpenFileRequested(const QString& filePath);

private:
    void ApplyEmbeddedScale();
    void SyncAuthStateToWeb();
    void PushCurrentAuthStateToWeb();
    void OnLoadEnd();
    void OnCefQueryRequest(const QCefQuery& query);
    void HandleDesktopLoginRequest(const QCefQuery* query = nullptr);
    void RequestLocalFilesSnapshot(const QCefQuery* query = nullptr);
    void PushLocalFilesSnapshot(const LocalFilesSnapshot::Result& result);
    void HandleOpenRecentFileRequest(const QVariantMap& payload, const QCefQuery* query = nullptr);
    bool OpenRecentLocalFile(const QVariantMap& payload);
    void OpenRecentCloudFile(const QVariantMap& payload, const QCefQuery* query = nullptr);
    QString ResolveRecentLocalPath(const QVariantMap& payload) const;
    QString BuildCacheFilePath(const QVariantMap& payload) const;
    bool IsDesktopRecentRequest(const QVariantMap& payload) const;
    void ReplyOpenRecentFileError(const QCefQuery& query, const QString& message, int errorCode = 500);
    void ReplyOpenRecentFileSuccess(const QCefQuery& query, const QString& openedPath);
    AuthHttpClient* EnsureFileHttpClient();

    QUrl m_pageUrl;
    QCefView* m_view { nullptr };
    QFutureWatcher<LocalFilesSnapshot::Result>* m_snapshotWatcher { nullptr };
    AuthHttpClient* m_fileHttpClient { nullptr };
    qianjizn::user::UserAuthService* m_authService { nullptr };
    QCefQuery m_pendingLocalFilesQuery;
    bool m_hasPendingLocalFilesQuery { false };
    QCefQuery m_pendingOpenRecentFileQuery;
    bool m_hasPendingOpenRecentFileQuery { false };
};
