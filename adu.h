#ifndef ADU_H
#define ADU_H

#include <QtCore>
#include "pdu.h"

// Transport-agnostic Application Data Unit. It carries the unit id, the PDU (via
// the PDU base) and the reply data. Concrete subclasses (TcpAdu, RtuAdu) add the
// transport specific framing (MBAP header / CRC) and know how to deliver a reply.
class ADU : public PDU
{
public:
	virtual ~ADU() {}

	uint getUnitID() const { return mUnitID; }
	void setReplyData(const QByteArray &replyData) { mReplyData = replyData; }

	// Serialize the complete reply frame (transport specific framing included).
	virtual QByteArray toQByteArray() const = 0;

	// Human readable description of where the request came from, used for logging.
	virtual QString source() const = 0;

	// Helpers
	QString aduToString() const;

protected:
	ADU() : PDU(), mUnitID(0) {}
	ADU(const QByteArray &frame, int pduStart, quint8 unitID) :
		PDU(frame, pduStart), mUnitID(unitID) {}

	// Build the reply PDU (function code + payload / exception), shared by all
	// transports. The transport specific framing is added by toQByteArray().
	QByteArray buildPduReply() const;

	quint8 mUnitID;
	QByteArray mReplyData;
};

#endif // ADU_H
