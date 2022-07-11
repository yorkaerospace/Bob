#include "data.h"

void setFlag(data_t *dataPtr, uint8_t flag) {
	dataPtr->flags |= flag;
}

void unsetFlag(data_t *dataPtr, uint8_t flag) {
	dataPtr->flags &= ~flag;
}
