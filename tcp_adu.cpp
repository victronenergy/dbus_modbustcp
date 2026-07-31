#include "tcp_adu.h"

//#define QS_LOG_DISABLE
#include "QsLog.h"

TcpAdu::TcpAdu(QTcpSocket * const socket, const QByteArray & aduRequest) :
	// The function code is at offset 7, after the 7 byte MBAP header.
	ADU(aduRequest, 7, static_cast<quint8>(aduRequest[6])),
	mSocket(socket),
	// Decode MBAP Header
	mTransID(toUInt16(aduRequest, 0)),
	mProdID(toUInt16(aduRequest, 2)),
	mLength(toUInt16(aduRequest, 4))
{
}

QByteArray TcpAdu::toQByteArray() const
{
	QByteArray pdu = buildPduReply();

	QByteArray reply;
	reply.reserve(pdu.size() + 7);
	// Create MBAP Header. The length field covers the unit id plus the PDU.
	appendUInt16(reply, mTransID);
	appendUInt16(reply, mProdID);
	appendUInt16(reply, static_cast<quint16>(pdu.size() + 1));
	reply.append(static_cast<char>(mUnitID));
	// Append the PDU
	reply.append(pdu);
	return reply;
}

QString TcpAdu::source() const
{
	return mSocket ? mSocket->peerAddress().toString() : QString("unknown");
}
