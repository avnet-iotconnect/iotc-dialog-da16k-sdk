/*
  Copyright (c) 2009-2017 Dave Gamble and _cJSON contributors

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.
*/

/* disable warnings about old C89 functions in MSVC */
#if !defined(_CRT_SECURE_NO_DEPRECATE) && defined(_MSC_VER)
#define _CRT_SECURE_NO_DEPRECATE
#endif

#ifdef __GNUCC__
#pragma GCC visibility push(default)
#endif
#if defined(_MSC_VER)
#pragma warning (push)
/* disable warning about single line comments in system headers */
#pragma warning (disable : 4001)
#endif

#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <math.h>
#include <float.h>
#include <math.h>

#if defined(_MSC_VER)
#pragma warning (pop)
#endif
#ifdef __GNUCC__
#pragma GCC visibility pop
#endif

#ifdef JSON_PREFIX
#undef JSON_PREFIX
#include "_cJSON_Utils.h"
#define JSON_PREFIX 1
#else
#include "_cJSON_Utils.h"
#endif

/* define our own boolean type */
#ifdef true
#undef true
#endif
#define true ((_cJSON_bool)1)

#ifdef false
#undef false
#endif
#define false ((_cJSON_bool)0)

static unsigned char* _cJSONUtils_strdup(const unsigned char* const string)
{
    size_t length = 0;
    unsigned char *copy = NULL;

    length = strlen((const char*)string) + sizeof("");
    copy = (unsigned char*) _cJSON_malloc(length);
    if (copy == NULL)
    {
        return NULL;
    }
    memcpy(copy, string, length);

    return copy;
}

/* string comparison which doesn't consider NULL pointers equal */
static int compare_strings(const unsigned char *string1, const unsigned char *string2, const _cJSON_bool case_sensitive)
{
    if ((string1 == NULL) || (string2 == NULL))
    {
        return 1;
    }

    if (string1 == string2)
    {
        return 0;
    }

    if (case_sensitive)
    {
        return strcmp((const char*)string1, (const char*)string2);
    }

    for(; tolower(*string1) == tolower(*string2); (void)string1++, string2++)
    {
        if (*string1 == '\0')
        {
            return 0;
        }
    }

    return tolower(*string1) - tolower(*string2);
}

/* securely comparison of floating-point variables */
static _cJSON_bool compare_double(double a, double b)
{
    double maxVal = fabs(a) > fabs(b) ? fabs(a) : fabs(b);
    return (fabs(a - b) <= maxVal * DBL_EPSILON);
}


/* Compare the next path element of two JSON pointers, two NULL pointers are considered unequal: */
static _cJSON_bool compare_pointers(const unsigned char *name, const unsigned char *pointer, const _cJSON_bool case_sensitive)
{
    if ((name == NULL) || (pointer == NULL))
    {
        return false;
    }

    for (; (*name != '\0') && (*pointer != '\0') && (*pointer != '/'); (void)name++, pointer++) /* compare until next '/' */
    {
        if (*pointer == '~')
        {
            /* check for escaped '~' (~0) and '/' (~1) */
            if (((pointer[1] != '0') || (*name != '~')) && ((pointer[1] != '1') || (*name != '/')))
            {
                /* invalid escape sequence or wrong character in *name */
                return false;
            }
            else
            {
                pointer++;
            }
        }
        else if ((!case_sensitive && (tolower(*name) != tolower(*pointer))) || (case_sensitive && (*name != *pointer)))
        {
            return false;
        }
    }
    if (((*pointer != 0) && (*pointer != '/')) != (*name != 0))
    {
        /* one string has ended, the other not */
        return false;;
    }

    return true;
}

/* calculate the length of a string if encoded as JSON pointer with ~0 and ~1 escape sequences */
static size_t pointer_encoded_length(const unsigned char *string)
{
    size_t length;
    for (length = 0; *string != '\0'; (void)string++, length++)
    {
        /* character needs to be escaped? */
        if ((*string == '~') || (*string == '/'))
        {
            length++;
        }
    }

    return length;
}

/* copy a string while escaping '~' and '/' with ~0 and ~1 JSON pointer escape codes */
static void encode_string_as_pointer(unsigned char *destination, const unsigned char *source)
{
    for (; source[0] != '\0'; (void)source++, destination++)
    {
        if (source[0] == '/')
        {
            destination[0] = '~';
            destination[1] = '1';
            destination++;
        }
        else if (source[0] == '~')
        {
            destination[0] = '~';
            destination[1] = '0';
            destination++;
        }
        else
        {
            destination[0] = source[0];
        }
    }

    destination[0] = '\0';
}

