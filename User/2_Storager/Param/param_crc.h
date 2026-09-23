#ifndef PARAM_CRC_H
#define PARAM_CRC_H

#include <stddef.h>
#include <stdint.h>

uint32_t ParamCRC32_Update(uint32_t crc, const void *data, size_t length);
uint32_t ParamCRC32(const void *data, size_t length);

#endif
