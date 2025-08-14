#include <celix_filter.h>
#include <celix_err.h>
#include <cstdint>
#include <cstdlib>
#include <cstring>

int filterParseFuzzOneInput(const uint8_t* data, size_t size) {
    char* buffer = static_cast<char*>(malloc(size + 1));
    if (buffer == nullptr) {
        return 0;
    }
    memcpy(buffer, data, size);
    buffer[size] = '\0';

    celix_filter_t* filter = celix_filter_create(buffer);
    celix_filter_destroy(filter);
    celix_err_resetErrors();

    free(buffer);
    return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) { return filterParseFuzzOneInput(data, size); }
