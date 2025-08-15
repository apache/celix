#include <celix_properties.h>
#include <celix_err.h>
#include <cstdint>
#include <cstdlib>
#include <cstring>

int propertiesParserFuzzOneInput(const uint8_t* data, size_t size) {
    int flags = 0;
    if (size > 0) {
        flags = data[0] & 0x3F; // use 6 bits for decode flags
        data += 1;
        size -= 1;
    }
    char* buffer = static_cast<char*>(malloc(size + 1));
    if (buffer == nullptr) {
        return 0;
    }
    memcpy(buffer, data, size);
    buffer[size] = '\0';

    celix_properties_t* props = nullptr;
    celix_properties_loadFromString(buffer, flags, &props);
    celix_properties_destroy(props);
    celix_err_resetErrors();

    free(buffer);
    return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    return propertiesParserFuzzOneInput(data, size);
}
