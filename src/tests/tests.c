#include <assert.h>

#include "test.h"

test_config_t test_config = {
    .cyw43_arch_init_result = 0,
};

int test_bad_cy43_init(void)
{
    test_config.cyw43_arch_init_result = 1;
    return test_main();
}

int main(void)
{
    assert(test_bad_cy43_init() == 1);

    return 0;
}
