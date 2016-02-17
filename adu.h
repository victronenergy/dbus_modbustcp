#ifndef ADU_H
#define ADU_H

#include <QtCore>
#include <QTcpSocket>
#include "pdu.h"

class ADU : public PDU
{
public:
	ADU();
	ADU(QTcpSocket * const socket, const QByteArray & aduRequest);
	~ADU();

	QTcpSocket * getSocket() { return mSocket.data(); }
	QByteArray & getReplyDataRef() { return mReplyData; }
	//void setReplyData(const QByteArray & replyData) { mReplyData = replyData; }
	uint getTransID() { return mTransID; }
	uint getProdID() { return mProdID; }
	uint getLength() { return mLength; }
	uint getUnitID() { return mUnitID; }

	void setTransID(uint id) { mTransID = id; }
	QByteArray toQByteArray();

	// Helpers
	QString aduToString();

private:
	QPointer<QTcpSocket> mSocket;
	QByteArray mReplyData;

	// MBAP Header members
	quint16 mTransID;
	quint16 mProdID;
	quint16 mLength;
	quint8 mUnitID;
};

#endif // ADU_H
