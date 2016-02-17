#ifndef BACKEND_H
#define BACKEND_H

#include <QObject>
#include "mapping_request.h"
#include "pdu.h"

class ADU;
class MappingRequest;

class Backend : public QObject
{
	Q_OBJECT
public:
	explicit Backend(QObject *parent = 0);

public slots:
	void modbusRequest(ADU *request);
	void requestCompleted(MappingRequest *request);

signals:
	void mappingRequest(MappingRequest *request);
	void modbusReply(ADU *reply);

private:
	void logError(const QString &message, ADU *request);
	static PDU::ExceptionCode getExceptionCode(MappingErrors error);
};

#endif // BACKEND_H
