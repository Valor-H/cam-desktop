#pragma once

#include "ultramill_global.h"
#include <SARibbonBar/SARibbonMainWindow.h>

#include <cloud_server/cloud_file_state.h>
#include <cloud_server/open_request_context.h>

#include <QString>

class QAction;
class QEvent;
class QWidget;

QJ_NAMESPACE_BEGIN1(cloudserver)
#ifdef ENABLE_CLOUD_SERVER_MODULE
class UserAuthService;
#endif
QJ_NAMESPACE_END1

QJ_NAMESPACE_ULTRACAM_ULTRAMILL_BEGIN

class HomeWorkspace;
class ToolLibDialog;
#ifdef ENABLE_CLOUD_SERVER_MODULE
class CloudController;
#endif

class NMainWindow : public SARibbonMainWindow
{
    Q_OBJECT

public:
    explicit NMainWindow(QWidget* parent = nullptr);
    ~NMainWindow() override;

    const qianjizn::cloudserver::CloudFileState& CurrentFileState() const { return _currentFileState; }
    bool OpenFile(const QString& file_name, const QString& backup_file = "", bool silent = false);
    bool SaveFile(bool silent = false);
	bool SaveAsFile(const QString& file_path = QString(), bool silent = false);
    void SetPendingOpenRequest(const qianjizn::cloudserver::OpenRequestContext& context);
#ifdef ENABLE_CLOUD_SERVER_MODULE
    qianjizn::cloudserver::UserAuthService& UserAuth();
    const qianjizn::cloudserver::UserAuthService& UserAuth() const;
#endif

protected:
    bool event(QEvent* event) override;

private:
    bool AddRecentlyOpenedFile(const QString& file_path);
    bool LoadTextFileIntoWorkspace(const QString& file_path, bool silent);
	bool SaveWorkspaceToPath(const QString& file_path, bool silent);
    void ApplyOpenedFileState(const qianjizn::cloudserver::OpenRequestContext& open_context);
    void InitializeMainWindowShell();
    void ApplyWindowPresentation();
#ifdef ENABLE_CLOUD_SERVER_MODULE
    void InitCloudController();
#endif
    void InitRibbonBar();
    void InitCentralWorkspace();
    void OnShowToolLibDialog();
    void OnOpen();
    void OnSave();
	void OnSaveAs();
#ifdef ENABLE_CLOUD_SERVER_MODULE
    void OnUpload();
#endif
    void OnNewProject();
    void OnHelpDoc();
    void OnAbout();
    void OnLicense();
#ifdef ENABLE_CLOUD_SERVER_MODULE
    QAction* _actionDocument;
    QAction* _actionUpload;
#endif
    QAction* _actionNew;
    QAction* _actionOpen;
    QAction* _actionSave;
	QAction* _actionSaveAs;
    qianjizn::cloudserver::CloudFileState _currentFileState;
    qianjizn::cloudserver::OpenRequestContext _pendingOpenRequest;
    HomeWorkspace* _homeWorkspace;
    ToolLibDialog* _toolLibDialog;
#ifdef ENABLE_CLOUD_SERVER_MODULE
    CloudController* _cloudController;
#endif
};

QJ_NAMESPACE_ULTRACAM_ULTRAMILL_END
