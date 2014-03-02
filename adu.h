#ifndef ADU_H
#define ADU_H

#include <QtCore>
#include "pdu.h"

class ADU
{
public:
	ADU();
	ADU(QByteArray tcpPacket);

	uint getTransID() { return mTransID; }
	uint getProdID() { return mProdID; }
	uint getLength() { return mLength; }
	uint getUnitID() { return mUnitID; }

	void setTransID(uint id) { mTransID = id; }

	quint16 getFunctionCode() { return mPDU.getFunctionCode(); }
	QByteArray getData() { return mPDU.getData(); }
	quint16 getAddres() { return mPDU.getAddres(); }
	quint16 getQuantity() { return mPDU.getQuantity(); }
	quint8 getByteCount() { return mPDU.getByteCount(); }
	PDU::ExceptionCode getExceptionCode() { return mPDU.getExceptionCode(); }

	//void setByteCount(const uint count) { mPDU.setByteCount(count); }
	void setData(const QByteArray & data) { mPDU.setData(data); }
	void setExceptionCode(PDU::ExceptionCode code) { mPDU.setExceptionCode(code); }

	QByteArray toQByteArray();

	// Helpers
	QString toString();

private:
	// MBAP Header members
	quint16 mTransID;
	quint16 mProdID;
	quint16 mLength;
	quint8 mUnitID;

	PDU mPDU;
};

#endif // ADU_H