CJSON_PUBLIC(char *) _cJSONUtils_FindPointerFromObjectTo(const _cJSON * const object, const _cJSON * const target)
{
    size_t child_index = 0;
    _cJSON *current_child = 0;

    if ((object == NULL) || (target == NULL))
    {
        return NULL;
    }

    if (object == target)
    {
        /* found */
        return (char*)_cJSONUtils_strdup((const unsigned char*)"");
    }

    /* recursively search all children of the object or array */
    for (current_child = object->child; current_child != NULL; (void)(current_child = current_child->next), child_index++)
    {
        unsigned char *target_pointer = (unsigned char*)_cJSONUtils_FindPointerFromObjectTo(current_child, target);
        /* found the target? */
        if (target_pointer != NULL)
        {
            if (_cJSON_IsArray(object))
            {
                /* reserve enough memory for a 64 bit integer + '/' and '\0' */
                unsigned char *full_pointer = (unsigned char*)_cJSON_malloc(strlen((char*)target_pointer) + 20 + sizeof("/"));
                /* check if conversion to unsigned long is valid
                 * This should be eliminated at compile time by dead code elimination
                 * if size_t is an alias of unsigned long, or if it is bigger */
                if (child_index > ULONG_MAX)
                {
                    _cJSON_free(target_pointer);
                    _cJSON_free(full_pointer);
                    return NULL;
                }
                sprintf((char*)full_pointer, "/%lu%s", (unsigned long)child_index, target_pointer); /* /<array_index><path> */
                _cJSON_free(target_pointer);

                return (char*)full_pointer;
            }

            if (_cJSON_IsObject(object))
            {
                unsigned char *full_pointer = (unsigned char*)_cJSON_malloc(strlen((char*)target_pointer) + pointer_encoded_length((unsigned char*)current_child->string) + 2);
                full_pointer[0] = '/';
                encode_string_as_pointer(full_pointer + 1, (unsigned char*)current_child->string);
                strcat((char*)full_pointer, (char*)target_pointer);
                _cJSON_free(target_pointer);

                return (char*)full_pointer;
            }

            /* reached leaf of the tree, found nothing */
            _cJSON_free(target_pointer);
            return NULL;
        }
    }

    /* not found */
    return NULL;
}

/* non broken version of _cJSON_GetArrayItem */
static _cJSON *get_array_item(const _cJSON *array, size_t item)
{
    _cJSON *child = array ? array->child : NULL;
    while ((child != NULL) && (item > 0))
    {
        item--;
        child = child->next;
    }

    return child;
}

static _cJSON_bool decode_array_index_from_pointer(const unsigned char * const pointer, size_t * const index)
{
    size_t parsed_index = 0;
    size_t position = 0;

    if ((pointer[0] == '0') && ((pointer[1] != '\0') && (pointer[1] != '/')))
    {
        /* leading zeroes are not permitted */
        return 0;
    }

    for (position = 0; (pointer[position] >= '0') && (pointer[0] <= '9'); position++)
    {
        parsed_index = (10 * parsed_index) + (size_t)(pointer[position] - '0');

    }

    if ((pointer[position] != '\0') && (pointer[position] != '/'))
    {
        return 0;
    }

    *index = parsed_index;

    return 1;
}

static _cJSON *get_item_from_pointer(_cJSON * const object, const char * pointer, const _cJSON_bool case_sensitive)
{
    _cJSON *current_element = object;

    if (pointer == NULL)
    {
        return NULL;
    }

    /* follow path of the pointer */
    while ((pointer[0] == '/') && (current_element != NULL))
    {
        pointer++;
        if (_cJSON_IsArray(current_element))
        {
            size_t index = 0;
            if (!decode_array_index_from_pointer((const unsigned char*)pointer, &index))
            {
                return NULL;
            }

            current_element = get_array_item(current_element, index);
        }
        else if (_cJSON_IsObject(current_element))
        {
            current_element = current_element->child;
            /* GetObjectItem. */
            while ((current_element != NULL) && !compare_pointers((unsigned char*)current_element->string, (const unsigned char*)pointer, case_sensitive))
            {
                current_element = current_element->next;
            }
        }
        else
        {
            return NULL;
        }

        /* skip to the next path token or end of string */
        while ((pointer[0] != '\0') && (pointer[0] != '/'))
        {
            pointer++;
        }
    }

    return current_element;
}

CJSON_PUBLIC(_cJSON *) _cJSONUtils_GetPointer(_cJSON * const object, const char *pointer)
{
    return get_item_from_pointer(object, pointer, false);
}

CJSON_PUBLIC(_cJSON *) _cJSONUtils_GetPointerCaseSensitive(_cJSON * const object, const char *pointer)
{
    return get_item_from_pointer(object, pointer, true);
}

