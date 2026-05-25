#pragma once

#include "cloud_server_config.h"

#include <QList>
#include <QObject>
#include <QString>
#include <functional>

class QLocalServer;
class QLocalSocket;
class QTimer;

QJ_NAMESPACE_FIT_CLOUD_SERVER_BEGIN

class LocalAuthSyncChannel final : public QObject
{
public:
    /** 构造本地认证状态同步通道。 */
    LocalAuthSyncChannel(const CloudServerConfig& cfg,
                         std::function<void(bool)> onAuthStateChanged,
                         QObject* parent = nullptr);

    /** 向其他实例广播当前认证状态。 */
    void Broadcast(bool authenticated);

private:
    /** 连接已有的本地同步服务端。 */
    bool ConnectToServer();
    /** 当前实例切换为本地同步服务端。 */
    void BecomeServer();
    /** 重置当前客户端连接。 */
    void ResetClient();
    /** 安排一次延迟重连。 */
    void ScheduleReconnect(int delayMs);
    /** 向指定连接写入同步消息。 */
    void WriteMessage(QLocalSocket* socket, bool authenticated);
    /** 处理收到的同步消息。 */
    void ProcessMessages(QLocalSocket* socket, bool rebroadcast);

    QString _serverName;
    QLocalServer* _server { nullptr };
    QLocalSocket* _client { nullptr };
    QList<QLocalSocket*> _peers;
    QTimer* _reconnectTimer { nullptr };
    std::function<void(bool)> _onAuthStateChanged;
    bool _ownsServer { false };
};

QJ_NAMESPACE_FIT_CLOUD_SERVER_END