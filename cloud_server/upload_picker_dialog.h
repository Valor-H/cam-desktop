#pragma once

#include "cloud_server_global.h"

#include <QDialog>
#include <QUrl>
#include <QVariantMap>

class QCefQuery;
class QCefView;
class QWidget;

QJ_NAMESPACE_FIT_CLOUD_SERVER_BEGIN
class UserAuthService;

class CLOUD_SERVER_EXPORT UploadPickerDialog : public QDialog
{
	Q_OBJECT

public:
	explicit UploadPickerDialog(QWidget* parent, const QUrl& page_url, UserAuthService* auth_service);
	~UploadPickerDialog() override;

signals:
	void UploadTargetSelected(const QVariantMap& payload);

private slots:
	void OnAddressChanged(const QString& url);
	void OnLoadStart();
	void OnLoadEnd(int http_status_code);
	void OnCefQueryRequest(const QCefQuery& query);

private:
	bool IsTrustedUiSource() const;
	void InjectDesktopRuntime();

	QCefView* _view;
	QUrl _currentUrl;
	QUrl _pageUrl;
	UserAuthService* _authService;
};

QJ_NAMESPACE_FIT_CLOUD_SERVER_END