/* JSON Patch implementation. */
static void decode_pointer_inplace(unsigned char *string)
{
    unsigned char *decoded_string = string;

    if (string == NULL) {
        return;
    }

    for (; *string; (void)decoded_string++, string++)
    {
        if (string[0] == '~')
        {
            if (string[1] == '0')
            {
                decoded_string[0] = '~';
            }
            else if (string[1] == '1')
            {
                decoded_string[1] = '/';
            }
            else
            {
                /* invalid escape sequence */
                return;
            }

            string++;
        }
    }

    decoded_string[0] = '\0';
}

/* non-broken _cJSON_DetachItemFromArray */
static _cJSON *detach_item_from_array(_cJSON *array, size_t which)
{
    _cJSON *c = array->child;
    while (c && (which > 0))
    {
        c = c->next;
        which--;
    }
    if (!c)
    {
        /* item doesn't exist */
        return NULL;
    }
    if (c != array->child)
    {
        /* not the first element */
        c->prev->next = c->next;
    }
    if (c->next)
    {
        c->next->prev = c->prev;
    }
    if (c == array->child)
    {
        array->child = c->next;
    }
    else if (c->next == NULL)
    {
        array->child->prev = c->prev;
    }
    /* make sure the detached item doesn't point anywhere anymore */
    c->prev = c->next = NULL;

    return c;
}

/* detach an item at the given path */
static _cJSON *detach_path(_cJSON *object, const unsigned char *path, const _cJSON_bool case_sensitive)
{
    unsigned char *parent_pointer = NULL;
    unsigned char *child_pointer = NULL;
    _cJSON *parent = NULL;
    _cJSON *detached_item = NULL;

    /* copy path and split it in parent and child */
    parent_pointer = _cJSONUtils_strdup(path);
    if (parent_pointer == NULL) {
        goto cleanup;
    }

    child_pointer = (unsigned char*)strrchr((char*)parent_pointer, '/'); /* last '/' */
    if (child_pointer == NULL)
    {
        goto cleanup;
    }
    /* split strings */
    child_pointer[0] = '\0';
    child_pointer++;

    parent = get_item_from_pointer(object, (char*)parent_pointer, case_sensitive);
    decode_pointer_inplace(child_pointer);

    if (_cJSON_IsArray(parent))
    {
        size_t index = 0;
        if (!decode_array_index_from_pointer(child_pointer, &index))
        {
            goto cleanup;
        }
        detached_item = detach_item_from_array(parent, index);
    }
    else if (_cJSON_IsObject(parent))
    {
        detached_item = _cJSON_DetachItemFromObject(parent, (char*)child_pointer);
    }
    else
    {
        /* Couldn't find object to remove child from. */
        goto cleanup;
    }

cleanup:
    if (parent_pointer != NULL)
    {
        _cJSON_free(parent_pointer);
    }

    return detached_item;
}

/* sort lists using mergesort */
static _cJSON *sort_list(_cJSON *list, const _cJSON_bool case_sensitive)
{
    _cJSON *first = list;
    _cJSON *second = list;
    _cJSON *current_item = list;
    _cJSON *result = list;
    _cJSON *result_tail = NULL;

    if ((list == NULL) || (list->next == NULL))
    {
        /* One entry is sorted already. */
        return result;
    }

    while ((current_item != NULL) && (current_item->next != NULL) && (compare_strings((unsigned char*)current_item->string, (unsigned char*)current_item->next->string, case_sensitive) < 0))
    {
        /* Test for list sorted. */
        current_item = current_item->next;
    }
    if ((current_item == NULL) || (current_item->next == NULL))
    {
        /* Leave sorted lists unmodified. */
        return result;
    }

    /* reset pointer to the beginning */
    current_item = list;
    while (current_item != NULL)
    {
        /* Walk two pointers to find the middle. */
        second = second->next;
        current_item = current_item->next;
        /* advances current_item two steps at a time */
        if (current_item != NULL)
        {
            current_item = current_item->next;
        }
    }
    if ((second != NULL) && (second->prev != NULL))
    {
        /* Split the lists */
        second->prev->next = NULL;
        second->prev = NULL;
    }

    /* Recursively sort the sub-lists. */
    first = sort_list(first, case_sensitive);
    second = sort_list(second, case_sensitive);
    result = NULL;

    /* Merge the sub-lists */
    while ((first != NULL) && (second != NULL))
    {
        _cJSON *smaller = NULL;
        if (compare_strings((unsigned char*)first->string, (unsigned char*)second->string, case_sensitive) < 0)
        {
            smaller = first;
        }
        else
        {
            smaller = second;
        }

        if (result == NULL)
        {
            /* start merged list with the smaller element */
            result_tail = smaller;
            result = smaller;
        }
        else
        {
            /* add smaller element to the list */
            result_tail->next = smaller;
            smaller->prev = result_tail;
            result_tail = smaller;
        }

        if (first == smaller)
        {
            first = first->next;
        }
        else
        {
            second = second->next;
        }
    }

    if (first != NULL)
    {
        /* Append rest of first list. */
        if (result == NULL)
        {
            return first;
        }
        result_tail->next = first;
        first->prev = result_tail;
    }
    if (second != NULL)
    {
        /* Append rest of second list */
        if (result == NULL)
        {
            return second;
        }
        result_tail->next = second;
        second->prev = result_tail;
    }

    return result;
}

