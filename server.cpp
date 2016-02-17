#include "server.h"
#include "backend.h"
#include "QsLog.h"

Server::Server(int tcpPort, QObject *parent) :
	QObject(parent)
{
	mServer = new QTcpServer(this);
	if (mServer->listen(QHostAddress::Any, tcpPort)) {
		QLOG_INFO() << QString("[Server] Server listening at: %1:%2").
					   arg(mServer->serverAddress().toString()).
					   arg(mServer->serverPort());
		connect(mServer, SIGNAL(newConnection()), SLOT(newConnection()));
	}
	else {
		QLOG_ERROR() << "[Server] QTcpServer error: " + mServer->errorString();
	}
}

void Server::newConnection()
{
	QTcpSocket * newConnection = mServer->nextPendingConnection();

	newConnection->socketOption(QAbstractSocket::LowDelayOption);
	QLOG_TRACE() << QString("[Server] New connecion: %1:%2").
					arg(newConnection->peerAddress().toString()).
					arg(newConnection->peerPort());
	connect(newConnection, SIGNAL(readyRead()), SLOT(readyRead()));
	connect(newConnection, SIGNAL(disconnected()), SLOT(disconnected()));
	connect(newConnection, SIGNAL(bytesWritten(qint64)), SLOT(bytesWritten(qint64)));
}

void Server::readyRead()
{
	QTcpSocket * socket = static_cast<QTcpSocket *>(sender());

	QByteArray tcpReq = socket->readAll();
	QLOG_DEBUG() << QString("[Server] request from: %1:%2").
					arg(socket->peerAddress().toString()).
					arg(socket->peerPort());
	QLOG_DEBUG() << "[Server] request data " << tcpReq.toHex().toUpper();
	ADU * request = new ADU(socket, tcpReq);
	QLOG_DEBUG() << "[Server] Request:" << request->aduToString();
	emit modbusRequest(request);
}

void Server::disconnected()
{
	QTcpSocket * socket = static_cast<QTcpSocket *>(sender());

	QLOG_TRACE() << QString("[Server] Disconnected: %1:%2").
					arg(socket->peerAddress().toString()).
					arg(socket->peerPort());
	socket->deleteLater();
}

void Server::modbusReply(ADU *modbusReply)
{
	QLOG_DEBUG() << "[Server] Reply:" << modbusReply->aduToString();
	QTcpSocket *socket = modbusReply->getSocket();
	if (socket == 0)
		return;
	socket->write(modbusReply->toQByteArray());
	delete modbusReply;
}

void Server::bytesWritten(qint64 bytes)
{
	QTcpSocket * socket = static_cast<QTcpSocket *>(sender());

	QLOG_DEBUG() << QString("[Server] bytes written: %1 %2:%3").
					arg(bytes).
					arg(socket->peerAddress().toString()).
					arg(socket->peerPort());
}
