#ifndef MODBUSREQUEST_H
#define MODBUSREQUEST_H

#include <QByteArray>
#include <QString>

enum MappingRequestType {
	ReadValues,
	WriteValues
};

enum MappingErrors {
	NoError,
	QuantityError,
	StartAddressError,
	AddressError,
	UnitIdError,
	ServiceError,
	PermissionError
};

class MappingRequest
{
public:
	MappingRequest(MappingRequestType type, int address, int unitId, int quantity);

	virtual ~MappingRequest() {}

	MappingRequestType type() const;

	int address() const;

	int unitId() const;

	int quantity() const;

	QByteArray &data();

	const QByteArray &data() const;

	void setError(MappingErrors error, const QString &errorString);

	MappingErrors error() const;

	QString errorString() const;

private:
	MappingRequestType mType;
	int mAddress;
	int mUnitId;
	int mQuantity;
	QByteArray mData;
	MappingErrors mError;
	QString mErrorString;
};

#endif // MODBUSREQUEST_H
