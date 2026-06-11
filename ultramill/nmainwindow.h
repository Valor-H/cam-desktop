#pragma once

#include "ultramill_global.h"
#include <SARibbonBar/SARibbonMainWindow.h>

#include <QString>

class QAction;
class QEvent;
class QWidget;

QJ_NAMESPACE_BEGIN1(cloudserver)
#ifdef ENABLE_CLOUD_SERVER_MODULE
class OpenRequestContext;
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

    bool OpenFile(const QString& file_name, const QString& backup_file = "", bool silent = false);
    bool SaveFile(bool silent = false);
	bool SaveAsFile(const QString& file_path = QString(), bool silent = false);
	bool IsEmptyDraftWindow() const;
	bool StartNewProcessForOpenRequest(const qianjizn::cloudserver::OpenRequestContext& context) const;
#ifdef ENABLE_CLOUD_SERVER_MODULE
    void SetPendingOpenRequest(const qianjizn::cloudserver::OpenRequestContext& context);
    qianjizn::cloudserver::UserAuthService& UserAuth();
    const qianjizn::cloudserver::UserAuthService& UserAuth() const;
#endif

protected:
    bool event(QEvent* event) override;

private:
    bool AddRecentlyOpenedFile(const QString& file_path);
    bool LoadTextFileIntoWorkspace(const QString& file_path, bool silent);
	bool SaveWorkspaceToPath(const QString& file_path, bool silent);
    void InitializeMainWindowShell();
    void ApplyWindowPresentation();
#ifdef ENABLE_CLOUD_SERVER_MODULE
    void InitCloudController();
#endif
    void InitRibbonBar();
    void InitCentralWorkspace();
    void UpdateWindowTitle();
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
    HomeWorkspace* _homeWorkspace;
    ToolLibDialog* _toolLibDialog;
    QString _currentFilePath;
#ifdef ENABLE_CLOUD_SERVER_MODULE
    CloudController* _cloudController;
#endif
};

QJ_NAMESPACE_ULTRACAM_ULTRAMILL_END
