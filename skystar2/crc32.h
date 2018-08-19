/*
 *      SkyStar2 driver based on chip FlexCopII
 */

#ifndef _crc32_h_
#define _crc32_h_

#include <sys/types.h>


extern u_int32_t
crc32_be(u_int32_t crc, u_int8_t const *data, size_t len);
#endif
