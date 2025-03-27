#include "wildcard.h"

#include <errno.h>
#include <string.h>
#include <sys/types.h>

#define ESC '\\'

#define ESCAPE_SUPPORTED ((flags & WILDCARD_NOESCAPE) == 0)

/// @return pointer To last @c len characters in @c str, or NULL if string too short.
static const char *string_tail(const char *str, size_t len)
{
    if (strlen(str) >= len) {
        return strchr(str, 0) - len;
    }
    return NULL;
}

/// Attempt to match word bounded by @c begin and @c end against @c str.
/// @return int Number of matched characters, or negative errno otherwise.
static ssize_t match_word(int flags, const char *str, const char *begin, const char *end)
{
    const char *p = begin;
    int escaped = 0;
    ssize_t n = 0;

    while (p < end) {
        if (ESCAPE_SUPPORTED && !escaped && *p == ESC) {
            escaped = 1;
            p++;

        } else if (*p == *str || (!escaped && *p == '?' && *str)) {
            p++;
            str++;
            n++;
            escaped = 0;

        } else {
            return -ESRCH;
        }
    }

    return n;
}

/// Attempt to find word bounded by @c begin and @c end in the @c str.
/// @return pointer To found word, or NULL otherwise.
static const char *find_word(int flags, const char *str, const char *begin, const char *end)
{
    if (str) {
        for (; *str; ++str) {
            if (match_word(flags, str, begin, end) >= 0) {
                return str;
            }
        }
    }
    return NULL;
}

int wildcard_match(const char *pattern, const char *text, int flags)
{
    const char *p = pattern;
    const char *begin;
    int escaped = 0;
    ssize_t n;

    if (!pattern || !text) {
        return -EINVAL;
    }

    while (*p) {
        if (*p == '*') {
            p++;

            // Handle possible sequence of '*', optionally containing '?'.
            for (n = 0; ; ) {
                if (*p == '*') {
                    // Rewrite "**" as "*".
                    p++;
                } else if (*p == '?') {
                    // Rewrite "*?" as "?*".
                    p++;
                    n++;
                } else {
                    break;
                }
            }

            // Consume acccumulated '?'.
            if (n) {
                if (strlen(text) < (size_t)n) {
                    return -ESRCH;
                }

                text += n;
            }

            // Consume word from pattern string.
            // (A word is a sequence of characters optionally including '?'.)
            n = 0;
            for (begin = p, escaped = 0; *p; p++) {
                if (escaped) {
                    escaped = 0;
                    n++;

                } else if (ESCAPE_SUPPORTED && *p == ESC) {
                    escaped = 1;

                } else if (*p == '*') {
                    break;

                } else {
                    n++;
                }
            }

            if (*p) {
                // Pattern continues.  Search for word in text.
                text = find_word(flags, text, begin, p);
                if (!text) {
                    return -ESRCH;
                }

            } else if (begin == p) {
                // Trailing '*'.  Consume any and all remaining text.
                text = strchr(text, 0);

            } else {
                // End of pattern reached.  Match word against tail of text.
                text = string_tail(text, (size_t)n);
                if (!text) {
                    return -ESRCH;
                }

                n = match_word(flags, text, begin, p);
                if (n < 0) {
                    return (int)n;
                }

                // Advance past "word" in text.
                text += n;
            }

        } else {
            // Consume word from pattern string.
            for (begin = p, escaped = 0; *p; p++) {
                if (escaped) {
                    escaped = 0;

                } else if (ESCAPE_SUPPORTED && *p == ESC) {
                    escaped = 1;

                } else if (*p == '*') {
                    break;
                }
            }

            n = match_word(flags, text, begin, p);
            if (n < 0) {
                return (int)n;
            }

            // Advance past "word" in text.
            text += n;
        }
    }

    if (*text) {
        // Text remains.  Wildcard match failed.
        return -ESRCH;
    }

    return 0;
}
