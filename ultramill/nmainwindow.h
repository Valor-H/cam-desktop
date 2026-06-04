#pragma once

#include "ultramill_global.h"
#include "workspace_file_context.h"
#include <SARibbonBar/SARibbonMainWindow.h>

#include <QString>

class QAction;
class QEvent;
class QWidget;

QJ_NAMESPACE_BEGIN1(cloudserver)
class UserAuthService;
QJ_NAMESPACE_END1

QJ_NAMESPACE_ULTRACAM_ULTRAMILL_BEGIN

class HomeWorkspace;
class ToolLibDialog;
class CloudController;

class NMainWindow : public SARibbonMainWindow
{
    Q_OBJECT

public:
    explicit NMainWindow(QWidget* parent = nullptr);
    ~NMainWindow() override;

    bool OpenFile(const QString& file_name, const QString& backup_file = "", bool silent = false);
    bool SaveFile(bool silent = false);
    void SetNextFileContext(const WorkspaceFileContext& context);
    qianjizn::cloudserver::UserAuthService& UserAuth();
    const qianjizn::cloudserver::UserAuthService& UserAuth() const;

protected:
    bool event(QEvent* event) override;

private:
    bool AddRecentlyOpenedFile(const QString& file_path);
    bool LoadTextFileIntoWorkspace(const QString& file_path, bool silent);
    void InitializeMainWindowShell();
    void ApplyWindowPresentation();
    void InitCloudController();
    void InitRibbonBar();
    void InitCentralWorkspace();
    void OnShowToolLibDialog();
    void OnOpen();
    void OnSave();
    void OnNewProject();
    void OnHelpDoc();
    void OnAbout();
    void OnLicense();
    QAction* _actionDocument;
    QAction* _actionNew;
    QAction* _actionOpen;
    QAction* _actionSave;
    QString _currentFilePath;
    WorkspaceFileContext _nextFileContext;
    HomeWorkspace* _homeWorkspace;
    ToolLibDialog* _toolLibDialog;
    CloudController* _cloudController;
};

QJ_NAMESPACE_ULTRACAM_ULTRAMILL_END
