#include "mapping_request.h"

MappingRequest::MappingRequest(MappingRequestType type, int address, int unitId, int quantity):
	mType(type),
	mAddress(address),
	mUnitId(unitId),
	mQuantity(quantity),
	mError(NoError)
{
}

MappingRequestType MappingRequest::type() const
{
	return mType;
}

int MappingRequest::address() const
{
	return mAddress;
}

int MappingRequest::unitId() const
{
	return mUnitId;
}

int MappingRequest::quantity() const
{
	return mQuantity;
}

QByteArray &MappingRequest::data()
{
	return mData;
}

const QByteArray &MappingRequest::data() const
{
	return mData;
}

void MappingRequest::setError(MappingErrors error, const QString &errorString)
{
	Q_ASSERT(error != NoError || errorString.isEmpty());
	mError = error;
	mErrorString = errorString;
}

MappingErrors MappingRequest::error() const
{
	return mError;
}

QString MappingRequest::errorString() const
{
	return mErrorString;
}