static void sort_object(_cJSON * const object, const _cJSON_bool case_sensitive)
{
    if (object == NULL)
    {
        return;
    }
    object->child = sort_list(object->child, case_sensitive);
}

static _cJSON_bool compare_json(_cJSON *a, _cJSON *b, const _cJSON_bool case_sensitive)
{
    if ((a == NULL) || (b == NULL) || ((a->type & 0xFF) != (b->type & 0xFF)))
    {
        /* mismatched type. */
        return false;
    }
    switch (a->type & 0xFF)
    {
        case _cJSON_Number:
            /* numeric mismatch. */
            if ((a->valueint != b->valueint) || (!compare_double(a->valuedouble, b->valuedouble)))
            {
                return false;
            }
            else
            {
                return true;
            }

        case _cJSON_String:
            /* string mismatch. */
            if (strcmp(a->valuestring, b->valuestring) != 0)
            {
                return false;
            }
            else
            {
                return true;
            }

        case _cJSON_Array:
            for ((void)(a = a->child), b = b->child; (a != NULL) && (b != NULL); (void)(a = a->next), b = b->next)
            {
                _cJSON_bool identical = compare_json(a, b, case_sensitive);
                if (!identical)
                {
                    return false;
                }
            }

            /* array size mismatch? (one of both children is not NULL) */
            if ((a != NULL) || (b != NULL))
            {
                return false;
            }
            else
            {
                return true;
            }

        case _cJSON_Object:
            sort_object(a, case_sensitive);
            sort_object(b, case_sensitive);
            for ((void)(a = a->child), b = b->child; (a != NULL) && (b != NULL); (void)(a = a->next), b = b->next)
            {
                _cJSON_bool identical = false;
                /* compare object keys */
                if (compare_strings((unsigned char*)a->string, (unsigned char*)b->string, case_sensitive))
                {
                    /* missing member */
                    return false;
                }
                identical = compare_json(a, b, case_sensitive);
                if (!identical)
                {
                    return false;
                }
            }

            /* object length mismatch (one of both children is not null) */
            if ((a != NULL) || (b != NULL))
            {
                return false;
            }
            else
            {
                return true;
            }

        default:
            break;
    }

    /* null, true or false */
    return true;
}

/* non broken version of _cJSON_InsertItemInArray */
static _cJSON_bool insert_item_in_array(_cJSON *array, size_t which, _cJSON *newitem)
{
    _cJSON *child = array->child;
    while (child && (which > 0))
    {
        child = child->next;
        which--;
    }
    if (which > 0)
    {
        /* item is after the end of the array */
        return 0;
    }
    if (child == NULL)
    {
        _cJSON_AddItemToArray(array, newitem);
        return 1;
    }

    /* insert into the linked list */
    newitem->next = child;
    newitem->prev = child->prev;
    child->prev = newitem;

    /* was it at the beginning */
    if (child == array->child)
    {
        array->child = newitem;
    }
    else
    {
        newitem->prev->next = newitem;
    }

    return 1;
}

static _cJSON *get_object_item(const _cJSON * const object, const char* name, const _cJSON_bool case_sensitive)
{
    if (case_sensitive)
    {
        return _cJSON_GetObjectItemCaseSensitive(object, name);
    }

    return _cJSON_GetObjectItem(object, name);
}

enum patch_operation { INVALID, ADD, REMOVE, REPLACE, MOVE, COPY, TEST };

static enum patch_operation decode_patch_operation(const _cJSON * const patch, const _cJSON_bool case_sensitive)
{
    _cJSON *operation = get_object_item(patch, "op", case_sensitive);
    if (!_cJSON_IsString(operation))
    {
        return INVALID;
    }

    if (strcmp(operation->valuestring, "add") == 0)
    {
        return ADD;
    }

    if (strcmp(operation->valuestring, "remove") == 0)
    {
        return REMOVE;
    }

    if (strcmp(operation->valuestring, "replace") == 0)
    {
        return REPLACE;
    }

    if (strcmp(operation->valuestring, "move") == 0)
    {
        return MOVE;
    }

    if (strcmp(operation->valuestring, "copy") == 0)
    {
        return COPY;
    }

    if (strcmp(operation->valuestring, "test") == 0)
    {
        return TEST;
    }

    return INVALID;
}

