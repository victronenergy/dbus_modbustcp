#ifndef CONNECTION_H
#define CONNECTION_H

#include <QTcpSocket>
#include "adu.h"

class Connection: public QObject
{
	Q_OBJECT
public:
	Connection(QTcpSocket *socket, QObject *parent=0);

signals:
	void modbusRequest(ADU *request);

private slots:
	void readyRead();

	void disconnected();

private:
	QByteArray mData;
	QTcpSocket *mSocket;
	int mLength;
};

#endif // CONNECTION_H
