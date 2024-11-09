/**
 * vector
 * CS 341 - Fall 2024
 */
#include "sstring.h"
#include "vector.h"

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <assert.h>
#include <string.h>
#include <unistd.h>

struct sstring
{
    char *str;
};

vector *string_vector_create();

void sstring_destroy(sstring *this)
{
    assert(this);
    free(this->str);
    free(this);
}

sstring *cstr_to_sstring(const char *input)
{
    assert(input);

    struct sstring *str = malloc(sizeof(sstring));
    str->str = strdup(input);
    return str;
}

char *sstring_to_cstr(sstring *input)
{
    assert(input);
    assert(input->str);

    return strdup(input->str);
}

size_t sstring_length(const sstring *str)
{
    assert(str);
    assert(str->str);

    return strlen(str->str);
}

int sstring_append(sstring *this, sstring *addition)
{
    assert(this);
    assert(this->str);
    assert(addition);
    assert(addition->str);

    size_t buffer_size = sstring_length(this) + sstring_length(addition) + 1;
    char *new_str = realloc(this->str, buffer_size);
    assert(new_str);

    this->str = new_str;
    strcat(this->str, addition->str);

    return (int)buffer_size - 1;
}

vector *sstring_split(sstring *this, char delimiter)
{
    assert(this);
    assert(this->str);

    vector *ret = string_vector_create();

    size_t start = 0;
    size_t len = sstring_length(this);
    while (start <= len)
    {
        size_t end;
        char *next = strchr(this->str + start, delimiter);
        if (next == NULL)
            end = len;
        else
            end = next - this->str;

        char *slice = sstring_slice(this, (int)start, (int)end);
        vector_push_back(ret, slice);
        free(slice);

        // + 1 to skips the delimiter
        start = end + 1;
    }

    return ret;
}

int sstring_substitute(sstring *this, size_t offset, char *target, char *substitution)
{
    assert(this);
    assert(target);
    assert(substitution);
    assert(offset < sstring_length(this));

    // find the substring
    char *substr = strstr(this->str + offset, target);
    size_t pos = (size_t)(substr - this->str);
    if (substr == NULL)
        return -1;

    // allocate more memory if needed
    size_t target_len = strlen(target);
    size_t substi_len = strlen(substitution);
    size_t this_len = strlen(this->str);
    if (substi_len > target_len)
    {
        char *new_str = realloc(this->str, this_len + substi_len - target_len + 1);
        assert(new_str);
        this->str = new_str;
    }

    // shift back (or up) everything past the target
    memmove(this->str + pos + substi_len, this->str + pos + target_len, this_len - pos - target_len);

    // replace the string
    memcpy(this->str + pos, substitution, substi_len);

    // set the correct null-byte
    this->str[this_len + substi_len - target_len] = '\0';

    return 0;
}

char *sstring_slice(sstring *this, int start, int end)
{
    assert(this);
    assert(0 <= start && start <= end && end <= (int)sstring_length(this));

    char *buffer = malloc(end - start + 1);
    memcpy(buffer, this->str + start, end - start);
    buffer[end - start] = '\0';

    return buffer;
}
