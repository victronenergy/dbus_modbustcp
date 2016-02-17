#include <QCoreApplication>
#include <QHostAddress>
#include <QTcpSocket>
#include "adu.h"
#include "backend.h"
#include "backend_request.h"
#include "QsLog.h"

Backend::Backend(QObject *parent) :
	QObject(parent)
{
}

void Backend::modbusRequest(ADU * const modbusRequest)
{
	uint functionCode = modbusRequest->getFunctionCode();
	quint16 address = modbusRequest->getAddres();
	uint unitID = modbusRequest->getUnitID();
	switch(functionCode) {
	case PDU::ReadHoldingRegisters:
	case PDU::ReadInputRegisters:
	{
		quint16 quantity = modbusRequest->getQuantity();
		QLOG_TRACE() << "Read registers" << functionCode << "address =" << address << "quantity =" << quantity;
		if (quantity == 0 || quantity > 125) {
			logError("Requested quantity invalid for this function", modbusRequest);
			modbusRequest->setExceptionCode(PDU::IllegalDataValue);
			emit modbusReply(modbusRequest);
		} else {
			BackendRequest *r = new BackendRequest(modbusRequest, ReadValues, address, unitID, quantity);
			emit mappingRequest(r);
		}
		break;
	}
	case PDU::WriteSingleRegister:
	{
		QLOG_TRACE() << "PDU::WriteSingleRegister Address = " << address;
		BackendRequest *r = new BackendRequest(modbusRequest, WriteValues, address, unitID, 1);
		r->data() = modbusRequest->getData();
		r->data().remove(0, 2); // Remove header
		emit mappingRequest(r);
		break;
	}
	case PDU::WriteMultipleRegisters:
	{
		quint16 quantity = modbusRequest->getQuantity();
		quint8 byteCount = modbusRequest->getByteCount();
		QLOG_TRACE() << "Write multiple registers" << functionCode << "address =" << address << "quantity =" << quantity;

		if ((quantity == 0 || quantity > 125) || (byteCount != (quantity * 2))) {
			logError("Requested quantity invalid for this function", modbusRequest);
			modbusRequest->setExceptionCode(PDU::IllegalDataValue);
			emit modbusReply(modbusRequest);
		} else {
			BackendRequest *r = new BackendRequest(modbusRequest, WriteValues, address, unitID, quantity);
			r->data() = modbusRequest->getData();
			r->data().remove(0, 5); // Remove header
			emit mappingRequest(r);
		}
		break;
	}
	default:
		logError("Illegal function", modbusRequest);
		modbusRequest->setExceptionCode(PDU::IllegalFunction);
		emit modbusReply(modbusRequest);
		break;
	}
}

void Backend::requestCompleted(MappingRequest *request)
{
	ADU *reply = static_cast<BackendRequest *>(request)->adu();
	QByteArray &replyData = reply->getReplyDataRef();
	if (request->error() == NoError) {
		uint functionCode = reply->getFunctionCode();
		switch(functionCode) {
		case PDU::ReadHoldingRegisters:
		case PDU::ReadInputRegisters:
			replyData.clear();
			replyData.append(static_cast<char>(request->data().size()));
			replyData.append(request->data());
			break;
		case PDU::WriteSingleRegister:
			replyData.clear();
			replyData.append(static_cast<char>(request->address() >> 8));
			replyData.append(static_cast<char>(request->address()));
			replyData.append(request->data());
			break;
		case PDU::WriteMultipleRegisters:
			replyData.clear();
			replyData.append(static_cast<char>(request->address() >> 8));
			replyData.append(static_cast<char>(request->address()));
			replyData.append(static_cast<char>(request->data().size() >> 8));
			replyData.append(static_cast<char>(request->data().size()));
			break;
		default:
			break;
		}
	} else {
		logError(request->errorString(), reply);
		reply->setExceptionCode(getExceptionCode(request->error()));
	}
	emit modbusReply(reply);
	delete request;
}

void Backend::logError(const QString &message, ADU *request)
{
	QString errorMessage = QString("Error processing function code %1, unit id %2, start address %3, quantity %4, src %5: %6").
			arg(request->getFunctionCode()).
			arg(request->getUnitID()).
			arg(request->getAddres()).
			arg(request->getQuantity()).
			arg(request->getSocket()->peerAddress().toString()).
			arg(message);
	QLOG_ERROR() << errorMessage;
}

PDU::ExceptionCode Backend::getExceptionCode(MappingErrors error)
{
	switch (error) {
	case NoError:
		return PDU::NoExeption;
	case StartAddressError:
		return PDU::IllegalDataAddress;
	case AddressError:
		return PDU::IllegalDataAddress;
	case QuantityError:
		return PDU::IllegalDataValue;
	case UnitIdError:
		return PDU::GatewayTargetDeviceFailedToRespond;
	case ServiceError:
		return PDU::GatewayPathUnavailable;
	case PermissionError:
		return PDU::IllegalDataAddress;
	default:
		QLOG_ERROR() << "Pitfall for error code handling:" + QString::number(error);
		return PDU::IllegalDataAddress;
	}
}
