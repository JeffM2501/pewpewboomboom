#pragma once
#include <string.h>

inline void CopyFixedSizeString(char* destination, const char* source, size_t maxLen)
{
    size_t len = strlen(source);
    if (len >= maxLen - 1)
        len = maxLen - 1;

    strncpy(destination, source, len);
    destination[len] = '\0';
}