
#ifndef _BASE64_H_
#define _BASE64_H_
/// @cond DO_NOT_SHOW

#include <stdio.h>
#include <stdlib.h>
#include "OGPlatformMacros.h"
 

OG_BEGIN

	
/** @file
 base64 helper functions
 */

/** 
 * Decodes a 64base encoded memory. The decoded memory is
 * expected to be freed by the caller by calling `free()`
 *
 * @returns the length of the out buffer
 *
 @since v0.8.1
 */
extern int  base64Decode(const unsigned char *in, unsigned int inLength, unsigned char **out);
    
/**
 * Encodes bytes into a 64base encoded memory with terminating '\0' character. 
 * The encoded memory is expected to be freed by the caller by calling `free()`
 *
 * @returns the length of the out buffer
 *
 @since v2.1.4
 */
extern int  base64Encode(const unsigned char *in, unsigned int inLength, char **out);



OG_END

 
#endif // __SUPPORT_BASE64_H__
