#include "rtu_server.h"
#include "rtu_adu.h"
#include "crc.h"
#include "QsLog.h"

// Sentinels returned by expectedFrameLength() in place of a real byte count.
static const int NeedMoreData = -1;   // not enough bytes buffered to decide yet
static const int UnknownLength = -2;  // function code whose length we cannot derive

// A Modbus RTU frame is delimited by at least 3.5 character times of silence.
// At 8N1 a character is 10 bits, but the official spec always uses
// 11 buts anyway. QTimer is coarser than this anyway, but also, in reality we
// are using USB hardware that typically has a latency of 16ms before it even
// passes the first byte to us, so this value cannot be less than 16. Its only
// purpose is a kind of watchdog that clears buffers if there has been silence
// on the line for long enough, so this doesn't have to be super small or
// accurate.
//FrameTimeoutMs = qMax(3, (35 * 1000) / baud + 1);
static const int FrameTimeoutMs = 32;

RtuServer::RtuServer(const QString &device, int baud, QObject *parent) :
	QObject(parent),
	mPort(new QSerialPort(this))
{

	mPort->setPortName(device);
	mPort->setBaudRate(baud);
	mPort->setDataBits(QSerialPort::Data8);
	mPort->setParity(QSerialPort::NoParity);
	mPort->setStopBits(QSerialPort::OneStop);
	mPort->setFlowControl(QSerialPort::NoFlowControl);

	if (mPort->open(QIODevice::ReadWrite)) {
		QLOG_INFO() << QString("[RtuServer] Listening on %1 at %2 baud 8N1").
					   arg(device).arg(baud);
		connect(mPort, SIGNAL(readyRead()), SLOT(readyRead()));
	} else {
		QLOG_ERROR() << QString("[RtuServer] Failed to open %1: %2").
						arg(device).arg(mPort->errorString());
	}

	mFrameTimer.setSingleShot(true);
	connect(&mFrameTimer, SIGNAL(timeout()), SLOT(frameTimeout()));
}

void RtuServer::readyRead()
{
	mData.append(mPort->readAll());
	processBuffer();
}

void RtuServer::processBuffer()
{
	// USB serial adapters tend to hand us bytes in bursts that do not line up with
	// frame boundaries: two frames can arrive glued together, or a single frame can
	// straddle two reads. Relying on 3.5 characters of silence alone then produces
	// runts (a burst cut short) or over-long frames (two glued together). Instead we
	// delimit by the length implied by the function code and act the moment a whole
	// frame is present.
	for (;;) {
		int len = expectedFrameLength(mData);
		if (len == NeedMoreData || len == UnknownLength)
			break;                       // wait for more bytes or the idle timeout
		if (mData.size() < len)
			break;                       // length known, frame not complete yet

		QByteArray frame = mData.left(len);
		if (crcValid(frame)) {
			mData.remove(0, len);
			QLOG_DEBUG() << "[RtuServer] Dispatching frame" << frame.toHex().toUpper();
			dispatchFrame(frame);
		} else {
			// A length-delimited frame that fails CRC usually means the buffer
			// started on stray line noise rather than a real unit id. Drop one
			// byte and try to resynchronise on the following frame instead of
			// discarding everything after it.
			QLOG_DEBUG() << "[RtuServer] Bad CRC, resyncing" << frame.toHex().toUpper();
			mData.remove(0, 1);
		}
		if (mData.isEmpty())
			break;
	}

	// Anything left is an incomplete frame or an unknown-length function code; let
	// the inter-frame silence flush it. A quiet, empty line stops the timer.
	if (mData.isEmpty())
		mFrameTimer.stop();
	else
		mFrameTimer.start(FrameTimeoutMs);
}

int RtuServer::expectedFrameLength(const QByteArray &frame) const
{
	// Need at least the unit id and function code before anything can be decided.
	if (frame.size() < 2)
		return NeedMoreData;

	switch (static_cast<quint8>(frame[1])) {
	case 1:  // Read Coils
	case 2:  // Read Discrete Inputs
	case 3:  // Read Holding Registers
	case 4:  // Read Input Registers
	case 5:  // Write Single Coil
	case 6:  // Write Single Register
		// unit id + function + address(2) + value/quantity(2) + CRC(2)
		return 8;

	case 15: // Write Multiple Coils
	case 16: // Write Multiple Registers
		// unit id + function + address(2) + quantity(2) + byte count(1) + data(N)
		// + CRC(2). The byte count at offset 6 gives the size of the data block.
		if (frame.size() < 7)
			return NeedMoreData;
		return 9 + static_cast<quint8>(frame[6]);

	default:
		// Unsupported function code: its length cannot be derived from the request,
		// so fall back to the inter-frame timeout to delimit it. The backend will
		// answer a well-formed one with an IllegalFunction exception.
		return UnknownLength;
	}
}

bool RtuServer::crcValid(const QByteArray &frame) const
{
	// Smallest possible frame: unit id + function code + 2 byte CRC.
	if (frame.size() < 4)
		return false;

	quint16 crc = modbusCrc16(frame.left(frame.size() - 2));
	quint8 crcLow = static_cast<quint8>(frame[frame.size() - 2]);
	quint8 crcHigh = static_cast<quint8>(frame[frame.size() - 1]);
	return (crc & 0xFF) == crcLow && (crc >> 8) == crcHigh;
}

void RtuServer::dispatchFrame(const QByteArray &frame)
{
	RtuAdu *request = new RtuAdu(frame);
	QLOG_TRACE() << "[RtuServer] Request:" << request->aduToString();
	emit modbusRequest(request);
}

void RtuServer::frameTimeout()
{
	if (mData.isEmpty())
		return;

	QByteArray frame = mData;
	mData.clear();

	// This only fires when processBuffer() could not delimit the buffer by length:
	// a frame that stalled part-way, or an unsupported function code of unknown
	// length. Accept it only if it is a complete, CRC-valid frame; otherwise it is
	// a runt or line noise and is dropped.
	if (!crcValid(frame)) {
		QLOG_DEBUG() << "[RtuServer] Discarding incomplete/invalid frame"
					 << frame.toHex().toUpper();
		return;
	}

	dispatchFrame(frame);
}

void RtuServer::modbusReply(ADU *reply)
{
	QLOG_TRACE() << "[RtuServer] Reply:" << reply->aduToString();
	mPort->write(reply->toQByteArray());
	delete reply;
}
