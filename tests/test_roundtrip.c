/* test_roundtrip.c - Highlight → strip ANSI → matches input */

#include "flare_testkit.h"
#include <ditty/highlight.h>
#include <stdlib.h>
#include <string.h>

static char *strip_ansi(const char *s)
{
    size_t len = strlen(s);
    char *out = malloc(len + 1);
    size_t j = 0;
    for (size_t i = 0; s[i];) {
        if (s[i] == '\033' && s[i + 1] == '[') {
            i += 2;
            while (s[i] && s[i] != 'm')
                i++;
            if (s[i] == 'm')
                i++;
        } else {
            out[j++] = s[i++];
        }
    }
    out[j] = '\0';
    return out;
}

static void test_roundtrip_simple(void)
{
    const char *src = "(+ 1 2)";
    FlareResult r = flare_highlight(src, strlen(src), NULL, NULL, NULL);
    char *plain = strip_ansi(r.data);
    ASSERT_STR_EQ(plain, src);
    free(plain);
    flare_result_free(r);
}

static void test_roundtrip_define_lambda(void)
{
    const char *src = "(define (add a b) (+ a b))";
    FlareResult r = flare_highlight(src, strlen(src), NULL, NULL, NULL);
    char *plain = strip_ansi(r.data);
    ASSERT_STR_EQ(plain, src);
    free(plain);
    flare_result_free(r);
}

static void test_roundtrip_quasiquote(void)
{
    const char *src = "`(list ,a ,@bs)";
    FlareResult r = flare_highlight(src, strlen(src), NULL, NULL, NULL);
    char *plain = strip_ansi(r.data);
    ASSERT_STR_EQ(plain, src);
    free(plain);
    flare_result_free(r);
}

static void test_roundtrip_comment_and_string(void)
{
    const char *src = "; comment\n\"hello world\"";
    FlareResult r = flare_highlight(src, strlen(src), NULL, NULL, NULL);
    char *plain = strip_ansi(r.data);
    ASSERT_STR_EQ(plain, src);
    free(plain);
    flare_result_free(r);
}

int main(void)
{
    RUN_TEST(test_roundtrip_simple);
    RUN_TEST(test_roundtrip_define_lambda);
    RUN_TEST(test_roundtrip_quasiquote);
    RUN_TEST(test_roundtrip_comment_and_string);
    TEST_SUMMARY();
}
