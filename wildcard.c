#include "wildcard.h"

#include <errno.h>
#include <string.h>
#include <sys/types.h>

#define ESC '\\'

#define ESCAPE_SUPPORTED ((flags & WILDCARD_NOESCAPE) == 0)

/// Attempt to match word bounded by @c begin and @c end against @c str.
/// Accepts '?' as a single-character wildcard.
/// @return Pointer to position after @c str on success, or NULL if not matched.
static const char *match_word(int flags, const char *str, const char *begin, const char *end)
{
    int escaped = 0;

    while (begin < end) {
        if (ESCAPE_SUPPORTED && !escaped && *begin == ESC) {
            escaped = 1;
            begin++;

        } else if (*begin == *str || (!escaped && *begin == '?' && *str != '\0')) {
            begin++;
            str++;
            escaped = 0;

        } else {
            return NULL;
        }
    }

    return str;
}

/// Attempt to find word bounded by @c begin and @c end in the @c str.
/// @return Pointer to position after @c str on success, or NULL if not found.
static const char *find_word(int flags, const char *str, const char *begin, const char *end)
{
    for (; *str; ++str) {
        const char *found = match_word(flags, str, begin, end);
        if (found) {
            return found;
        }
    }

    return NULL;
}

/// @return Pointer to last @c len characters in @c str, or NULL if string too short.
static const char *string_tail(const char *str, size_t len)
{
    if (strlen(str) >= len) {
        return strchr(str, 0) - len;
    }

    return NULL;
}

int wildcard_match(const char *pattern, const char *text, int flags)
{
    if (!pattern || !text) {
        return -EINVAL;
    }

    // This algorithm breaks the pattern into words separated by wildcard '*' which denotes a sequence of zero, one, or more characters.
    //
    // Example: "ab*cd"
    // 1. Match "ab" in text.
    // 2. Skip until 2 characters remain in text.
    // 3. Match "cd" in text.
    //
    // Example: "ab*cd*"
    // 1. Match "ab" in text.
    // 2. Find "cd" in text.
    // 3. Consume remainder of text.
    //
    // Example: "*ab*cd"
    // 1. Find "ab" in text.
    // 2. Skip until 2 characters remain in text.
    // 3. Match "cd" in text.
    //
    // Example: "*ab*cd*"
    // 1. Find "ab" in text.
    // 2. Find "cd" in text.
    // 3. Consume remainder of text.

    int wild = 0;

    for (;;) {
        if (*pattern == '*') {
            // Treat "**" as "*".
            wild = 1;

            pattern++;
            continue;

        } else if (*pattern == '?') {
            // Treat "*?" as "?*".
            if (*text) {
                text++;

            } else {
                return -ESRCH;
            }

            pattern++;
            continue;
        }

        // Consume fragment from pattern string.
        int escaped = 0;
        size_t fragment_length = 0;

        const char *end = pattern;
        for (; *end; end++) {
            if (escaped) {
                escaped = 0;
                fragment_length++;

            } else if (ESCAPE_SUPPORTED && *end == ESC) {
                escaped = 1;

            } else if (*end == '*') {
                break;

            } else {
                fragment_length++;
            }
        }

        if (wild) {
            wild = 0;

            if (*end) {
                // Search for fragment in text.
                text = find_word(flags, text, pattern, end);
                if (!text) {
                    return -ESRCH;
                }

                pattern = end;
                continue;
            }

            if (end == pattern) {
                // Trailing '*'.
                // Consume any and all remaining text.
                text = strchr(text, 0);
                break;
            }

            // End of pattern reached.
            // Prepare to match final fragment_length characters.
            text = string_tail(text, fragment_length);
            if (!text) {
                return -ESRCH;
            }
        }

        // Match fragment against text.
        text = match_word(flags, text, pattern, end);
        if (!text) {
            return -ESRCH;
        }

        // Advance in pattern.
        pattern = end;

        if (!*pattern) {
            break;
        }
    }

    if (*text) {
        // Text remains: wildcard match failed.
        return -ESRCH;
    }

    return 0;
}
