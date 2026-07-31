#ifndef CRC_H
#define CRC_H

#include <QByteArray>
#include <QtGlobal>

// Compute the Modbus RTU CRC-16 (polynomial 0xA001) over the given data.
// Returns the CRC accumulator value. On the wire the CRC is appended low byte
// first, i.e. append (crc & 0xFF) then (crc >> 8) at the end of an RTU frame.
quint16 modbusCrc16(const QByteArray &data);

#endif // CRC_H
