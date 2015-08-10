/*
 * Licensed under Apache License v2. See LICENSE for more information.
 */

#ifndef CELIX_TST_SERVICE_H
#define CELIX_TST_SERVICE_H

#define TST_SERVICE_NAME "tst_service"

struct tst_service {
    void *handle;
    void (*test)(void *handle);
};

typedef struct tst_service *tst_service_pt;

#endif //CELIX_TST_SERVICE_H
