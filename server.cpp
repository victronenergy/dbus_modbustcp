#include "server.h"
#include "backend.h"

//#define QS_LOG_DISABLE
#include "QsLog.h"

Server::Server(QObject *parent) :
	QObject(parent)
{
	mServer = new QTcpServer(this);
	if (mServer->listen(QHostAddress::Any, 1502))
	{
		QLOG_INFO() << "[Server] Server listening at: " << mServer->serverAddress().toString()+":"+QString::number(mServer->serverPort());
		connect(mServer, SIGNAL(newConnection()), SLOT(newConnection()));
	}
	else QLOG_ERROR() << "[Server] QTcpServer error: " + mServer->errorString();
}

void Server::newConnection()
{
	QTcpSocket * newConnection = mServer->nextPendingConnection();

	newConnection->socketOption(QAbstractSocket::LowDelayOption);
	mClients.append(newConnection);
	QLOG_INFO() << "[Server] New connecion: " << newConnection->peerAddress().toString()+":"+QString::number(newConnection->peerPort());
	connect(newConnection, SIGNAL(readyRead()), SLOT(readyRead()));
	connect(newConnection, SIGNAL(disconnected()), SLOT(disconnected()));
}

void Server::readyRead()
{
	QTcpSocket * socket = (QTcpSocket *) sender();
	if (socket == 0) return;

	QByteArray tcpReq = socket->readAll();
	QLOG_TRACE() << "[Server] request from " << socket->peerAddress().toString()+":"+QString::number(socket->peerPort());
	QLOG_TRACE() << "[Server] request data " << tcpReq.toHex().toUpper();
	ADU * request = new ADU(tcpReq);
	mRequests.insert(request, socket);
	QLOG_DEBUG() << "[Server] Request:" << request->toString();
	emit modbusRequest(request);
}

void Server::disconnected()
{
	QTcpSocket *socket = (QTcpSocket *) sender();
	if (socket == 0) return;

	QLOG_INFO() << "[Server] Disconnected: " << socket->peerAddress().toString()+":"+QString::number(socket->peerPort());
	mClients.removeAll(socket);
	delete mRequests.key(socket);
	socket->deleteLater();
}

void Server::modbusReply(ADU * const modbusReply)
{
	QLOG_DEBUG() << "[Server] Reply:" << modbusReply->toString();
	mRequests.value(modbusReply)->write(modbusReply->toQByteArray());
	mRequests.remove(modbusReply);
	delete modbusReply;
}
