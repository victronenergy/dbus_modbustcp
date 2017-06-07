#include "adu.h"

//#define QS_LOG_DISABLE
#include "QsLog.h"

ADU::ADU() :
	PDU(),
	mSocket(0),
	mTransID(0),
	mProdID(0),
	mLength(0),
	mUnitID(0)
{
}

ADU::ADU(QTcpSocket * const socket, const QByteArray & aduRequest) :
	PDU(aduRequest),
	mSocket(socket),
	// Decode MBAP Header
	mTransID((aduRequest[0] << 8) | (quint8)aduRequest[1]),
	mProdID((aduRequest[2] << 8) | (quint8)aduRequest[3]),
	mLength((aduRequest[4] << 8) | (quint8)aduRequest[5]),
	mUnitID(aduRequest[6])
{
}

ADU::~ADU()
{
}

QByteArray ADU::toQByteArray()
{
	uint length = 0;
	uint functionCode = getFunctionCode();
	ExceptionCode exeptionCode = getExceptionCode();
	if (exeptionCode == NoExeption) {
		switch(functionCode) {
		case ReadHoldingRegisters:
		case ReadInputRegisters:
			length = 3 + mReplyData.size();
			break;
		case WriteSingleRegister:
			length = 6;
			break;
		case WriteMultipleRegisters:
			length = 6;
			break;
		default:
			break;
		}
	} else {
		length = 3;
	}

	QByteArray reply;
	reply.reserve(length + 6);
	// Create MBAP Header
	reply.append((quint8)(mTransID >> 8));
	reply.append((quint8)mTransID);
	reply.append((quint8)(mProdID >> 8));
	reply.append((quint8)mProdID);
	reply.append(static_cast<quint8>(length>> 8));
	reply.append(static_cast<quint8>(length));
	reply.append((quint8)mUnitID);
	// Create PDU
	reply.append(functionCode);

	if (exeptionCode == NoExeption) {
		switch(functionCode) {
		case ReadHoldingRegisters:
		case ReadInputRegisters:
			reply.append(static_cast<char>(mReplyData.size()));
			reply.append(mReplyData);
			break;
		case WriteSingleRegister:
			reply.append(static_cast<char>(getAddres() >> 8));
			reply.append(static_cast<char>(getAddres()));
			reply.append(mReplyData);
			break;
		case WriteMultipleRegisters:
			reply.append(static_cast<char>(getAddres() >> 8));
			reply.append(static_cast<char>(getAddres()));
			reply.append(static_cast<char>(0));
			reply.append(static_cast<char>(getByteCount() / 2));
			break;
		default:
			break;
		}
	} else {
		reply.append(exeptionCode);
	}
	Q_ASSERT(reply.size() == length + 6);
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
