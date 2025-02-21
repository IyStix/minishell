#ifndef STRING_UTILS_H
#define STRING_UTILS_H

#include <stdlib.h>
#include <string.h>

static inline char *my_strdup(const char *str)
{
    size_t size = strlen(str) + 1;
    char *new_str = malloc(size);
    if (new_str)
        memcpy(new_str, str, size);
    return new_str;
}

static inline char *my_strndup(const char *str, size_t n)
{
    size_t len = 0;
    while (len < n && str[len])
        len++;
    
    char *new_str = malloc(len + 1);
    if (new_str)
    {
        memcpy(new_str, str, len);
        new_str[len] = '\0';
    }
    return new_str;
}

#endif /* STRING_UTILS_H */
