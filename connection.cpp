#include <QHostAddress>
#include <QsLog.h>
#include "connection.h"

Connection::Connection(QTcpSocket *socket, QObject *parent):
	QObject(parent),
	mSocket(socket),
	mLength(-1)
{
	connect(socket, SIGNAL(readyRead()), SLOT(readyRead()));
	connect(socket, SIGNAL(disconnected()), SLOT(disconnected()));
}

void Connection::readyRead()
{
	QByteArray tcpReq = mSocket->readAll();
	foreach(char byte, tcpReq) {
		mData.append(byte);
		if (mData.count() == 6) {
			mLength = (static_cast<quint8>(mData[4]) << 8 | static_cast<quint8>(mData[5])) + 6;
			mData.reserve(mLength);
		}
		if (mData.count() == mLength) {
			QLOG_DEBUG() << QString("[Server] request from: %1:%2").
							arg(mSocket->peerAddress().toString()).
							arg(mSocket->peerPort());
			QLOG_TRACE() << "[Server] request data " << tcpReq.toHex().toUpper();
			ADU *request = new ADU(mSocket, mData);
			QLOG_TRACE() << "[Server] Request:" << request->aduToString();
			mLength = -1;
			mData.resize(0);
			mData.reserve(6);
			emit modbusRequest(request);
		}
	}
}

void Connection::disconnected()
{
	QTcpSocket *socket = static_cast<QTcpSocket *>(sender());
	QLOG_TRACE() << QString("[Server] Disconnected: %1:%2").
					arg(socket->peerAddress().toString()).
					arg(socket->peerPort());
	socket->deleteLater();
	deleteLater();
}
