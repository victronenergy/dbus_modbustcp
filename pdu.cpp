#include "pdu.h"

#define QS_LOG_DISABLE
#include "QsLog.h"

const QMap <int,QString> PDU::initFunctionMap()
{
	QMap <int,QString> functionMap;
	functionMap[1] = "Read Coils";
	functionMap[2] = "Read Discrete Inputs";
	functionMap[3] = "Read Holding Registers";
	functionMap[4] = "Read Input Registers";
	functionMap[5] = "Write Single Coil";
	functionMap[6] = "Write Single Register";
	functionMap[7] = "Read Exception Status";
	functionMap[8] = "Diagnostics";
	functionMap[11] = "Get Comm Event Counter";
	functionMap[12] = "Get Comm Event Log";
	functionMap[15] = "Write Multiple Coils";
	functionMap[16] = "Write Multiple registers";
	functionMap[17] = "Report Slave ID";
	functionMap[20] = "Read File Record";
	functionMap[21] = "Write File Record";
	functionMap[22] = "Mask Write Register";
	functionMap[23] = "Read/Write Multiple registers";
	functionMap[24] = "Read FIFO Queue";
	functionMap[43] = "Encapsulated Interface Transport";
	return functionMap;
}
const QMap <int,QString> PDU::functionMap = initFunctionMap();

const QMap <int,QString> PDU::initExceptionMap()
{
	QMap <int,QString> exceptionMap;
	exceptionMap[0] = "No Exeption";
	exceptionMap[1] = "Illegal Function";
	exceptionMap[2] = "Illegal Data Address";
	exceptionMap[3] = "Illegal Data Value";
	exceptionMap[4] = "Slave Device Failure";
	exceptionMap[5] = "Acknowledge";
	exceptionMap[6] = "Slave Device Busy";
	exceptionMap[7] = "Memory Parity Error";
	exceptionMap[10] = "Gateway Path Unavailable";
	exceptionMap[11] = "Gateway Target Device Failed To Respond";
	return exceptionMap;
}
const QMap <int,QString> PDU::exceptionMap = initExceptionMap();

PDU::PDU()
{
	mFunctionCode = 0;
	mExeptionCode = NoExeption;
}

PDU::PDU(const QByteArray & pduRequest)
{
	// First 6 byte are MBAP Header so starting with 7
	mFunctionCode = pduRequest[7];
	mExeptionCode = NoExeption;
	mData = pduRequest.mid(8);
}

void PDU::setData(const QByteArray & data)
{
	mData = data;
}

void PDU::setExceptionCode(ExceptionCode code)
{
	mFunctionCode |= 0x80;
	mExeptionCode = code;
}

QString PDU::pduToString()
{
	QString string;

	if (mFunctionCode >= 0x80)
		string += "\tException Code: " + exceptionMap[mExeptionCode] + "\n";
	else
		string += "\tFunction Code: " + functionMap[mFunctionCode] + "\n";
	string += "\tData: " + mData.toHex().toUpper() + "\n";
	return string;
}
