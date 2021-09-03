/**
 * vhs-head - an ADE front-end
 *
 * @author Thomas Cherryhomes
 * @email thom.cherryhomes@gmail.com
 *
 * This is licensed under the
 * GNU Public License v.3
 * See LICENSE.md, for details.
 */

#include <stdbool.h>

#ifndef ADE_IO_H
#define ADE_IO_H

typedef struct _ade_info
{
  unsigned char ade[3]; // The ADE ID string
  unsigned char device_id; // The display device #
  bool device_4_enabled; // Device 4 enabled?
  bool device_5_enabled; // Device 5 enabled?
  bool device_6_enabled; // Device 6 enabled?
  bool device_7_enabled; // device 7 enabled?
  unsigned char ver[3]; // Version #
  unsigned char lcd_len; // LC name length
  unsigned short num_files; // # of files in current directory
  unsigned char reserved[6]; // Reserved bytes
  unsigned char current_path[256]; // Current file path
} ADEInfo;

/**
 * @brief enable ADE extended mode
 */
void ade_io_enable_extended_mode(void);

/**
 * @brief read current directory block
 * @param len - Length of each directory entry
 * @param blockno - The block # to request
 * @param buf - destination buffer
 * @return errno, and block data in block global variable
 */
unsigned char ade_io_read_current_directory(unsigned char len, unsigned char blockno, void *buf);

/**
 * @brief Return currently mounted image for dev
 * @param dev - Device (4-7)
 * @return path at device
 */
char *ade_io_display_mount(unsigned char dev);

/**
 * @brief mount file number into device ID
 * @param dev - Device ID (4,5,6,7)
 * @param fileno - File #
 * @return errno
 */
unsigned char ade_io_mount(unsigned char dev, unsigned short fileno);

/**
 * @brief delete file referenced by fileno
 * @param fileno - File #
 * @return errno
 */
unsigned char ade_io_delete(unsigned short fileno);

/**
 * @brief create image of specified type and number of blocks
 * @param filename of image to create
 * @param # of blocks - image size requested (0-16383)
 * @param type - Image type to create
 * @return error code
 */
unsigned char ade_io_create_image(const char *filename, unsigned short b, unsigned char t);

/**
 * @brief create image of specified type
 * @param filename of image to create
 * @param # of blocks - image size requested (0-16383)
 * @param type - Image type to create
 * @return error code
 */
unsigned char ade_io_create_image(const char *filename, unsigned short b, unsigned char t);

/**
 * @brief rename file
 * @param new filename
 * @param file number of existing file
 * @param type - Image type to create
 * @return error code
 */
unsigned char ade_io_rename_image(const char *filename, unsigned short fileno, unsigned char t);

/**
 * @brief cut fileno onto clipboard
 * @param file number
 * @param operation, 1 = cut, 2 = copy, 3 = paste, 4 = clear
 * @return error if any
 */
unsigned char ade_io_clipboard(unsigned short fileno, unsigned char op);

#endif /* ADE_IO_H */
