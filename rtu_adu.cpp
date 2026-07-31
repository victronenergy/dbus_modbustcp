#include "rtu_adu.h"
#include "crc.h"

//#define QS_LOG_DISABLE
#include "QsLog.h"

RtuAdu::RtuAdu(const QByteArray & frame) :
	// The function code is at offset 1, after the unit id. The trailing 2 CRC
	// bytes are dropped so the PDU data does not include them.
	ADU(frame.left(frame.size() - 2), 1, static_cast<quint8>(frame[0]))
{
}

QByteArray RtuAdu::toQByteArray() const
{
	QByteArray reply;
	reply.append(static_cast<char>(mUnitID));
	reply.append(buildPduReply());

	quint16 crc = modbusCrc16(reply);
	reply.append(static_cast<char>(crc & 0xFF));
	reply.append(static_cast<char>(crc >> 8));
	return reply;
}

QString RtuAdu::source() const
{
	return QString("serial unit %1").arg(mUnitID);
}
