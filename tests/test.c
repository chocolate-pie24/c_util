#include "include/test_core_string.h"

#include "../../core/include/message.h"

int main(void) {
    INFO_MESSAGE("[TEST] core_string_t: started");
    test_core_string();
    INFO_MESSAGE("[TEST] core_string_t: success");

    return 0;
}
