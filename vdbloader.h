/*
    vdbloader - Copyright (c) 2019 Hisanari Otsu
    Distributed under MIT license. See LICENSE file for details.
*/

#pragma once

#ifdef __cplusplus
    #define VDBLOADER_EXTERN_C extern "C"
#else
    #define VDBLOADER_EXTERN_C
#endif

#ifdef _MSC_VER
    #ifdef VDBLOADER_EXPORTS
        #define VDBLOADER_PUBLIC_API VDBLOADER_EXTERN_C __declspec(dllexport)
    #else
        #define VDBLOADER_PUBLIC_API VDBLOADER_EXTERN_C __declspec(dllimport)
    #endif
#elif defined(__GNUC__) || defined(__clang__)
    #ifdef LM_EXPORTS
        #define VDBLOADER_PUBLIC_API VDBLOADER_EXTERN_C __attribute__ ((visibility("default")))
    #else
        #define VDBLOADER_PUBLIC_API VDBLOADER_EXTERN_C
    #endif
#endif

// ----------------------------------------------------------------------------

/*!
    \brief Error codes.
*/
enum VDBLoaderError {
    VDBLOADER_ERROR_INVALID_CONTEXT,    //!< Invalid context.
    VDBLOADER_ERROR_INVALID_ARGUMENT,   //!< Invalid argument error.
    VDBLOADER_ERROR_UNKNOWN,            //<! Unkonwn errors.
};

/*!
    \brief Callback function for error reporting.
    \param user User pointer.
    \param errorCode Error code.
    \param message Error message.
*/
typedef void (*VDBLoaderErrorFunc)(void* user, int errorCode, const char* message);

/*!
    \brief Register error reporting function.
    \param errorFunc Error reporting function.
    \param user User pointer.
*/
VDBLOADER_PUBLIC_API void vdbloaderSetErrorFunc(void* user, VDBLoaderErrorFunc errorFun);

// ----------------------------------------------------------------------------

// Three tuple of floating point values
struct VDBLoaderFloat3 {
    float x;
    float y;
    float z;
};

// Bound
struct VDBLoaderBound {
    VDBLoaderFloat3 min;
    VDBLoaderFloat3 max;
};

// Opaque context type
typedef struct VDBLoaderContext_* VDBLoaderContext;

/*!
    \brief Initialize context.
    \return Context.
*/
VDBLOADER_PUBLIC_API VDBLoaderContext vdbloaderCreateContext();

/*!
    \brief Release context.
    \param context Context.
*/
VDBLOADER_PUBLIC_API void vdbloaderReleaseContext(VDBLoaderContext context);

/*!
    \brief Load VDB file.
    \param context Context.
    \param path Path to the VDB file.
    \return True if loading is successful, false otherwise.
*/
VDBLOADER_PUBLIC_API bool vdbloaderLoadVDBFile(VDBLoaderContext context, const char* path);

/*!
    \brief Get bound of the grid.
    \param context Context.
    \return Bound of the grid.
*/
VDBLOADER_PUBLIC_API VDBLoaderBound vdbloaderGetBound(VDBLoaderContext context);

/*!
    \brief Get maximum scalar value in the grid.
    \param context Context.
    \return Maximum scalar value in the grid.
*/
VDBLOADER_PUBLIC_API float vbdloaderGetMaxScalar(VDBLoaderContext context);

/*!
    \brief Evaluate scalar at the give position.
    \param context Context.
    \param p Position.
    \return Evaluated value.
*/
VDBLOADER_PUBLIC_API float vbdloaderEvalScalar(VDBLoaderContext context, VDBLoaderFloat3 p);