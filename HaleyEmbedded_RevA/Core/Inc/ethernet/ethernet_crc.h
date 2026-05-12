/*
 * ethernet_crc.h
 *
 * CRC32 computation for Haley packet validation.
 * Uses the standard IEEE 802.3 CRC-32 polynomial (reflected, 0xEDB88320).
 *
 * Layer: Ethernet / Service Layer
 * Dependencies: stdint.h, stddef.h
 *
 *  Created on: May 12, 2026
 *      Author: jeffreya181
 */

#ifndef INC_ETHERNET_ETHERNET_CRC_H_
#define INC_ETHERNET_ETHERNET_CRC_H_

#include <stdint.h>
#include <stddef.h>

/* -----------------------------------------------------------------------
 * API
 * ----------------------------------------------------------------------- */

/**
 * @brief  Compute CRC32 over a data buffer.
 *         Uses the reflected IEEE 802.3 polynomial (0xEDB88320).
 *
 * @param  data   Pointer to the start of the data buffer.
 * @param  length Number of bytes to process.
 * @return 32-bit CRC value.
 */
uint32_t ETH_CRC_Compute(const uint8_t *data, size_t length);

/**
 * @brief  Continue an in-progress CRC32 calculation.
 *         Used to compute CRC over non-contiguous buffers.
 *
 * @param  crc    Starting CRC value (use 0xFFFFFFFF to begin a new computation).
 * @param  data   Pointer to the next data chunk.
 * @param  length Number of bytes in this chunk.
 * @return Updated CRC value. Call ETH_CRC_Finalize() when done.
 */
uint32_t ETH_CRC_Update(uint32_t crc, const uint8_t *data, size_t length);

/**
 * @brief  Finalize a CRC that was built with ETH_CRC_Update().
 *         Inverts the running CRC to produce the final value.
 *
 * @param  crc  Running CRC value returned by the last ETH_CRC_Update() call.
 * @return Final CRC32 value.
 */
uint32_t ETH_CRC_Finalize(uint32_t crc);

/**
 * @brief  Verify that a buffer matches an expected CRC32 value.
 *
 * @param  data         Pointer to the data buffer.
 * @param  length       Number of bytes to check.
 * @param  expected_crc Expected CRC32 value.
 * @return 1 if CRC matches, 0 if mismatch.
 */
int ETH_CRC_Verify(const uint8_t *data, size_t length, uint32_t expected_crc);

#endif /* INC_ETHERNET_ETHERNET_CRC_H_ */
