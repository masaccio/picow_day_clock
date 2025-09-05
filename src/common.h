
#ifndef _COMMON_H
#define _COMMON_H

#ifdef CLOCK_DEBUG_ENABLED
#define CLOCK_DEBUG(...) printf(__VA_ARGS__)
#else
#define CLOCK_DEBUG(...) ((void)0)
#endif

#endif /* _COMMON_H */
