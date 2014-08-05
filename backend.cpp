#include "backend.h"
//#define QS_LOG_DISABLE
#include "QsLog.h"

Backend::Backend(QObject *parent) :
	QObject(parent)
{
}

void Backend::modbusRequest(ADU * const modbusRequest)
{
	ADU * const reply =  modbusRequest;
	uint functionCode = modbusRequest->getFunctionCode();
	const quint8 unitID = modbusRequest->getUnitID();
	QByteArray & data = modbusRequest->getData();
	QByteArray & replyData = modbusRequest->getReplyDataRef();

	switch(functionCode) {
	case PDU::ReadHoldingRegisters:
	case PDU::ReadInputRegisters:
	{
		int address = modbusRequest->getAddres();
		quint16 quantity = modbusRequest->getQuantity();
		QLOG_TRACE() << "Read registers" << functionCode << "address =" << address << "quantity =" << quantity;

		if (quantity == 0 || quantity > 125) {
			QLOG_ERROR() << "Requested quantity" << quantity << "invalid for function" << functionCode;
			modbusRequest->setExceptionCode(PDU::IllegalDataValue);
		} else {
			emit getValues(address, unitID, quantity, replyData);
			if (replyData.isEmpty()) {
				QLOG_ERROR() << "Non existing address" << address << "init ID" << unitID << "combination or gap(s) in range with quantity" << quantity << "for function" << functionCode;
				modbusRequest->setExceptionCode(PDU::IllegalDataAddress);
			} else {
				replyData.prepend(replyData.size());
			}
		}
		break;
	}
	case PDU::WriteSingleRegister:
	{
		replyData = data;
		int address = modbusRequest->getAddres();
		data.remove(0,2); // Remove header
		QLOG_TRACE() << "PDU::WriteSingleRegister Address = " << address;

		emit setValues(address, unitID, 1, data);
		if (data.isEmpty()) {
			QLOG_ERROR() << "Not existing address" << address << "for function" << functionCode;
			modbusRequest->setExceptionCode(PDU::IllegalDataAddress);
		}
		break;
	}
	case PDU::WriteMultipleRegisters:
	{
		replyData = data;
		int address = modbusRequest->getAddres();
		quint16 quantity = modbusRequest->getQuantity();
		quint8 byteCount = modbusRequest->getByteCount();
		data.remove(0,5); // Remove header
		QLOG_TRACE() << "Write multiple registers" << functionCode << "address =" << address << "quantity =" << quantity;

		if ( (quantity == 0 || quantity > 125) && (byteCount != (quantity*2)) ) {
			QLOG_ERROR() << "Requested quantity" << quantity << "or byte count" << byteCount << "invalid for function" << functionCode;
			modbusRequest->setExceptionCode(PDU::IllegalDataValue);
		} else {
			emit setValues(address, unitID, quantity, data);
			if (data.isEmpty()) {
				QLOG_ERROR() << "Not existing address" << address << "or gap(s) in range with quantity" << quantity << "for function" << functionCode;
				modbusRequest->setExceptionCode(PDU::IllegalDataAddress);
			} else
				replyData.truncate(4); // Remove byte count and data
		}
		break;
	}
	default:
		QLOG_ERROR() << "Unknown function " << functionCode;
		modbusRequest->setExceptionCode(PDU::IllegalFunction);
		break;
	}
	emit modbusReply(reply);
}
