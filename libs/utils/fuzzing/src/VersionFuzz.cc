#include <celix_version.h>
#include <celix_err.h>
#include <cstdint>
#include <cstdlib>
#include <cstring>

int versionParseFuzzOneInput(const uint8_t* data, size_t size) {
    char* buffer = static_cast<char*>(malloc(size + 1));
    if (buffer == nullptr) {
        return 0;
    }
    memcpy(buffer, data, size);
    buffer[size] = '\0';

    celix_version_t* version = celix_version_createVersionFromString(buffer);
    celix_version_destroy(version);
    celix_err_resetErrors();

    free(buffer);
    return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) { return versionParseFuzzOneInput(data, size); }
