#ifndef WEBRTC_SYSTEM_WRAPPERS_INCLUDE_MEMORY_WRAPPER_H_
#define WEBRTC_SYSTEM_WRAPPERS_INCLUDE_MEMORY_WRAPPER_H_

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#if defined(__GNUC__) && defined(__arm__)
#include "med_memory.h"
#define WEBRTC_MALLOC(size) \
    med_malloc(size)
    
#define WEBRTC_FREE(p) \
    med_free((void*)(p))
#else
#include <stdlib.h>
#define WEBRTC_MALLOC(size) \
    malloc(size)

#define WEBRTC_FREE(p) \
    free((void*)(p))
#endif

#if defined(__cplusplus) || defined(c_plusplus)
}  // extern "C"
#endif

#endif // WEBRTC_SYSTEM_WRAPPERS_INCLUDE_MEMORY_WRAPPER_H_