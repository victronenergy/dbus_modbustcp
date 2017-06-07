#include "mapping_request.h"

MappingRequest::MappingRequest(MappingRequestType type, int address, int unitId, int quantity):
	mType(type),
	mAddress(address),
	mUnitId(unitId),
	mQuantity(quantity),
	mError(NoError)
{
}

MappingRequest::~MappingRequest()
{
}

void MappingRequest::setError(MappingErrors error, const QString &errorString)
{
	Q_ASSERT(error != NoError || errorString.isEmpty());
	mError = error;
	mErrorString = errorString;
}
