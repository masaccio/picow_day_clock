#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

#include "test.h"

typedef int (*test_func_t)(void);

static void run_test(test_func_t func, const char *msg, bool expect_fail)
{
    int status = func();
    if (status == 1 && expect_fail) {
        printf("%s: OK (expected failure)\n", msg);
    } else if (status == 0 && expect_fail) {
        printf("%s: FAIL (unexpected success)\n", msg);
    } else if (status == 1) {
        printf("%s: FAIL\n", msg);
    } else {
        printf("%s: OK\n", msg);
    }
}

test_config_t test_config = {
    .cyw43_arch_init_result = 0,
};

int test_bad_cy43_init(void)
{
    test_config.cyw43_arch_init_result = 1;
    return test_main() == 1;
}

int main(void)
{
    run_test(test_bad_cy43_init, "cyw43_arch_init failure propagates", true);
    return 0;
}
