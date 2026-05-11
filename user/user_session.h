#pragma once

#include "user_global.h"

#include <QObject>
#include <QString>
#include <QVariantMap>

QJ_NAMESPACE_FIT_USER_BEGIN

class USER_EXPORT UserSession final : public QObject
{
    Q_OBJECT

public:
    explicit UserSession(QObject* parent = nullptr);

    bool IsAuthenticated() const { return _authenticated; }
    QString AuthToken() const { return _authToken; }
    QVariantMap CurrentUser() const { return _currentUser; }

    void ApplyFromLoginPayload(const QVariantMap& payload);
    void ApplyFromProbe(const QVariantMap& data);
    void Logout();

signals:
    void AuthStateChanged(bool authenticated);
    void UserProfileChanged();

private:
    void SetAuthenticatedState(bool on);

    bool _authenticated { false };
    QString _authToken;
    QVariantMap _currentUser;
};

QJ_NAMESPACE_FIT_USER_END
