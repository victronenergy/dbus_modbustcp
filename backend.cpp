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
	QString errorMessage = "Error processing function code "+QString::number(functionCode)+", unit id "+QString::number(unitID);
	Mappings::MappingErrors error;

	switch(functionCode) {
	case PDU::ReadHoldingRegisters:
	case PDU::ReadInputRegisters:
	{
		int address = modbusRequest->getAddres();
		quint16 quantity = modbusRequest->getQuantity();
		errorMessage += ", start address "+QString::number(address)+", quantity "+QString::number(quantity)+" :";
		QLOG_TRACE() << "Read registers" << functionCode << "address =" << address << "quantity =" << quantity;
		if (quantity == 0 || quantity > 125) {
			QLOG_ERROR() << errorMessage << "Requested quantity invalid for this function";
			modbusRequest->setExceptionCode(PDU::IllegalDataValue);
		} else {
			emit getValues(address, unitID, quantity, replyData, error);
			if (error == Mappings::NoError)
				replyData.prepend(replyData.size());
			else
				QLOG_ERROR() << errorMessage << handleError(error, modbusRequest);
		}
		break;
	}
	case PDU::WriteSingleRegister:
	{
		replyData = data;
		int address = modbusRequest->getAddres();
		errorMessage += ", address "+QString::number(address)+" :";
		data.remove(0,2); // Remove header
		QLOG_TRACE() << "PDU::WriteSingleRegister Address = " << address;

		emit setValues(address, unitID, 1, data, error);
		if (error != Mappings::NoError) {
			QLOG_ERROR() << errorMessage << handleError(error, modbusRequest);
		}
		break;
	}
	case PDU::WriteMultipleRegisters:
	{
		replyData = data;
		int address = modbusRequest->getAddres();
		quint16 quantity = modbusRequest->getQuantity();
		quint8 byteCount = modbusRequest->getByteCount();
		errorMessage += ", start address "+QString::number(address)+", quantity "+QString::number(quantity)+", byte count "+QString::number(byteCount)+" :";
		data.remove(0,5); // Remove header
		QLOG_TRACE() << "Write multiple registers" << functionCode << "address =" << address << "quantity =" << quantity;

		if ( (quantity == 0 || quantity > 125) && (byteCount != (quantity*2)) ) {
			QLOG_ERROR() << errorMessage << "Requested quantity invalid for this function";
			modbusRequest->setExceptionCode(PDU::IllegalDataValue);
		} else {
			emit setValues(address, unitID, quantity, data, error);
			if (error == Mappings::NoError)
				replyData.truncate(4); // Remove byte count and data
			else
				QLOG_ERROR() << errorMessage << handleError(error, modbusRequest);
		}
		break;
	}
	default:
		QLOG_ERROR() << errorMessage << ": Illegal function";
		modbusRequest->setExceptionCode(PDU::IllegalFunction);
		break;
	}
	emit modbusReply(reply);
}

QString Backend::handleError(Mappings::MappingErrors &error, ADU * const modbusRequest)
{
	switch (error) {
	case Mappings::NoError:
		break;
	case Mappings::StartAddressError:
		modbusRequest->setExceptionCode(PDU::IllegalDataAddress);
		return "Unit id is available, but start address does not exist";
	case Mappings::AddressError:
		modbusRequest->setExceptionCode(PDU::IllegalDataAddress);
		return "Unit id is available, but some of the addresses do not exist";
	case Mappings::QuantityError:
		modbusRequest->setExceptionCode(PDU::IllegalDataValue);
		return "Requested quantity invalid for this function";
	case Mappings::UnitIdError:
		modbusRequest->setExceptionCode(PDU::GatewayTargetDeviceFailedToRespond);
		return "Unit id not found";
	case Mappings::ServiceError:
		modbusRequest->setExceptionCode(PDU::GatewayPathUnavailable);
		return "Requested device (service) does not exists";
	case Mappings::PermissionError:
		modbusRequest->setExceptionCode(PDU::IllegalDataAddress);
		return "Address not writable";
	}
	return "Pitfall for error code handling:" + QString::number(error);
}
