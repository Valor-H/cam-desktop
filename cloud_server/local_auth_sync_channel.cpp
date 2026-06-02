#include "local_auth_sync_channel.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QLocalServer>
#include <QLocalSocket>
#include <QTimer>

QJ_USING_NAMESPACE_FIT_CLOUD_SERVER

namespace
{
	const char kAuthSyncEventKey[] = "event";
	const char kAuthSyncAuthenticatedKey[] = "authenticated";
	const char kAuthSyncEventValue[] = "auth-state-changed";

	QString buildAuthSyncServerName(const CloudServerConfig& cfg)
	{
		QString name = QStringLiteral("%1.%2.auth-sync")
			.arg(cfg.settingsOrg.trimmed(), cfg.settingsApp.trimmed());
		for (QChar& ch : name) {
			if (!ch.isLetterOrNumber() && ch != QLatin1Char('.') && ch != QLatin1Char('_')) {
				ch = QLatin1Char('_');
			}
		}
		return name;
	}

	QByteArray buildAuthSyncPayload(bool authenticated)
	{
		QJsonObject root;
		root.insert(QString::fromLatin1(kAuthSyncEventKey), QString::fromLatin1(kAuthSyncEventValue));
		root.insert(QString::fromLatin1(kAuthSyncAuthenticatedKey), authenticated);

		QByteArray payload = QJsonDocument(root).toJson(QJsonDocument::Compact);
		payload.append('\n');
		return payload;
	}

	bool parseAuthSyncPayload(const QByteArray& payload, bool* authenticated)
	{
		if (!authenticated) {
			return false;
		}

		const QJsonDocument doc = QJsonDocument::fromJson(payload.trimmed());
		if (!doc.isObject()) {
			return false;
		}

		const QJsonObject root = doc.object();
		if (root.value(QString::fromLatin1(kAuthSyncEventKey)).toString()
			!= QString::fromLatin1(kAuthSyncEventValue)) {
			return false;
		}

		if (!root.contains(QString::fromLatin1(kAuthSyncAuthenticatedKey))) {
			return false;
		}

		*authenticated = root.value(QString::fromLatin1(kAuthSyncAuthenticatedKey)).toBool();
		return true;
	}
}

QJ_NAMESPACE_FIT_CLOUD_SERVER_BEGIN

LocalAuthSyncChannel::LocalAuthSyncChannel(const CloudServerConfig& cfg,
	std::function<void(bool)> onAuthStateChanged,
	QObject* parent)
	: QObject(parent)
	, _serverName(buildAuthSyncServerName(cfg))
	, _server(new QLocalServer(this))
	, _onAuthStateChanged(std::move(onAuthStateChanged))
{
	_reconnectTimer = new QTimer(this);
	_reconnectTimer->setSingleShot(true);

	connect(_reconnectTimer, &QTimer::timeout, this, [this]() {
		if (_ownsServer) {
			return;
		}
		if (!ConnectToServer()) {
			BecomeServer();
		}
		});

	connect(_server, &QLocalServer::newConnection, this, [this]() {
		while (_server->hasPendingConnections()) {
			QLocalSocket* peer = _server->nextPendingConnection();
			if (!peer) {
				continue;
			}

			_peers.append(peer);
			connect(peer, &QLocalSocket::readyRead, this, [this, peer]() {
				ProcessMessages(peer, true);
				});
			connect(peer, &QLocalSocket::disconnected, this, [this, peer]() {
				_peers.removeAll(peer);
				peer->deleteLater();
				});

			WriteMessage(peer, false);
		}
		});

	if (!ConnectToServer()) {
		BecomeServer();
	}
}

void LocalAuthSyncChannel::Broadcast(bool authenticated)
{
	if (_ownsServer) {
		const auto peers = _peers;
		for (QLocalSocket* peer : peers) {
			WriteMessage(peer, authenticated);
		}
		return;
	}

	WriteMessage(_client, authenticated);
}

bool LocalAuthSyncChannel::ConnectToServer()
{
	if (_serverName.isEmpty()) {
		return false;
	}

	ResetClient();

	QLocalSocket* socket = new QLocalSocket(this);
	connect(socket, &QLocalSocket::readyRead, this, [this]() {
		ProcessMessages(_client, false);
		});
	connect(socket, &QLocalSocket::disconnected, this, [this]() {
		ResetClient();
		ScheduleReconnect(250);
		});
	connect(socket,
		qOverload<QLocalSocket::LocalSocketError>(&QLocalSocket::errorOccurred),
		this,
		[this](QLocalSocket::LocalSocketError) {
			ScheduleReconnect(250);
		});

	socket->connectToServer(_serverName, QIODevice::ReadWrite);
	if (!socket->waitForConnected(100)) {
		socket->deleteLater();
		return false;
	}

	_client = socket;
	_ownsServer = false;
	return true;
}

void LocalAuthSyncChannel::BecomeServer()
{
	if (_serverName.isEmpty()) {
		return;
	}

	ResetClient();
	_server->close();
	QLocalServer::removeServer(_serverName);
	if (_server->listen(_serverName)) {
		_ownsServer = true;
		return;
	}

	_ownsServer = false;
	ScheduleReconnect(500);
}

void LocalAuthSyncChannel::ResetClient()
{
	if (!_client) {
		return;
	}

	_client->disconnect(this);
	_client->abort();
	_client->deleteLater();
	_client = nullptr;
}

void LocalAuthSyncChannel::ScheduleReconnect(int delayMs)
{
	if (_ownsServer || !_reconnectTimer) {
		return;
	}
	_reconnectTimer->start(delayMs);
}

void LocalAuthSyncChannel::WriteMessage(QLocalSocket* socket, bool authenticated)
{
	if (!socket || socket->state() != QLocalSocket::ConnectedState) {
		return;
	}

	socket->write(buildAuthSyncPayload(authenticated));
	socket->flush();
}

void LocalAuthSyncChannel::ProcessMessages(QLocalSocket* socket, bool rebroadcast)
{
	if (!socket) {
		return;
	}

	while (socket->canReadLine()) {
		bool authenticated = false;
		if (!parseAuthSyncPayload(socket->readLine(), &authenticated)) {
			continue;
		}

		if (rebroadcast) {
			const auto peers = _peers;
			for (QLocalSocket* peer : peers) {
				if (peer == socket) {
					continue;
				}
				WriteMessage(peer, authenticated);
			}
		}

		if (_onAuthStateChanged) {
			_onAuthStateChanged(authenticated);
		}
	}
}

QJ_NAMESPACE_FIT_CLOUD_SERVER_END
