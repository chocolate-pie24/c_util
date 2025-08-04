#include "include/test_core_string.h"
#include "include/test_dynamic_array.h"

#include "../../core/include/message.h"

int main(void) {
    INFO_MESSAGE("[TEST] core_string_t: started");
    test_core_string();
    INFO_MESSAGE("[TEST] core_string_t: success");

    INFO_MESSAGE("[TEST] dynamic_array_t: started");
    test_dynamic_array();
    INFO_MESSAGE("[TEST] dynamic_array_t: success");

    return 0;
}