/* overwrite and existing item with another one and free resources on the way */
static void overwrite_item(_cJSON * const root, const _cJSON replacement)
{
    if (root == NULL)
    {
        return;
    }

    if (root->string != NULL)
    {
        _cJSON_free(root->string);
    }
    if (root->valuestring != NULL)
    {
        _cJSON_free(root->valuestring);
    }
    if (root->child != NULL)
    {
        _cJSON_Delete(root->child);
    }

    memcpy(root, &replacement, sizeof(_cJSON));
}

static int apply_patch(_cJSON *object, const _cJSON *patch, const _cJSON_bool case_sensitive)
{
    _cJSON *path = NULL;
    _cJSON *value = NULL;
    _cJSON *parent = NULL;
    enum patch_operation opcode = INVALID;
    unsigned char *parent_pointer = NULL;
    unsigned char *child_pointer = NULL;
    int status = 0;

    path = get_object_item(patch, "path", case_sensitive);
    if (!_cJSON_IsString(path))
    {
        /* malformed patch. */
        status = 2;
        goto cleanup;
    }

    opcode = decode_patch_operation(patch, case_sensitive);
    if (opcode == INVALID)
    {
        status = 3;
        goto cleanup;
    }
    else if (opcode == TEST)
    {
        /* compare value: {...} with the given path */
        status = !compare_json(get_item_from_pointer(object, path->valuestring, case_sensitive), get_object_item(patch, "value", case_sensitive), case_sensitive);
        goto cleanup;
    }

    /* special case for replacing the root */
    if (path->valuestring[0] == '\0')
    {
        if (opcode == REMOVE)
        {
            static const _cJSON invalid = { NULL, NULL, NULL, _cJSON_Invalid, NULL, 0, 0, NULL};

            overwrite_item(object, invalid);

            status = 0;
            goto cleanup;
        }

        if ((opcode == REPLACE) || (opcode == ADD))
        {
            value = get_object_item(patch, "value", case_sensitive);
            if (value == NULL)
            {
                /* missing "value" for add/replace. */
                status = 7;
                goto cleanup;
            }

            value = _cJSON_Duplicate(value, 1);
            if (value == NULL)
            {
                /* out of memory for add/replace. */
                status = 8;
                goto cleanup;
            }

            overwrite_item(object, *value);

            /* delete the duplicated value */
            _cJSON_free(value);
            value = NULL;

            /* the string "value" isn't needed */
            if (object->string != NULL)
            {
                _cJSON_free(object->string);
                object->string = NULL;
            }

            status = 0;
            goto cleanup;
        }
    }

    if ((opcode == REMOVE) || (opcode == REPLACE))
    {
        /* Get rid of old. */
        _cJSON *old_item = detach_path(object, (unsigned char*)path->valuestring, case_sensitive);
        if (old_item == NULL)
        {
            status = 13;
            goto cleanup;
        }
        _cJSON_Delete(old_item);
        if (opcode == REMOVE)
        {
            /* For Remove, this job is done. */
            status = 0;
            goto cleanup;
        }
    }

    /* Copy/Move uses "from". */
    if ((opcode == MOVE) || (opcode == COPY))
    {
        _cJSON *from = get_object_item(patch, "from", case_sensitive);
        if (from == NULL)
        {
            /* missing "from" for copy/move. */
            status = 4;
            goto cleanup;
        }

        if (opcode == MOVE)
        {
            value = detach_path(object, (unsigned char*)from->valuestring, case_sensitive);
        }
        if (opcode == COPY)
        {
            value = get_item_from_pointer(object, from->valuestring, case_sensitive);
        }
        if (value == NULL)
        {
            /* missing "from" for copy/move. */
            status = 5;
            goto cleanup;
        }
        if (opcode == COPY)
        {
            value = _cJSON_Duplicate(value, 1);
        }
        if (value == NULL)
        {
            /* out of memory for copy/move. */
            status = 6;
            goto cleanup;
        }
    }
    else /* Add/Replace uses "value". */
    {
        value = get_object_item(patch, "value", case_sensitive);
        if (value == NULL)
        {
            /* missing "value" for add/replace. */
            status = 7;
            goto cleanup;
        }
        value = _cJSON_Duplicate(value, 1);
        if (value == NULL)
        {
            /* out of memory for add/replace. */
            status = 8;
            goto cleanup;
        }
    }

    /* Now, just add "value" to "path". */

    /* split pointer in parent and child */
    parent_pointer = _cJSONUtils_strdup((unsigned char*)path->valuestring);
    if (parent_pointer) {
        child_pointer = (unsigned char*)strrchr((char*)parent_pointer, '/');
    }
    if (child_pointer != NULL)
    {
        child_pointer[0] = '\0';
        child_pointer++;
    }
    parent = get_item_from_pointer(object, (char*)parent_pointer, case_sensitive);
    decode_pointer_inplace(child_pointer);

    /* add, remove, replace, move, copy, test. */
    if ((parent == NULL) || (child_pointer == NULL))
    {
        /* Couldn't find object to add to. */
        status = 9;
        goto cleanup;
    }
    else if (_cJSON_IsArray(parent))
    {
        if (strcmp((char*)child_pointer, "-") == 0)
        {
            _cJSON_AddItemToArray(parent, value);
            value = NULL;
        }
        else
        {
            size_t index = 0;
            if (!decode_array_index_from_pointer(child_pointer, &index))
            {
                status = 11;
                goto cleanup;
            }

            if (!insert_item_in_array(parent, index, value))
            {
                status = 10;
                goto cleanup;
            }
            value = NULL;
        }
    }
    else if (_cJSON_IsObject(parent))
    {
        if (case_sensitive)
        {
            _cJSON_DeleteItemFromObjectCaseSensitive(parent, (char*)child_pointer);
        }
        else
        {
            _cJSON_DeleteItemFromObject(parent, (char*)child_pointer);
        }
        _cJSON_AddItemToObject(parent, (char*)child_pointer, value);
        value = NULL;
    }
    else /* parent is not an object */
    {
        /* Couldn't find object to add to. */
        status = 9;
        goto cleanup;
    }

cleanup:
    if (value != NULL)
    {
        _cJSON_Delete(value);
    }
    if (parent_pointer != NULL)
    {
        _cJSON_free(parent_pointer);
    }

    return status;
}

