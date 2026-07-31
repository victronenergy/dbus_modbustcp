#include "crc.h"

quint16 modbusCrc16(const QByteArray &data)
{
	quint16 crc = 0xFFFF;
	for (int i = 0; i < data.size(); ++i) {
		crc ^= static_cast<quint8>(data[i]);
		for (int bit = 0; bit < 8; ++bit) {
			if (crc & 0x0001)
				crc = static_cast<quint16>((crc >> 1) ^ 0xA001);
			else
				crc = static_cast<quint16>(crc >> 1);
		}
	}
	return crc;
}
