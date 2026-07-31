#include "adu.h"

//#define QS_LOG_DISABLE
#include "QsLog.h"

QByteArray ADU::buildPduReply() const
{
	quint8 functionCode = getFunctionCode();
	ExceptionCode exeptionCode = getExceptionCode();

	QByteArray pdu;
	pdu.append(static_cast<char>(functionCode));

	if (exeptionCode == NoExeption) {
		switch(functionCode) {
		case ReadHoldingRegisters:
		case ReadInputRegisters:
			pdu.append(static_cast<char>(mReplyData.size()));
			pdu.append(mReplyData);
			break;
		case WriteSingleRegister:
			appendUInt16(pdu, getAddres());
			pdu.append(mReplyData);
			break;
		case WriteMultipleRegisters:
			appendUInt16(pdu, getAddres());
			pdu.append(static_cast<char>(0));
			pdu.append(static_cast<char>(getByteCount() / 2));
			break;
		default:
			break;
		}
	} else {
		pdu.append(exeptionCode);
	}
	return pdu;
}

QString ADU::aduToString() const
{
	QString string;

	string += "\n\tUnit Identifier: " + QString::number(mUnitID,16).toUpper() + "\n";
	string += pduToString();
	return string;
}
