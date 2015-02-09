
#ifndef _SHM_H_
#define _SHM_H_

#include <celix_errno.h>

#define SHM_ENTRY_MAX_KEY_LENGTH	256
#define SHM_ENTRY_MAX_VALUE_LENGTH	256

// defines the time-to-live in seconds
#define SHM_ENTRY_DEFAULT_TTL		60

// we currently support 64 separate discovery instances
#define SHM_DATA_MAX_ENTRIES		64

typedef struct shmData* shmData_pt;

/* creates a new shared memory block */
celix_status_t discovery_shmCreate(shmData_pt* data);
celix_status_t discovery_shmAttach(shmData_pt* data);
celix_status_t discovery_shmSet(shmData_pt data, char *key, char* value);
celix_status_t discovery_shmGet(shmData_pt data, char* key, char* value);
celix_status_t discovery_shmGetKeys(shmData_pt data, char** keys, int* size);
celix_status_t discovery_shmRemove(shmData_pt data, char* key);
celix_status_t discovery_shmDetach(shmData_pt data);
celix_status_t discovery_shmDestroy(shmData_pt data);

#endif
