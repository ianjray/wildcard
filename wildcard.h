#ifndef LIBWILDCARD_WILDCARD_H_
#define LIBWILDCARD_WILDCARD_H_

/// When set, backslashes are treated as literal characters, not escape sequences.
#define WILDCARD_NOESCAPE (1 << 0)

/// Match text against a wildcard pattern (case-sensitive).
/// @param pattern Wildcard pattern.
///                '?' matches single character.
///                '*' matches zero or more characters.
/// @param text    Text to match.
/// @param flags   WILDCARD_NOESCAPE disables backslash escaping.
/// @return 0 on success.
/// @return -EINVAL if any arguments are NULL.
/// @return -ESRCH if @c text does not @c match pattern.
int wildcard_match(const char *pattern, const char *text, int flags);

#endif // LIBWILDCARD_WILDCARD_H_
