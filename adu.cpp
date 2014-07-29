#include "adu.h"

//#define QS_LOG_DISABLE
#include "QsLog.h"

ADU::ADU() :
	PDU(),
	mSocket(0)
{
	mTransID = 0;
	mProdID = 0;
	mLength = 0;
	mUnitID = 0;
}

ADU::ADU(QTcpSocket * const socket, const QByteArray & aduRequest) :
	PDU(aduRequest),
	mSocket(socket)
{
	// Decode MBAP Header
	mTransID = (aduRequest[0] << 8) | (quint8)aduRequest[1];
	mProdID = (aduRequest[2] << 8) | (quint8)aduRequest[3];
	mLength = (aduRequest[4] << 8) | (quint8)aduRequest[5];
	mUnitID = aduRequest[6];
}

ADU::~ADU()
{
}

QByteArray ADU::toQByteArray()
{
	QByteArray reply;

	uint length;
	const ExceptionCode exeptionCode = getExceptionCode();

	// Create MBAP Header
	reply[0] = (quint8)(mTransID >> 8);
	reply[1] = (quint8)mTransID;
	reply[2] = (quint8)(mProdID >> 8);
	reply[3] = (quint8)mProdID;
	// Length later
	reply[6] = (quint8)mUnitID;
	// Create PDU
	reply[7] = getFunctionCode();

	if (exeptionCode == NoExeption) {
		length = mReplyData.size() + 2; // unit ID + function code + data
		reply.append(mReplyData);
	} else {
		length = 2;
		reply[8] = exeptionCode;
	}
	reply[4] = (quint8)(length>> 8);
	reply[5] = (quint8)length;

	return reply;
}

QString ADU::aduToString()
{
	QString string;

	string += "\n\tTransaction Identifier: " + QString::number(mTransID,16).toUpper() + "\n";
	string += "\tProtocol Identifier: " + QString::number(mProdID,16).toUpper() + "\n";
	string += "\tLength: " + QString::number(mLength) + "\n";
	string += "\tUnit Identifier: " + QString::number(mUnitID,16).toUpper() + "\n";
	string += pduToString();
	return string;
}
