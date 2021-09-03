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

#include <eos.h>
#include <string.h>
#include <smartkeys.h>
#include "ade_io.h"

ADEInfo ade_info; // defined in ade_io.h
unsigned char errno;

static unsigned char block[1024];

void ade_io_enable_extended_mode(void)
{
  errno=eos_read_block(4,0xDEADBEEF,&block); // Request block #
  memcpy(&ade_info,&block,sizeof(ade_info));
  ade_info.num_files = (ade_info.num_files>>8) | (ade_info.num_files<<8); // Byte swap.
  return errno;
}

unsigned char ade_io_read_current_directory(unsigned char len, unsigned char blockno, void *buf)
{
  return eos_read_block(4,(0xF1010000 | (len << 8) | blockno),buf);
}

char *ade_io_display_mount(unsigned char dev)
{
  unsigned char *p;
  eos_read_block(dev,0xF2000000,&block);
  p = block;
  p += 2;
  return p;
}

unsigned char ade_io_mount(unsigned char dev, unsigned short fileno)
{
  return eos_read_block(dev,0xF3010000 | fileno,&block);
}

unsigned char ade_io_delete(unsigned short fileno)
{
  eos_read_block(4,0xF6000000 | fileno,&block);
  return eos_read_block(4,0xF6F60000 | fileno,&block);
}

unsigned char ade_io_create_image(const char *filename, unsigned short b, unsigned char t)
{
  char *p = (char *)&block[3];
    
  memset(block,0,sizeof(block));

  block[0]=t;
  block[1]=b&0xFF;
  block[2]=b>>8;
  
  strcpy(p,filename);
  
  return eos_write_block(4,0xF7000000,&block);
}

unsigned char ade_io_rename_image(const char *filename, unsigned short fileno, unsigned char t)
{
  char *p = (char *)&block[3];
    
  memset(block,0,sizeof(block));

  block[0]=t;
  block[1]=fileno&0xFF;
  block[2]=fileno>>8;
  
  strcpy(p,filename);
  
  return eos_write_block(4,0xF8000000,&block);  
}

unsigned char ade_io_clipboard(unsigned short fileno, unsigned char op)
{
  return eos_read_block(4,0xF9000000 | (op << 16) | fileno,&block);
}

unsigned char ade_io_save_config(void)
{
}