CJSON_PUBLIC(int) _cJSONUtils_ApplyPatches(_cJSON * const object, const _cJSON * const patches)
{
    const _cJSON *current_patch = NULL;
    int status = 0;

    if (!_cJSON_IsArray(patches))
    {
        /* malformed patches. */
        return 1;
    }

    if (patches != NULL)
    {
        current_patch = patches->child;
    }

    while (current_patch != NULL)
    {
        status = apply_patch(object, current_patch, false);
        if (status != 0)
        {
            return status;
        }
        current_patch = current_patch->next;
    }

    return 0;
}

CJSON_PUBLIC(int) _cJSONUtils_ApplyPatchesCaseSensitive(_cJSON * const object, const _cJSON * const patches)
{
    const _cJSON *current_patch = NULL;
    int status = 0;

    if (!_cJSON_IsArray(patches))
    {
        /* malformed patches. */
        return 1;
    }

    if (patches != NULL)
    {
        current_patch = patches->child;
    }

    while (current_patch != NULL)
    {
        status = apply_patch(object, current_patch, true);
        if (status != 0)
        {
            return status;
        }
        current_patch = current_patch->next;
    }

    return 0;
}

static void compose_patch(_cJSON * const patches, const unsigned char * const operation, const unsigned char * const path, const unsigned char *suffix, const _cJSON * const value)
{
    _cJSON *patch = NULL;

    if ((patches == NULL) || (operation == NULL) || (path == NULL))
    {
        return;
    }

    patch = _cJSON_CreateObject();
    if (patch == NULL)
    {
        return;
    }
    _cJSON_AddItemToObject(patch, "op", _cJSON_CreateString((const char*)operation));

    if (suffix == NULL)
    {
        _cJSON_AddItemToObject(patch, "path", _cJSON_CreateString((const char*)path));
    }
    else
    {
        size_t suffix_length = pointer_encoded_length(suffix);
        size_t path_length = strlen((const char*)path);
        unsigned char *full_path = (unsigned char*)_cJSON_malloc(path_length + suffix_length + sizeof("/"));

        sprintf((char*)full_path, "%s/", (const char*)path);
        encode_string_as_pointer(full_path + path_length + 1, suffix);

        _cJSON_AddItemToObject(patch, "path", _cJSON_CreateString((const char*)full_path));
        _cJSON_free(full_path);
    }

    if (value != NULL)
    {
        _cJSON_AddItemToObject(patch, "value", _cJSON_Duplicate(value, 1));
    }
    _cJSON_AddItemToArray(patches, patch);
}

CJSON_PUBLIC(void) _cJSONUtils_AddPatchToArray(_cJSON * const array, const char * const operation, const char * const path, const _cJSON * const value)
{
    compose_patch(array, (const unsigned char*)operation, (const unsigned char*)path, NULL, value);
}

