#ifndef WILDCARD__H
#define WILDCARD__H

/// Set this flag to disable backslash escaping.
#define WILDCARD_NOESCAPE (1 << 0)

/// Match string to pattern
/// @discussion Attempts to match string @c text with @c pattern.  The pattern
/// may contain wildcards: ? matches any single character and * matches zero,
/// one, or more characters.
/// @return int Zero on success or negative errno.
int wildcard_match(const char *pattern, const char *text, int flags);

#endif
