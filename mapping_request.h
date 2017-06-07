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

	virtual ~MappingRequest();

	MappingRequestType type() const
	{
		return mType;
	}

	int address() const
	{
		return mAddress;
	}

	int unitId() const
	{
		return mUnitId;
	}

	int quantity() const
	{
		return mQuantity;
	}

	QByteArray &data()
	{
		return mData;
	}

	const QByteArray &data() const
	{
		return mData;
	}

	void setError(MappingErrors error, const QString &errorString);

	MappingErrors error() const
	{
		return mError;
	}

	QString errorString() const
	{
		return mErrorString;
	}

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
