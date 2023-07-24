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

#ifdef CJSON_PREFIX
#define _cJSON dJSON
#endif

#ifndef _cJSON_Utils__h
#define _cJSON_Utils__h

#ifdef __cplusplus
extern "C"
{
#endif

#include "_cJSON.h"

/* Implement RFC6901 (https://tools.ietf.org/html/rfc6901) JSON Pointer spec. */
CJSON_PUBLIC(_cJSON *) _cJSONUtils_GetPointer(_cJSON * const object, const char *pointer);
CJSON_PUBLIC(_cJSON *) _cJSONUtils_GetPointerCaseSensitive(_cJSON * const object, const char *pointer);

/* Implement RFC6902 (https://tools.ietf.org/html/rfc6902) JSON Patch spec. */
/* NOTE: This modifies objects in 'from' and 'to' by sorting the elements by their key */
CJSON_PUBLIC(_cJSON *) _cJSONUtils_GeneratePatches(_cJSON * const from, _cJSON * const to);
CJSON_PUBLIC(_cJSON *) _cJSONUtils_GeneratePatchesCaseSensitive(_cJSON * const from, _cJSON * const to);
/* Utility for generating patch array entries. */
CJSON_PUBLIC(void) _cJSONUtils_AddPatchToArray(_cJSON * const array, const char * const operation, const char * const path, const _cJSON * const value);
/* Returns 0 for success. */
CJSON_PUBLIC(int) _cJSONUtils_ApplyPatches(_cJSON * const object, const _cJSON * const patches);
CJSON_PUBLIC(int) _cJSONUtils_ApplyPatchesCaseSensitive(_cJSON * const object, const _cJSON * const patches);

/*
// Note that ApplyPatches is NOT atomic on failure. To implement an atomic ApplyPatches, use:
//int _cJSONUtils_AtomicApplyPatches(_cJSON **object, _cJSON *patches)
//{
//    _cJSON *modme = _cJSON_Duplicate(*object, 1);
//    int error = _cJSONUtils_ApplyPatches(modme, patches);
//    if (!error)
//    {
//        _cJSON_Delete(*object);
//        *object = modme;
//    }
//    else
//    {
//        _cJSON_Delete(modme);
//    }
//
//    return error;
//}
// Code not added to library since this strategy is a LOT slower.
*/

/* Implement RFC7386 (https://tools.ietf.org/html/rfc7396) JSON Merge Patch spec. */
/* target will be modified by patch. return value is new ptr for target. */
CJSON_PUBLIC(_cJSON *) _cJSONUtils_MergePatch(_cJSON *target, const _cJSON * const patch);
CJSON_PUBLIC(_cJSON *) _cJSONUtils_MergePatchCaseSensitive(_cJSON *target, const _cJSON * const patch);
/* generates a patch to move from -> to */
/* NOTE: This modifies objects in 'from' and 'to' by sorting the elements by their key */
CJSON_PUBLIC(_cJSON *) _cJSONUtils_GenerateMergePatch(_cJSON * const from, _cJSON * const to);
CJSON_PUBLIC(_cJSON *) _cJSONUtils_GenerateMergePatchCaseSensitive(_cJSON * const from, _cJSON * const to);

/* Given a root object and a target object, construct a pointer from one to the other. */
CJSON_PUBLIC(char *) _cJSONUtils_FindPointerFromObjectTo(const _cJSON * const object, const _cJSON * const target);

/* Sorts the members of the object into alphabetical order. */
CJSON_PUBLIC(void) _cJSONUtils_SortObject(_cJSON * const object);
CJSON_PUBLIC(void) _cJSONUtils_SortObjectCaseSensitive(_cJSON * const object);

#ifdef __cplusplus
}
#endif

#endif
