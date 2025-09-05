#ifndef _TEST_H
#define _TEST_H

typedef struct
{
    int cyw43_arch_init_result;
} test_config_t;

extern int test_main(void);
extern test_config_t test_config;

#endif /* _TEST_H */
