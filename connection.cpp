#include <QHostAddress>
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
			mLength = toUInt16(mData, 4) + 6;
			mData.reserve(mLength);
		}
		if (mData.count() == mLength) {
			qDebug() << QString("[Server] request from: %1:%2").
							arg(mSocket->peerAddress().toString()).
							arg(mSocket->peerPort());
			qDebug() << "[Server] request data " << tcpReq.toHex().toUpper();
			ADU *request = new ADU(mSocket, mData);
			qDebug() << "[Server] Request:" << request->aduToString();
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
	qDebug() << QString("[Server] Disconnected: %1:%2").
					arg(socket->peerAddress().toString()).
					arg(socket->peerPort());
	socket->deleteLater();
	deleteLater();
}
