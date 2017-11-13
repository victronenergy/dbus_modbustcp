#ifndef PDU_H
#define PDU_H

#include <QtCore>

inline quint16 toUInt16(const QByteArray &array, int offset)
{
	return static_cast<quint16>(
		(static_cast<quint8>(array[offset]) << 8) |
		static_cast<quint8>(array[offset + 1]));
}

inline void appendUInt16(QByteArray &array, quint16 v) {
	array.append(static_cast<char>(v >> 8));
	array.append(static_cast<char>(v & 0xFF));
}

class PDU
{
public:
	PDU();
	PDU(const QByteArray & pduRequest);

	enum FunctionCode
	{
		ReadCoils						= 1,
		ReadDiscreteInputs				= 2,
		ReadHoldingRegisters			= 3,
		ReadInputRegisters				= 4,
		WriteSingleCoil					= 5,
		WriteSingleRegister				= 6,
		WriteMultipleCoils				= 15,
		WriteMultipleRegisters			= 16,
		ReadFileRecord					= 20,
		WriteFileRecord					= 21,
		MaskWriteRegister				= 22,
		ReadWriteMultipleRegisters		= 23,
		ReadFIFOQueue					= 24,
		EncapsulatedInterfaceTransport	= 43,
	};

	enum ExceptionCode
	{
		NoExeption							= 0,
		IllegalFunction						= 1,
		IllegalDataAddress					= 2,
		IllegalDataValue					= 3,
		SlaveDeviceFailure					= 4,
		Acknowledge							= 5,
		SlaveDeviceBusy						= 6,
		MemoryParityError					= 7,
		GatewayPathUnavailable				= 10,
		GatewayTargetDeviceFailedToRespond	= 11
	};

	quint8 getFunctionCode() const { return mFunctionCode; }
	QByteArray & getData() { return mData; }
	int getDataSize() const { return mData.size(); }
	quint16 getAddres() const { return toUInt16(mData, 0); }
	quint16 getQuantity() const { return toUInt16(mData, 2); }
	quint8 getByteCount() const { return static_cast<quint8>(mData[4]); }
	ExceptionCode getExceptionCode() const { return mExeptionCode; }

	void setData(const QByteArray & data);
	void setExceptionCode(ExceptionCode code);

	//Helpers
	QString pduToString() const;

private:
	quint8 mFunctionCode;
	ExceptionCode mExeptionCode;
	QByteArray mData;

	// Helpers
	static const QMap<int,QString> initFunctionMap();
	static const QMap<int,QString> functionMap;
	static const QMap<int,QString> initExceptionMap();
	static const QMap<int,QString> exceptionMap;
};

#endif // PDU_H
