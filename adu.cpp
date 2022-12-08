#include "adu.h"

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
	mTransID(toUInt16(aduRequest, 0)),
	mProdID(toUInt16(aduRequest, 2)),
	mLength(toUInt16(aduRequest, 4)),
	mUnitID(static_cast<quint8>(aduRequest[6]))
{
}

ADU::~ADU()
{
}

QByteArray ADU::toQByteArray() const
{
	quint16 length = 0;
	quint8 functionCode = getFunctionCode();
	ExceptionCode exeptionCode = getExceptionCode();
	if (exeptionCode == NoExeption) {
		switch(functionCode) {
		case ReadHoldingRegisters:
		case ReadInputRegisters:
			length = static_cast<quint16>(3 + mReplyData.size());
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
	appendUInt16(reply, mTransID);
	appendUInt16(reply, mProdID);
	appendUInt16(reply, length);
	reply.append(static_cast<char>(mUnitID));
	// Create PDU
	reply.append(static_cast<char>(functionCode));

	if (exeptionCode == NoExeption) {
		switch(functionCode) {
		case ReadHoldingRegisters:
		case ReadInputRegisters:
			reply.append(static_cast<char>(mReplyData.size()));
			reply.append(mReplyData);
			break;
		case WriteSingleRegister:
			appendUInt16(reply, getAddres());
			reply.append(mReplyData);
			break;
		case WriteMultipleRegisters:
			appendUInt16(reply, getAddres());
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

QString ADU::aduToString() const
{
	QString string;

	string += "\n\tTransaction Identifier: " + QString::number(mTransID,16).toUpper() + "\n";
	string += "\tProtocol Identifier: " + QString::number(mProdID,16).toUpper() + "\n";
	string += "\tLength: " + QString::number(mLength) + "\n";
	string += "\tUnit Identifier: " + QString::number(mUnitID,16).toUpper() + "\n";
	string += pduToString();
	return string;
}
