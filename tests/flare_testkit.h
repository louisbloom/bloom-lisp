/* testkit.h - Reusable C test infrastructure for bloom-flare
 *
 * Provides RUN_TEST / TEST_SUMMARY macros and assertion helpers,
 * following the pattern from bloom-telnet's testkit.h.
 */

#ifndef BLOOM_FLARE_TESTKIT_H
#define BLOOM_FLARE_TESTKIT_H

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int testkit_tests_run = 0;
static int testkit_tests_passed = 0;

#define RUN_TEST(fn)                 \
    do {                             \
        testkit_tests_run++;         \
        fn();                        \
        testkit_tests_passed++;      \
        printf("  PASS: %s\n", #fn); \
    } while (0)

#define TEST_SUMMARY()                                              \
    do {                                                            \
        printf("\n%d/%d tests passed.\n", testkit_tests_passed,     \
               testkit_tests_run);                                  \
        return (testkit_tests_passed == testkit_tests_run) ? 0 : 1; \
    } while (0)

#define ASSERT_EQ(a, b)                                  \
    do {                                                 \
        if ((a) != (b)) {                                \
            fprintf(stderr, "  FAIL: %s:%d: %s != %s\n", \
                    __FILE__, __LINE__, #a, #b);         \
            abort();                                     \
        }                                                \
    } while (0)

#define ASSERT_STR_EQ(a, b)                                      \
    do {                                                         \
        if (strcmp((a), (b)) != 0) {                             \
            fprintf(stderr, "  FAIL: %s:%d: \"%s\" != \"%s\"\n", \
                    __FILE__, __LINE__, (a), (b));               \
            abort();                                             \
        }                                                        \
    } while (0)

#define ASSERT_MEM_EQ(a, b, n)                                  \
    do {                                                        \
        if (memcmp((a), (b), (n)) != 0) {                       \
            fprintf(stderr, "  FAIL: %s:%d: memory mismatch\n", \
                    __FILE__, __LINE__);                        \
            abort();                                            \
        }                                                       \
    } while (0)

#define ASSERT_TRUE(cond)                          \
    do {                                           \
        if (!(cond)) {                             \
            fprintf(stderr, "  FAIL: %s:%d: %s\n", \
                    __FILE__, __LINE__, #cond);    \
            abort();                               \
        }                                          \
    } while (0)

#define ASSERT_NULL(p)     ASSERT_TRUE((p) == NULL)
#define ASSERT_NOT_NULL(p) ASSERT_TRUE((p) != NULL)

#endif /* BLOOM_FLARE_TESTKIT_H */
