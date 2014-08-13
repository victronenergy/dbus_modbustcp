#ifndef BACKEND_H
#define BACKEND_H

#include <QObject>
#include <QTcpSocket>

#include "adu.h"
#include "mappings.h"

class Backend : public QObject
{
	Q_OBJECT
public:
	explicit Backend(QObject *parent = 0);

public slots:
	void modbusRequest(ADU * const request);

signals:
	void getValues(const int modbusAddress, const int unitID, const int quantity, QByteArray &replyData, Mappings::MappingErrors &error);
	void setValues(const int modbusAddress, const int unitID, const int quantity, QByteArray &data, Mappings::MappingErrors &error);
	void modbusReply(ADU * const reply);

private:
	QString handleError(Mappings::MappingErrors &error, ADU * const modbusRequest);
};

#endif // BACKEND_H
