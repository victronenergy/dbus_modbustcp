#ifndef RTU_SERVER_H
#define RTU_SERVER_H

#include <QObject>
#include <QByteArray>
#include <QSerialPort>
#include <QTimer>

#include "adu.h"

// Modbus RTU slave transport. Owns a serial port, delimits incoming frames using
// the inter-character silence, validates the CRC and emits modbusRequest for each
// valid frame. Mirrors the role of Server for the TCP case.
class RtuServer : public QObject
{
	Q_OBJECT
public:
	RtuServer(const QString &device, int baud, QObject *parent = 0);

public slots:
	void modbusReply(ADU *reply);

private slots:
	void readyRead();
	void frameTimeout();

signals:
	void modbusRequest(ADU *request);

private:
	// Consume as many complete frames as the buffer holds, delimiting them by the
	// length implied by the function code rather than waiting for line silence.
	void processBuffer();
	// Total length in bytes of the request frame at the front of the buffer, or
	// NeedMoreData / UnknownLength when it cannot (yet) be determined.
	int expectedFrameLength(const QByteArray &frame) const;
	bool crcValid(const QByteArray &frame) const;
	void dispatchFrame(const QByteArray &frame);

	QSerialPort *mPort;
	QTimer mFrameTimer;
	QByteArray mData;
};

#endif // RTU_SERVER_H
