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
	void setReplyData(const QByteArray &replyData) { mReplyData = replyData; }
	uint getTransID() const { return mTransID; }
	uint getProdID() const { return mProdID; }
	uint getLength() const { return mLength; }
	uint getUnitID() const { return mUnitID; }

	void setTransID(uint id) { mTransID = id; }
	QByteArray toQByteArray() const;

	// Helpers
	QString aduToString() const;

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
