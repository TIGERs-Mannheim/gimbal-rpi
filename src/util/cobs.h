/*
 * Consistent Overhead Byte Stuffing with Zero Pair and Zero Run Elimination.
 *
 * Taken from:  PPP Consistent Overhead Byte Stuffing (COBS)
 * http://tools.ietf.org/html/draft-ietf-pppext-cobs-00.txt
 *
 * - Removed PPP flag extraction (not required and modifies data).
 * - Changed function prototypes slightly
 */

#pragma once

#include <stdint.h>

typedef struct _COBSState
{
    uint8_t code;
    uint8_t* pOut;
    const uint8_t* pOutOrig;
    uint8_t* pCode;
} COBSState;

/**
 * Calculate maximum size of stuffed data in worst-case.
 */
#define COBSMaxStuffedSize(size) (size+size/208+1)

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Stuff bytes and remove zeros.
 *
 * @param pIn Data to be stuffed.
 * @param sizeIn Input data size.
 * @param pOut Stuffed output data.
 * @param sizeOut Maximum output size.
 * @param pBytesWritten Actual size of stuffed data, pass 0 if not needed.
 *
 * @return 0 on success
 */
int16_t COBSEncode(const uint8_t* pIn, uint32_t sizeIn, uint8_t* pOut, uint32_t sizeOut, uint32_t* pBytesWritten);

/**
 * Prepare stuffing bytes and initialize state.
 *
 * @param pState Encoding state.
 * @param sizeIn Maximum data input size.
 * @param pOut Stuffed output data.
 * @param sizeOut Maximum output size.
 *
 * @return 0 on success
 */
int16_t COBSEncodeStart(COBSState* pState, uint32_t sizeIn, uint8_t* pOut, uint32_t sizeOut);

/**
 * Encode a single byte.
 *
 * @param pState Encoding state.
 * @param c Byte to encode.
 */
void COBSEncodeAddByte(COBSState* pState, uint8_t c);

/**
 * Encode multiple bytes.
 *
 * @param pState Encoding state.
 * @param pData Data to encode.
 * @param dataSize Number of bytes to encode from pData.
 */
void COBSEncodeAddData(COBSState* pState, const void* const pData, uint32_t dataSize);

/**
 * Finalize encoding process.
 *
 * @param pState Encoding state.
 * @param pBytesWritten Actual size of stuffed data, pass 0 if not needed.
 */
void COBSEncodeFinalize(COBSState* pState, uint32_t* pBytesWritten);

/**
 * Unstuff COBS data.
 *
 * @param pIn Stuffed data.
 * @param sizeIn Stuffed data size.
 * @param pOut Unstuffed data.
 * @param sizeOut Maximum output size.
 * @param pBytesWritten Actual unstuffed data size.
 *
 * @return 0 on success
 */
int16_t COBSDecode(const uint8_t* pIn, uint32_t sizeIn, uint8_t* pOut, uint32_t sizeOut, uint32_t* pBytesWritten);

#ifdef __cplusplus
}
#endif
