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
    r = wildcard_match(NULL, NULL);
    assert(r == -EINVAL);
    r = wildcard_match(NULL, "");
    assert(r == -EINVAL);
    r = wildcard_match("", NULL);
    assert(r == -EINVAL);

    // Empty strings.
    r = wildcard_match("", "");
    assert(r == 0);
    r = wildcard_match("*", "");
    assert(r == 0);
    r = wildcard_match("?", "");
    assert(r == -ESRCH);
    r = wildcard_match("", "a");
    assert(r == -ESRCH);

    // Simple strings.
    r = wildcard_match("?", "?");
    assert(r == 0);
    r = wildcard_match("?", "f");
    assert(r == 0);
    r = wildcard_match("?", "fo");
    assert(r == -ESRCH);
    r = wildcard_match("??", "fo");
    assert(r == 0);
    r = wildcard_match("?o", "fo");
    assert(r == 0);
    r = wildcard_match("Abc", "abc");
    assert(r == -ESRCH);
    r = wildcard_match("abc", "abc");
    assert(r == 0);

    // Single wildcard.
    r = wildcard_match("*abc", "ab");
    assert(r == -ESRCH);
    r = wildcard_match("*abc", "abc");
    assert(r == 0);
    r = wildcard_match("*abc", "_abc");
    assert(r == 0);
    r = wildcard_match("*abc", "a_bc");
    assert(r == -ESRCH);
    r = wildcard_match("a*bc", "abc");
    assert(r == 0);
    r = wildcard_match("a*bc", "a_bc");
    assert(r == 0);
    r = wildcard_match("a*bc", "ab_c");
    assert(r == -ESRCH);
    r = wildcard_match("ab*c", "abc");
    assert(r == 0);
    r = wildcard_match("ab*c", "ab_c");
    assert(r == 0);
    r = wildcard_match("ab*c", "abc_");
    assert(r == -ESRCH);
    r = wildcard_match("abc*", "abc");
    assert(r == 0);
    r = wildcard_match("abc*", "abc_");
    assert(r == 0);
    r = wildcard_match("abc*", "ab_c");
    assert(r == -ESRCH);

    r = wildcard_match("*B?",   "ABC_B");
    assert(r == -ESRCH);
    r = wildcard_match("*B?",   "ABC__D");
    assert(r == -ESRCH);
    r = wildcard_match("*B?",   "ABC_BD");
    assert(r == 0);
    r = wildcard_match("*B?_*", "ABC_BD");
    assert(r == 0);

    // Multiple wildcard.
    r = wildcard_match("a*foo*b", "a__fo_foo__b");
    assert(r == 0);
    r = wildcard_match("a*foo*b", "a__fo_o__b");
    assert(r == -ESRCH);
    r = wildcard_match("****?***?**?*?e*jkl", "abcdefghijkl");
    assert(r == 0);
    r = wildcard_match("****?***?**?*?e*jkl", "abcdEfghijkl");
    assert(r == -ESRCH);
    r = wildcard_match("a?c?e*jkl*op", "abcdefghijklmnop");
    assert(r == 0);
    r = wildcard_match("a?c?e*jkl*op", "abcdefghiJklmnop");
    assert(r == -ESRCH);
    r = wildcard_match("****?***?**?*?", "abcd");
    assert(r == 0);
    r = wildcard_match("****?***?**?*?", "abc");
    assert(r == -ESRCH);

    // Wikipedia.
    r = wildcard_match("da*da*la*", "daaadabadmanda");
    assert(r == -ESRCH);
    r = wildcard_match("da*da*da*", "daaadabadmanda");
    assert(r == 0);
    r = wildcard_match("*?", "xx");
    assert(r == 0);

    {
        // https://research.swtch.com/glob
        char *file = a_n(100);
        size_t n = 2;
        char *pattern = a_star_n_b(n);
        r = wildcard_match(pattern, file);
        assert(r == -ESRCH);
        pattern[n * 2] = '\0';
        r = wildcard_match(pattern, file);
        assert(r == 0);
        free(pattern);
        free(file);
    }
}
