#ifndef RTU_ADU_H
#define RTU_ADU_H

#include <QtCore>
#include "adu.h"

// Modbus RTU ADU: unit id byte, PDU, then a 2 byte CRC-16. There is no MBAP
// header and no transaction id. The reply is written back to the single serial
// port owned by the RtuServer, so no per-request socket is needed.
class RtuAdu : public ADU
{
public:
	// The frame must be a complete, CRC-validated RTU frame (unit id + PDU + CRC).
	RtuAdu(const QByteArray & frame);

	QByteArray toQByteArray() const;
	QString source() const;
};

#endif // RTU_ADU_H
