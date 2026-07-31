#ifndef TCP_ADU_H
#define TCP_ADU_H

#include <QtCore>
#include <QTcpSocket>
#include "adu.h"

// Modbus TCP ADU: MBAP header (transaction id, protocol id, length, unit id)
// followed by the PDU. Replies are written back to the originating socket.
class TcpAdu : public ADU
{
public:
	TcpAdu(QTcpSocket * const socket, const QByteArray & aduRequest);

	QTcpSocket * getSocket() { return mSocket.data(); }
	uint getTransID() const { return mTransID; }
	uint getProdID() const { return mProdID; }
	uint getLength() const { return mLength; }

	void setTransID(uint id) { mTransID = id; }
	QByteArray toQByteArray() const;
	QString source() const;

private:
	QPointer<QTcpSocket> mSocket;

	// MBAP Header members
	quint16 mTransID;
	quint16 mProdID;
	quint16 mLength;
};

#endif // TCP_ADU_H
