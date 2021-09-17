#include "wildcard.h"

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

/// @return Pointer to buffer containing @c n 'a' characters.
static char *a_n(size_t n)
{
    char *str = (char *)malloc(n + 1);
    memset(str, 'a', n);
    str[n] = '\0';
    return str;
}

/// @return Pointer to buffer containing @c n 'a*' strings.
static char *a_star_n(size_t n)
{
    char *str = (char *)malloc(n * 2 + 1);
    char *p = str;
    for (size_t i = 0; i < n; ++i) {
        *p++ = 'a';
        *p++ = '*';
    }
    *p++ = 0;
    return str;
}

/// @return Pointer to buffer containing @c n 'a*' strings, with a 'b' suffix.
static char *a_star_n_b(size_t n)
{
    char *str = (char *)malloc(n * 2 + 1 + 1);
    char *p = str;
    for (size_t i = 0; i < n; ++i) {
        *p++ = 'a';
        *p++ = '*';
    }
    *p++ = 'b';
    *p++ = 0;
    return str;
}

static void expect_fail(int err, const char *pattern, const char *text, int flags)
{
    assert(-err == wildcard_match(pattern, text, flags));
}

static void test_invalid_pointers(void)
{
    expect_fail(EFAULT, NULL, NULL, 0);
    expect_fail(EFAULT, NULL, "", 0);
    expect_fail(EFAULT, "", NULL, 0);
}

static void test_empty_strings(void)
{
    assert(0 == wildcard_match("", "", 0));
    assert(0 == wildcard_match("*", "", 0));
    expect_fail(ESRCH, "?", "", 0);
    expect_fail(ESRCH, "", "a", 0);
}

static void test_simple_strings(void)
{
    assert(0 == wildcard_match("?", "?", 0));
    assert(0 == wildcard_match("?", "f", 0));
    expect_fail(ESRCH, "?", "fo", 0);
    assert(0 == wildcard_match("??", "fo", 0));
    assert(0 == wildcard_match("?o", "fo", 0));
    expect_fail(ESRCH, "Abc", "abc", 0);
    assert(0 == wildcard_match("abc", "abc", 0));
}

static void test_escaped_wildcards(void)
{
    assert(0 == wildcard_match("\\?", "?", 0));
    expect_fail(ESRCH, "\\?", "x", 0);
    assert(0 == wildcard_match("\\?o", "?o", 0));
    expect_fail(ESRCH, "\\?o", "xo", 0);
}

static void test_single_wildcard(void)
{
    expect_fail(ESRCH, "*abc", "ab", 0);
    assert(0 == wildcard_match("*abc", "abc", 0));
    assert(0 == wildcard_match("*abc", "_abc", 0));
    expect_fail(ESRCH, "*abc", "a_bc", 0);
    assert(0 == wildcard_match("a*bc", "abc", 0));
    assert(0 == wildcard_match("a*bc", "a_bc", 0));
    expect_fail(ESRCH, "a*bc", "ab_c", 0);
    assert(0 == wildcard_match("ab*c", "abc", 0));
    assert(0 == wildcard_match("ab*c", "ab_c", 0));
    expect_fail(ESRCH, "ab*c", "abc_", 0);
    assert(0 == wildcard_match("abc*", "abc", 0));
    assert(0 == wildcard_match("abc*", "abc_", 0));
    expect_fail(ESRCH, "abc*", "ab_c", 0);

    expect_fail(ESRCH, "*B?",   "ABC_B", 0);
    expect_fail(ESRCH, "*B?",   "ABC__D", 0);
    assert(0 == wildcard_match("*B?",   "ABC_BD", 0));
    assert(0 == wildcard_match("*B?_*", "ABC_BD", 0));

    assert(0 == wildcard_match("\\*abc", "*abc", 0));
    expect_fail(ESRCH, "\\*abc", "_abc", 0);
    assert(0 == wildcard_match("*a\\*c", "_a*c", 0));
}

static void test_multiple_wildcard(void)
{
    assert(0 == wildcard_match("a*foo*b", "a__fo_foo__b", 0));
    expect_fail(ESRCH, "a*foo*b", "a__fo_o__b", 0);
    assert(0 == wildcard_match("****?***?**?*?e*jkl", "abcdefghijkl", 0));
    expect_fail(ESRCH, "****?***?**?*?e*jkl", "abcdEfghijkl", 0);
    assert(0 == wildcard_match("a?c?e*jkl*op", "abcdefghijklmnop", 0));
    expect_fail(ESRCH, "a?c?e*jkl*op", "abcdefghiJklmnop", 0);
    assert(0 == wildcard_match("****?***?**?*?", "abcd", 0));
    expect_fail(ESRCH, "****?***?**?*?", "abc", 0);

    assert(0 == wildcard_match("*ab*cd*", "abcd", 0));
    assert(0 == wildcard_match("*ab*cd*", "abcdcd", 0));
    assert(0 == wildcard_match("*ab*cd*", "_abcd", 0));
    assert(0 == wildcard_match("*ab*cd*", "abcd_", 0));
    assert(0 == wildcard_match("*ab*cd*", "_abcd_", 0));
    assert(0 == wildcard_match("*ab*cd*", "ab_cd", 0));
    assert(0 == wildcard_match("*ab*cd*", "_ab_cd_", 0));
    assert(0 == wildcard_match("*ab*cd*", "ab_cd_", 0));
    assert(0 == wildcard_match("*ab*cd*", "_ab_cd_", 0));

    assert(0 == wildcard_match("\\*ab*cd*", "*abcd", 0));
    expect_fail(ESRCH, "\\*ab*cd*", "_abcd", 0);
    assert(0 == wildcard_match("*ab\\*cd*", "_ab*cd", 0));
    expect_fail(ESRCH, "*ab\\*cd*", "_ab_cd", 0);
    assert(0 == wildcard_match("*ab*cd\\*", "abcd*", 0));
    expect_fail(ESRCH, "*ab*cd\\*", "abcd_", 0);

    assert(0 == wildcard_match("\\*ab*cd*", "\\abcd", WILDCARD_NOESCAPE));
    assert(0 == wildcard_match("\\*ab*cd*", "\\_abcd", WILDCARD_NOESCAPE));
    assert(0 == wildcard_match("*ab\\*cd*", "_ab\\cd", WILDCARD_NOESCAPE));
    assert(0 == wildcard_match("*ab\\*cd*", "_ab\\_cd", WILDCARD_NOESCAPE));
    assert(0 == wildcard_match("*ab*cd\\*", "abcd\\", WILDCARD_NOESCAPE));
    assert(0 == wildcard_match("*ab*cd\\*", "abcd\\_", WILDCARD_NOESCAPE));
}

static void test_wikipedia(void)
{
    expect_fail(ESRCH, "da*da*la*", "daaadabadmanda", 0);
    assert(0 == wildcard_match("da*da*da*", "daaadabadmanda", 0));
    assert(0 == wildcard_match("*?", "xx", 0));
}

/// https://research.swtch.com/glob
static void test_russ_cox(void)
{
    char *file = a_n(100);
    size_t n = 10;

    char *pattern = a_star_n_b(n);
    expect_fail(ESRCH, pattern, file, 0);
    free(pattern);

    pattern = a_star_n(n);
    assert(0 == wildcard_match(pattern, file, 0));
    free(pattern);

    free(file);
}

int main(void)
{
    test_invalid_pointers();
    test_empty_strings();
    test_simple_strings();
    test_escaped_wildcards();
    test_single_wildcard();
    test_multiple_wildcard();
    test_wikipedia();
    test_russ_cox();

    return 0;
}
