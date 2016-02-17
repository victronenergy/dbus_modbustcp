#ifndef BACKENDREQUEST_H
#define BACKENDREQUEST_H

#include "mapping_request.h"

class ADU;

class BackendRequest : public MappingRequest
{
public:
	BackendRequest(ADU *adu, MappingRequestType type, int address, int unitId, int quantity);

	ADU *adu() const;

private:
	ADU *mAdu;
};

#endif // BACKENDREQUEST_H
