#include "backend_request.h"

BackendRequest::BackendRequest(ADU *adu, MappingRequestType type, int address,
							   int unitId, int quantity):
	MappingRequest(type, address, unitId, quantity),
	mAdu(adu)
{
}

ADU *BackendRequest::adu() const
{
	return mAdu;
}