static void create_patches(_cJSON * const patches, const unsigned char * const path, _cJSON * const from, _cJSON * const to, const _cJSON_bool case_sensitive)
{
    if ((from == NULL) || (to == NULL))
    {
        return;
    }

    if ((from->type & 0xFF) != (to->type & 0xFF))
    {
        compose_patch(patches, (const unsigned char*)"replace", path, 0, to);
        return;
    }

    switch (from->type & 0xFF)
    {
        case _cJSON_Number:
            if ((from->valueint != to->valueint) || !compare_double(from->valuedouble, to->valuedouble))
            {
                compose_patch(patches, (const unsigned char*)"replace", path, NULL, to);
            }
            return;

        case _cJSON_String:
            if (strcmp(from->valuestring, to->valuestring) != 0)
            {
                compose_patch(patches, (const unsigned char*)"replace", path, NULL, to);
            }
            return;

        case _cJSON_Array:
        {
            size_t index = 0;
            _cJSON *from_child = from->child;
            _cJSON *to_child = to->child;
            unsigned char *new_path = (unsigned char*)_cJSON_malloc(strlen((const char*)path) + 20 + sizeof("/")); /* Allow space for 64bit int. log10(2^64) = 20 */

            /* generate patches for all array elements that exist in both "from" and "to" */
            for (index = 0; (from_child != NULL) && (to_child != NULL); (void)(from_child = from_child->next), (void)(to_child = to_child->next), index++)
            {
                /* check if conversion to unsigned long is valid
                 * This should be eliminated at compile time by dead code elimination
                 * if size_t is an alias of unsigned long, or if it is bigger */
                if (index > ULONG_MAX)
                {
                    _cJSON_free(new_path);
                    return;
                }
                sprintf((char*)new_path, "%s/%lu", path, (unsigned long)index); /* path of the current array element */
                create_patches(patches, new_path, from_child, to_child, case_sensitive);
            }

            /* remove leftover elements from 'from' that are not in 'to' */
            for (; (from_child != NULL); (void)(from_child = from_child->next))
            {
                /* check if conversion to unsigned long is valid
                 * This should be eliminated at compile time by dead code elimination
                 * if size_t is an alias of unsigned long, or if it is bigger */
                if (index > ULONG_MAX)
                {
                    _cJSON_free(new_path);
                    return;
                }
                sprintf((char*)new_path, "%lu", (unsigned long)index);
                compose_patch(patches, (const unsigned char*)"remove", path, new_path, NULL);
            }
            /* add new elements in 'to' that were not in 'from' */
            for (; (to_child != NULL); (void)(to_child = to_child->next), index++)
            {
                compose_patch(patches, (const unsigned char*)"add", path, (const unsigned char*)"-", to_child);
            }
            _cJSON_free(new_path);
            return;
        }

        case _cJSON_Object:
        {
            _cJSON *from_child = NULL;
            _cJSON *to_child = NULL;
            sort_object(from, case_sensitive);
            sort_object(to, case_sensitive);

            from_child = from->child;
            to_child = to->child;
            /* for all object values in the object with more of them */
            while ((from_child != NULL) || (to_child != NULL))
            {
                int diff;
                if (from_child == NULL)
                {
                    diff = 1;
                }
                else if (to_child == NULL)
                {
                    diff = -1;
                }
                else
                {
                    diff = compare_strings((unsigned char*)from_child->string, (unsigned char*)to_child->string, case_sensitive);
                }

                if (diff == 0)
                {
                    /* both object keys are the same */
                    size_t path_length = strlen((const char*)path);
                    size_t from_child_name_length = pointer_encoded_length((unsigned char*)from_child->string);
                    unsigned char *new_path = (unsigned char*)_cJSON_malloc(path_length + from_child_name_length + sizeof("/"));

                    sprintf((char*)new_path, "%s/", path);
                    encode_string_as_pointer(new_path + path_length + 1, (unsigned char*)from_child->string);

                    /* create a patch for the element */
                    create_patches(patches, new_path, from_child, to_child, case_sensitive);
                    _cJSON_free(new_path);

                    from_child = from_child->next;
                    to_child = to_child->next;
                }
                else if (diff < 0)
                {
                    /* object element doesn't exist in 'to' --> remove it */
                    compose_patch(patches, (const unsigned char*)"remove", path, (unsigned char*)from_child->string, NULL);

                    from_child = from_child->next;
                }
                else
                {
                    /* object element doesn't exist in 'from' --> add it */
                    compose_patch(patches, (const unsigned char*)"add", path, (unsigned char*)to_child->string, to_child);

                    to_child = to_child->next;
                }
            }
            return;
        }

        default:
            break;
    }
}

CJSON_PUBLIC(_cJSON *) _cJSONUtils_GeneratePatches(_cJSON * const from, _cJSON * const to)
{
    _cJSON *patches = NULL;

    if ((from == NULL) || (to == NULL))
    {
        return NULL;
    }

    patches = _cJSON_CreateArray();
    create_patches(patches, (const unsigned char*)"", from, to, false);

    return patches;
}

CJSON_PUBLIC(_cJSON *) _cJSONUtils_GeneratePatchesCaseSensitive(_cJSON * const from, _cJSON * const to)
{
    _cJSON *patches = NULL;

    if ((from == NULL) || (to == NULL))
    {
        return NULL;
    }

    patches = _cJSON_CreateArray();
    create_patches(patches, (const unsigned char*)"", from, to, true);

    return patches;
}

