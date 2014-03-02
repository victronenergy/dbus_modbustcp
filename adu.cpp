#include "adu.h"

#define QS_LOG_DISABLE
#include "QsLog.h"

ADU::ADU()
{
	mTransID = 0;
	mProdID = 0;
	mLength = 0;
	mUnitID = 0;
}

ADU::ADU(QByteArray tcpPacket) :
	mPDU(tcpPacket)
{
	// Decode MBAP Header
	mTransID = (tcpPacket[0] << 8) | (quint8)tcpPacket[1];
	mProdID = (tcpPacket[2] << 8) | (quint8)tcpPacket[3];
	mLength = (tcpPacket[4] << 8) | (quint8)tcpPacket[5];
	mUnitID = tcpPacket[6];
}

QByteArray ADU::toQByteArray()
{
	QByteArray reply;

	uint length;
	const PDU::ExceptionCode exeptionCode = getExceptionCode();

	// Create MBAP Header
	reply[0] = (quint8)(mTransID >> 8);
	reply[1] = (quint8)mTransID;
	reply[2] = (quint8)(mProdID >> 8);
	reply[3] = (quint8)mProdID;
	// Length later
	reply[6] = (quint8)mUnitID;
	// Create PDU
	reply[7] = getFunctionCode();

	if (exeptionCode == PDU::NoExeption) {
		length = getData().size() + 3;
		reply[8] = getData().size();
		reply.append(getData());
	} else {
		length = 2;
		reply[8] = exeptionCode;
	}
	reply[4] = (quint8)(length>> 8);
	reply[5] = (quint8)length;

	QLOG_TRACE() << reply.toHex().toUpper();

	return reply;
}

QString ADU::toString()
{
	QString string;

	string += "Application Data Unit\n";
	string += "\tTransaction Identifier: " + QString::number(mTransID,16).toUpper() + "\n";
	string += "\tProtocol Identifier: " + QString::number(mProdID,16).toUpper() + "\n";
	string += "\tLength: " + QString::number(mLength) + "\n";
	string += "\tUnit Identifier: " + QString::number(mUnitID,16).toUpper() + "\n";
	string += mPDU.toString();
	return string;
}
