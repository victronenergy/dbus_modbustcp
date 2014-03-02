#ifndef PDU_H
#define PDU_H

#include <QtCore>

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
		NoExeption                          = 0,
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

	uint getFunctionCode() { return mFunctionCode; }
	QByteArray getData() { return mData; }
	quint16 getAddres() { return (mData[0] << 8) | (quint8)mData[1]; }
	quint16 getQuantity() { return (mData[2] << 8) | (quint8)mData[3]; }
	quint8 getByteCount() { return mData[4]; }
	ExceptionCode getExceptionCode() { return mExeptionCode; }

	//void setByteCount(uint count) { mByteCount = count; }
	void setData(const QByteArray & data);
	void setExceptionCode(ExceptionCode code);

	//Helpers
	QString toString();

private:
	quint8 mFunctionCode;
	ExceptionCode mExeptionCode;
	QByteArray mData;

	//uint mByteCount;

	// Helpers
	static const QMap <int,QString> initFunctionMap();
	static const QMap <int,QString> functionMap;
	static const QMap <int,QString> initExceptionMap();
	static const QMap <int,QString> exceptionMap;
};

#endif // PDU_H