CJSON_PUBLIC(void) _cJSONUtils_SortObject(_cJSON * const object)
{
    sort_object(object, false);
}

CJSON_PUBLIC(void) _cJSONUtils_SortObjectCaseSensitive(_cJSON * const object)
{
    sort_object(object, true);
}

static _cJSON *merge_patch(_cJSON *target, const _cJSON * const patch, const _cJSON_bool case_sensitive)
{
    _cJSON *patch_child = NULL;

    if (!_cJSON_IsObject(patch))
    {
        /* scalar value, array or NULL, just duplicate */
        _cJSON_Delete(target);
        return _cJSON_Duplicate(patch, 1);
    }

    if (!_cJSON_IsObject(target))
    {
        _cJSON_Delete(target);
        target = _cJSON_CreateObject();
    }

    patch_child = patch->child;
    while (patch_child != NULL)
    {
        if (_cJSON_IsNull(patch_child))
        {
            /* NULL is the indicator to remove a value, see RFC7396 */
            if (case_sensitive)
            {
                _cJSON_DeleteItemFromObjectCaseSensitive(target, patch_child->string);
            }
            else
            {
                _cJSON_DeleteItemFromObject(target, patch_child->string);
            }
        }
        else
        {
            _cJSON *replace_me = NULL;
            _cJSON *replacement = NULL;

            if (case_sensitive)
            {
                replace_me = _cJSON_DetachItemFromObjectCaseSensitive(target, patch_child->string);
            }
            else
            {
                replace_me = _cJSON_DetachItemFromObject(target, patch_child->string);
            }

            replacement = merge_patch(replace_me, patch_child, case_sensitive);
            if (replacement == NULL)
            {
                return NULL;
            }

            _cJSON_AddItemToObject(target, patch_child->string, replacement);
        }
        patch_child = patch_child->next;
    }
    return target;
}

CJSON_PUBLIC(_cJSON *) _cJSONUtils_MergePatch(_cJSON *target, const _cJSON * const patch)
{
    return merge_patch(target, patch, false);
}

CJSON_PUBLIC(_cJSON *) _cJSONUtils_MergePatchCaseSensitive(_cJSON *target, const _cJSON * const patch)
{
    return merge_patch(target, patch, true);
}

static _cJSON *generate_merge_patch(_cJSON * const from, _cJSON * const to, const _cJSON_bool case_sensitive)
{
    _cJSON *from_child = NULL;
    _cJSON *to_child = NULL;
    _cJSON *patch = NULL;
    if (to == NULL)
    {
        /* patch to delete everything */
        return _cJSON_CreateNull();
    }
    if (!_cJSON_IsObject(to) || !_cJSON_IsObject(from))
    {
        return _cJSON_Duplicate(to, 1);
    }

    sort_object(from, case_sensitive);
    sort_object(to, case_sensitive);

    from_child = from->child;
    to_child = to->child;
    patch = _cJSON_CreateObject();
    if (patch == NULL)
    {
        return NULL;
    }
    while (from_child || to_child)
    {
        int diff;
        if (from_child != NULL)
        {
            if (to_child != NULL)
            {
                diff = strcmp(from_child->string, to_child->string);
            }
            else
            {
                diff = -1;
            }
        }
        else
        {
            diff = 1;
        }

        if (diff < 0)
        {
            /* from has a value that to doesn't have -> remove */
            _cJSON_AddItemToObject(patch, from_child->string, _cJSON_CreateNull());

            from_child = from_child->next;
        }
        else if (diff > 0)
        {
            /* to has a value that from doesn't have -> add to patch */
            _cJSON_AddItemToObject(patch, to_child->string, _cJSON_Duplicate(to_child, 1));

            to_child = to_child->next;
        }
        else
        {
            /* object key exists in both objects */
            if (!compare_json(from_child, to_child, case_sensitive))
            {
                /* not identical --> generate a patch */
                _cJSON_AddItemToObject(patch, to_child->string, _cJSONUtils_GenerateMergePatch(from_child, to_child));
            }

            /* next key in the object */
            from_child = from_child->next;
            to_child = to_child->next;
        }
    }
    if (patch->child == NULL)
    {
        /* no patch generated */
        _cJSON_Delete(patch);
        return NULL;
    }

    return patch;
}

CJSON_PUBLIC(_cJSON *) _cJSONUtils_GenerateMergePatch(_cJSON * const from, _cJSON * const to)
{
    return generate_merge_patch(from, to, false);
}

CJSON_PUBLIC(_cJSON *) _cJSONUtils_GenerateMergePatchCaseSensitive(_cJSON * const from, _cJSON * const to)
{
    return generate_merge_patch(from, to, true);
}
