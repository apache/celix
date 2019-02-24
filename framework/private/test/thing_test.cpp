#include <stdlib.h>
#include <stdio.h>

#include "gtest.h"
#include "string.h"

extern "C" {
#include "bundle_revision_private.h"
#include "bundle_private.h"
#include "utils.h"
#include "celix_log.h"
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
