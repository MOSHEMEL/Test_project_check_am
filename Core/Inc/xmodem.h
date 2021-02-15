/**
 * @file    xmodem.h
 * @author  Ferenc Nemeth
 * @date    21 Dec 2018
 * @brief   This module is the implementation of the Xmodem protocol.
 *
 *          Copyright (c) 2018 Ferenc Nemeth - https://github.com/ferenc-nemeth
 */

#ifndef XMODEM_H_
#define XMODEM_H_

#include "flash_if.h"
#include "stdbool.h"

/* Xmodem (128 bytes) packet format
 * Byte  0:       Header
 * Byte  1:       Packet number
 * Byte  2:       Packet number complement
 * Bytes 3-130:   Data
 * Bytes 131-132: CRC
 */

/* Xmodem (1024 bytes) packet format
 * Byte  0:         Header
 * Byte  1:         Packet number
 * Byte  2:         Packet number complement
 * Bytes 3-1026:    Data
 * Bytes 1027-1028: CRC
 */

/* Maximum allowed errors (user defined). */
#define X_MAX_ERRORS ((uint8_t)3u)

/* Sizes of the packets. */
#define X_PACKET_128_SIZE   ((uint16_t)128u)
#define X_PACKET_1024_SIZE  ((uint16_t)1024u)
#define X_PACKET_CRC_SIZE   ((uint16_t)2u)

/* Relative (not counting the header) location of the packets. */
#define X_PACKET_NUMBER_INDEX             ((uint16_t)0u)
#define X_PACKET_NUMBER_COMPLEMENT_INDEX  ((uint16_t)1u)
#define X_PACKET_DATA_INDEX               ((uint16_t)2u)

/* Bytes defined by the protocol. */
#define X_SOH ((uint8_t)0x01u)  /**< Start Of Header (128 bytes). */
#define X_STX ((uint8_t)0x02u)  /**< Start Of Header (1024 bytes). */
#define X_EOT ((uint8_t)0x04u)  /**< End Of Transmission. */
#define X_ACK ((uint8_t)0x06u)  /**< Acknowledge. */
#define X_NAK ((uint8_t)0x15u)  /**< Not Acknowledge. */
#define X_CAN ((uint8_t)0x18u)  /**< Cancel. */
#define X_C   ((uint8_t)0x43u)  /**< ASCII "C", to notify the host, we want to use CRC16. */

/* Status report for the functions. */
typedef enum {
  X_OK            = 0x00u, /**< The action was successful. */
  X_ERROR_CRC     = 0x01u, /**< CRC calculation error. */
  X_ERROR_NUMBER  = 0x02u, /**< Packet number mismatch error. */
  X_ERROR_UART    = 0x04u, /**< UART communication error. */
  X_ERROR_FLASH   = 0x06u, /**< Flash related error. */
  X_ERROR         = 0xFFu  /**< Generic error. */
} xmodem_status;

/* Packet structure defines */
#define PACKET_HEADER_SIZE      ((uint32_t)3)
#define PACKET_DATA_INDEX       ((uint32_t)3) // TODO: Fix, maybe.
#define PACKET_START_INDEX      ((uint32_t)0)
#define PACKET_NUMBER_INDEX     ((uint32_t)1)
#define PACKET_CNUMBER_INDEX    ((uint32_t)2)
#define PACKET_TRAILER_SIZE     ((uint32_t)2)
#define PACKET_OVERHEAD_SIZE    (PACKET_HEADER_SIZE + PACKET_TRAILER_SIZE - 1)
#define PACKET_SIZE             ((uint32_t)128)
#define PACKET_1K_SIZE          ((uint32_t)1024)

/* /-------- Packet in IAP memory ------------------------------------------\
 * | 0      |  1    |  2     |  3   |  4      | ... | n+4     | n+5  | n+6  | 
 * |------------------------------------------------------------------------|
 * | unused | start | number | !num | data[0] | ... | data[n] | crc0 | crc1 |
 * \------------------------------------------------------------------------/
 * the first byte is left unused for memory alignment reasons                 */

#define FILE_NAME_LENGTH        ((uint32_t)64)
#define FILE_SIZE_LENGTH        ((uint32_t)16)

#define SOH                     ((uint8_t)0x01)  /* start of 128-byte data packet */
#define STX                     ((uint8_t)0x02)  /* start of 1024-byte data packet */
#define EOT                     ((uint8_t)0x04)  /* end of transmission */
#define ACK                     ((uint8_t)0x06)  /* acknowledge */
#define NAK                     ((uint8_t)0x15)  /* negative acknowledge */
#define CA                      ((uint8_t)0x18) /* two of these in succession aborts transfer */
#define CRC16                   ((uint8_t)0x43)  /* 'C' == 0x43, request 16-bit CRC */
#define NEGATIVE_BYTE           ((uint8_t)0xFF)

#define ABORT1                  ((uint8_t)0x41)  /* 'A' == 0x41, abort by user */
#define ABORT2                  ((uint8_t)0x61)  /* 'a' == 0x61, abort by user */

#define NAK_TIMEOUT             ((uint32_t)0x100000)
#define DOWNLOAD_TIMEOUT        ((uint32_t)1000) /* One second retry delay */
#define MAX_ERRORS              ((uint32_t)50)
void xmodem_receive(void);
uint16_t xmodem_calc_crc(uint8_t* data, uint16_t length);

#endif /* XMODEM_H_ */
