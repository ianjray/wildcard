#include "wildcard.h"

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

/// @return char* Pointer to buffer containing @c n 'a' characters.
static char *a_n(size_t n)
{
    char *str = (char *)malloc(n + 1);
    memset(str, 'a', n);
    str[n] = '\0';
    return str;
}

/// @return char* Pointer to buffer containing @c n 'a*' strings, with a 'b' suffix.
static char *a_star_n_b(size_t n)
{
    char *str = (char *)malloc(n * 2 + 1 + 1);
    for (size_t i = 0; i < n; ++i) {
        str[i * 2]     = 'a';
        str[i * 2 + 1] = '*';
    }
    str[n * 2]     = 'b';
    str[n * 2 + 1] = '\0';
    return str;
}

int main(void)
{
    int r;

    // Invalid pointers.
    r = wildcard_match(NULL, NULL, 0);
    assert(r == -EINVAL);
    r = wildcard_match(NULL, "", 0);
    assert(r == -EINVAL);
    r = wildcard_match("", NULL, 0);
    assert(r == -EINVAL);

    // Empty strings.
    r = wildcard_match("", "", 0);
    assert(r == 0);
    r = wildcard_match("*", "", 0);
    assert(r == 0);
    r = wildcard_match("?", "", 0);
    assert(r == -ESRCH);
    r = wildcard_match("", "a", 0);
    assert(r == -ESRCH);

    // Simple strings.
    r = wildcard_match("?", "?", 0);
    assert(r == 0);
    r = wildcard_match("?", "f", 0);
    assert(r == 0);
    r = wildcard_match("?", "fo", 0);
    assert(r == -ESRCH);
    r = wildcard_match("??", "fo", 0);
    assert(r == 0);
    r = wildcard_match("?o", "fo", 0);
    assert(r == 0);
    r = wildcard_match("Abc", "abc", 0);
    assert(r == -ESRCH);
    r = wildcard_match("abc", "abc", 0);
    assert(r == 0);

    r = wildcard_match("\\?", "?", 0);
    assert(r == 0);
    r = wildcard_match("\\?", "x", 0);
    assert(r == -ESRCH);
    r = wildcard_match("\\?o", "?o", 0);
    assert(r == 0);
    r = wildcard_match("\\?o", "xo", 0);
    assert(r == -ESRCH);

    // Single wildcard.
    r = wildcard_match("*abc", "ab", 0);
    assert(r == -ESRCH);
    r = wildcard_match("*abc", "abc", 0);
    assert(r == 0);
    r = wildcard_match("*abc", "_abc", 0);
    assert(r == 0);
    r = wildcard_match("*abc", "a_bc", 0);
    assert(r == -ESRCH);
    r = wildcard_match("a*bc", "abc", 0);
    assert(r == 0);
    r = wildcard_match("a*bc", "a_bc", 0);
    assert(r == 0);
    r = wildcard_match("a*bc", "ab_c", 0);
    assert(r == -ESRCH);
    r = wildcard_match("ab*c", "abc", 0);
    assert(r == 0);
    r = wildcard_match("ab*c", "ab_c", 0);
    assert(r == 0);
    r = wildcard_match("ab*c", "abc_", 0);
    assert(r == -ESRCH);
    r = wildcard_match("abc*", "abc", 0);
    assert(r == 0);
    r = wildcard_match("abc*", "abc_", 0);
    assert(r == 0);
    r = wildcard_match("abc*", "ab_c", 0);
    assert(r == -ESRCH);

    r = wildcard_match("*B?",   "ABC_B", 0);
    assert(r == -ESRCH);
    r = wildcard_match("*B?",   "ABC__D", 0);
    assert(r == -ESRCH);
    r = wildcard_match("*B?",   "ABC_BD", 0);
    assert(r == 0);
    r = wildcard_match("*B?_*", "ABC_BD", 0);
    assert(r == 0);

    r = wildcard_match("\\*abc", "*abc", 0);
    assert(r == 0);
    r = wildcard_match("\\*abc", "_abc", 0);
    assert(r == -ESRCH);
    r = wildcard_match("*a\\*c", "_a*c", 0);
    assert(r == 0);

    // Multiple wildcard.
    r = wildcard_match("a*foo*b", "a__fo_foo__b", 0);
    assert(r == 0);
    r = wildcard_match("a*foo*b", "a__fo_o__b", 0);
    assert(r == -ESRCH);
    r = wildcard_match("****?***?**?*?e*jkl", "abcdefghijkl", 0);
    assert(r == 0);
    r = wildcard_match("****?***?**?*?e*jkl", "abcdEfghijkl", 0);
    assert(r == -ESRCH);
    r = wildcard_match("a?c?e*jkl*op", "abcdefghijklmnop", 0);
    assert(r == 0);
    r = wildcard_match("a?c?e*jkl*op", "abcdefghiJklmnop", 0);
    assert(r == -ESRCH);
    r = wildcard_match("****?***?**?*?", "abcd", 0);
    assert(r == 0);
    r = wildcard_match("****?***?**?*?", "abc", 0);
    assert(r == -ESRCH);

    r = wildcard_match("*ab*cd*", "abcd", 0);
    assert(r == 0);
    r = wildcard_match("*ab*cd*", "_abcd", 0);
    assert(r == 0);
    r = wildcard_match("*ab*cd*", "abcd_", 0);
    assert(r == 0);
    r = wildcard_match("*ab*cd*", "_abcd_", 0);
    assert(r == 0);
    r = wildcard_match("*ab*cd*", "ab_cd", 0);
    assert(r == 0);
    r = wildcard_match("*ab*cd*", "_ab_cd_", 0);
    assert(r == 0);
    r = wildcard_match("*ab*cd*", "ab_cd_", 0);
    assert(r == 0);
    r = wildcard_match("*ab*cd*", "_ab_cd_", 0);
    assert(r == 0);

    r = wildcard_match("\\*ab*cd*", "*abcd", 0);
    assert(r == 0);
    r = wildcard_match("\\*ab*cd*", "_abcd", 0);
    assert(r == -ESRCH);
    r = wildcard_match("*ab\\*cd*", "_ab*cd", 0);
    assert(r == 0);
    r = wildcard_match("*ab\\*cd*", "_ab_cd", 0);
    assert(r == -ESRCH);
    r = wildcard_match("*ab*cd\\*", "abcd*", 0);
    assert(r == 0);
    r = wildcard_match("*ab*cd\\*", "abcd_", 0);
    assert(r == -ESRCH);

    r = wildcard_match("\\*ab*cd*", "\\abcd", WILDCARD_NOESCAPE);
    assert(r == 0);
    r = wildcard_match("\\*ab*cd*", "\\_abcd", WILDCARD_NOESCAPE);
    assert(r == 0);
    r = wildcard_match("*ab\\*cd*", "_ab\\cd", WILDCARD_NOESCAPE);
    assert(r == 0);
    r = wildcard_match("*ab\\*cd*", "_ab\\_cd", WILDCARD_NOESCAPE);
    assert(r == 0);
    r = wildcard_match("*ab*cd\\*", "abcd\\", WILDCARD_NOESCAPE);
    assert(r == 0);
    r = wildcard_match("*ab*cd\\*", "abcd\\_", WILDCARD_NOESCAPE);
    assert(r == 0);

    // Wikipedia.
    r = wildcard_match("da*da*la*", "daaadabadmanda", 0);
    assert(r == -ESRCH);
    r = wildcard_match("da*da*da*", "daaadabadmanda", 0);
    assert(r == 0);
    r = wildcard_match("*?", "xx", 0);
    assert(r == 0);

    {
        // https://research.swtch.com/glob
        char *file = a_n(100);
        size_t n = 2;
        char *pattern = a_star_n_b(n);
        r = wildcard_match(pattern, file, 0);
        assert(r == -ESRCH);
        pattern[n * 2] = '\0';
        r = wildcard_match(pattern, file, 0);
        assert(r == 0);
        free(pattern);
        free(file);
    }
}
