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

#include <stdlib.h>
#include <smartkeys.h>
#include <stdio.h>
#include <string.h>
#include <msx.h>
#include <eos.h>
#include "ade_io.h"

#define DISPLAY_ROWS 17
#define DISPLAY_LENGTH 60
#define ADDR_PATH_LINE MODE2_ATTR
#define ATTR_PATH_LINE 0xF5
#define ADDR_FILE_LIST ADDR_PATH_LINE + 256
#define ATTR_FILE_LIST 0x1F
#define ADDR_FILE_TYPE ADDR_FILE_LIST
#define ATTR_FILE_TYPE 0xF5

extern ADEInfo ade_info; // ade_io.h

static unsigned char *blocks = (unsigned char *)0x8000; // store blocks here
static unsigned char page=0;
static unsigned char num_pages=0;
static unsigned char offset=0;
static unsigned char fileno=0;
static unsigned char key=0;
static unsigned char joystick=0;
static unsigned char joystick_copy=0;
static unsigned char button=0;
static unsigned char button_copy=0;
static unsigned char keypad=0;
static unsigned char keypad_copy=0;
static unsigned char repeat=0;
static unsigned char dir_loaded=false;
static unsigned char repaginate=false;
static GameControllerData cont;
static unsigned char out[64];
static unsigned char file_on_clipboard=false;
static unsigned char visible_entries=0;

static void select_sound_chime(void)
{
  smartkeys_sound_play(SOUND_POSITIVE_CHIME);
}

static void select_sound_buzz(void)
{
  smartkeys_sound_play(SOUND_NEGATIVE_BUZZ);
}

static void select_sound_keypress(void)
{
  smartkeys_sound_play(SOUND_KEY_PRESS);
}

static void select_sound_mode_change(void)
{
  smartkeys_sound_play(SOUND_MODE_CHANGE);
}

static void select_sound_confirm(void)
{
  smartkeys_sound_play(SOUND_CONFIRM);
}

static void select_clear_bottom(void)
{
  msx_vfill(0x1200,0x00,768);
}

static unsigned char select_input_line(char *c)
{
  unsigned char w=0;
  unsigned char pos=0;
  char *begin=c;
  
  select_clear_bottom();
  
  while (key = eos_read_keyboard())
    {
      select_sound_keypress();
      if ((key == 0x08) && (pos > 0)) // because smartkeys_puts doesn't clear pixels, we force a full repaginate.
	{
	  pos--;
	  c--;
	  *c = 0;
	  select_clear_bottom();
	  smartkeys_puts(0,152,begin);  
	}
      else if (key == 0x1B)
	return 0;
      else if (key == 0x0D)
	return 1;
      else
	{
	  w += smartkeys_putc(w,152,key);
	  *c = key;
	  c++;
	  pos++;
	}
    }
}

static void select_load_directory(void)
{
  unsigned short b=1;

  blocks = (unsigned char *)0x8000;

  memset(blocks,0x00,19456);
  num_pages = 0;
  
  smartkeys_display(NULL,NULL,NULL,NULL,NULL,NULL);
  smartkeys_status("  LOADING DIRECTORY.");

  
  do
    {
      ade_io_read_current_directory(DISPLAY_LENGTH,b++,blocks);
      if (*blocks != 0x00)
	{
	  num_pages++;
	  blocks+=1024;
	}
      else
	break;
    }
  while (num_pages < 19);

  page=offset=0;

  blocks = (unsigned char *)0x8000;
}

static void select_paint_attrs(void)
{
  // Paint path line
  msx_vfill(ADDR_PATH_LINE,ATTR_PATH_LINE,256);

  // Paint file list 
  msx_vfill(ADDR_FILE_LIST,ATTR_FILE_LIST,4352);

  // Paint type column
  msx_vfill_v(ADDR_FILE_TYPE,ATTR_FILE_TYPE,136);
  msx_vfill_v(ADDR_FILE_TYPE+8,ATTR_FILE_TYPE,136);
  
}

static char select_icon(char p)
{
  switch(p)
    {
    case 3:
      return 3;
    case 10:
      return 0x0C;
    case 11:
      return 0x0B;
    case 12:
      return 0x0D;
    default:
      return 0x20;
    }
}

static unsigned char select_file_type(void)
{
  return &blocks[(page*1024)+(offset*DISPLAY_LENGTH)];
}

static void select_display_block(void)
{
  unsigned char *p;
  
  visible_entries=0;
  
  for (unsigned char i=0;i<DISPLAY_ROWS;i++)
    {      
      p = &blocks[(page*1024)+(i*DISPLAY_LENGTH)];
      if (p[1] != 0x00)
	{
	  msx_color(15,5,7);
	  smartkeys_putc(3,(i<<3)+8,select_icon(*p++));
	  msx_color(1,15,7);
	  smartkeys_puts(16,(i<<3)+8,(const char *)p);
	  visible_entries++;
	}
    }

  if (visible_entries == 0)
    key = 0xA2; // go back.
}

static void select_display_path(void)
{
  msx_color(15,5,7);
  smartkeys_puts(0,0,ade_info.current_path);
}

static void select_update_selection(void)
{
  // Paint file list
  for (int i=0;i<17;i++)
    msx_vfill(ADDR_FILE_LIST + (i << 8) + 16, ATTR_FILE_LIST, 240);
  
  // Paint selection
  msx_vfill(ADDR_FILE_LIST+(offset<<8) + 16,0x13,240);
}

static void select_page_up(void)
{
  if (page>0)
    {
      page--;
    }
}

static void select_page_down(void)
{
  if (page<num_pages)
    {
      page++;
    }
}

static void select_up(void)
{
  if (offset==0 && page > 0)
    {
      if (offset == 0)
	offset = 16;
      select_page_up();
      repaginate=true;
    }
  else
    {
      offset--;
      select_update_selection();
    }
}

static void select_down(void)
{
  if (offset==16)
    {
      offset=0;
      select_page_down();
      repaginate=true;
    }
  else
    {
      offset++;

      if (offset == visible_entries)
	offset--;
      
      select_update_selection();
    }
}

static void select_input(void)
{  
  key = eos_end_read_keyboard();
  
  if (key > 1)
    {
      eos_start_read_keyboard();
      return;
    }
  else
    {
      eos_read_game_controller(0x03,&cont);
      joystick = cont.joystick1 | cont.joystick2;
      button = cont.joystick1_button_left | cont.joystick1_button_right | cont.joystick2_button_left | cont.joystick2_button_right;
      keypad = cont.joystick1_keypad;

      if ((button > 0) && (button != button_copy))
	{
	  key=0x0d; // same as RETURN
	}
      else if ((keypad != 0x0F) && (keypad != keypad_copy))
	{
	  switch (keypad)
	    {
	    case 1: // Slot 1
	      key=0x83;
	      break;
	    case 2: // Slot 2
	      key=0x84;
	      break;
	    case 3: // Slot 3
	      key=0x85;
	      break;
	    case 4: // Slot 4
	      key=0x86;
	      break;
	    }
	}
      else if ((joystick > 0) && (joystick == joystick_copy))
	repeat++;
      else
	repeat=0;

      if (repeat > 16)
	{
	  repeat=0;
	  switch(joystick)
	    {
	    case 1: // UP
	      key=0xA0;
	      break;
	    case 4: // DOWN
	      key=0xA2;
	      break;
	    }
	}
      
      joystick_copy = joystick;
      button_copy = button;
      keypad_copy = keypad;
    }

}

static void select_reboot(void)
{
  eos_init();
}

static void select_smartkeys(void)
{
  select_sound_mode_change();
  smartkeys_display(NULL,NULL,"   D1:","   D2:","   D3:","   D4:");
  smartkeys_status("  BOOT IMAGE\n  WITH RETURN\n  OR PUT IN SLOT:");
}

static void select_status(const char *c)
{
  select_sound_confirm();
  smartkeys_display(NULL,NULL,NULL,NULL,NULL,NULL);
  smartkeys_status(c);
  sleep(2);
}

static void select_mount(unsigned char dev)
{
  ade_io_mount(dev,fileno);  
}

static void select_return(void)
{
  select_mount(4);

  if (blocks[(page*1024)+offset*DISPLAY_LENGTH] == 0x03)
    {
      strcpy(ade_info.current_path,blocks[(page*1024)+offset*DISPLAY_LENGTH]);
      repaginate=true;
      dir_loaded=false;
    }
  else
    select_reboot();
}

static void select_clear(void)
{
  msx_vfill(0x0100,0x00,4352);
  msx_vfill_v(ADDR_FILE_TYPE,ATTR_FILE_TYPE,136);
  msx_vfill_v(ADDR_FILE_TYPE+8,ATTR_FILE_TYPE,136);
  repaginate=false;
}

static void select_operation_aborted(void)
{
  select_status("OPERATION ABORTED");
  eos_end_read_keyboard();
  key=0;
}

static void select_clear_slot(void)
{
  smartkeys_display(NULL,"  ALL","   D1:","   D2:","   D3:","   D4:");
  smartkeys_status("  CLEAR\n  WHICH?");

  key=eos_read_keyboard();
  
  switch(key)
    {
    case 0x82:
    case 0x96:
      ade_io_mount(4,0);
      ade_io_mount(5,0);
      ade_io_mount(6,0);
      ade_io_mount(7,0);
      select_status("ALL SLOTS CLEARED.");
      break;
    case 0x83:
      ade_io_mount(4,0);
      select_status("SLOT 1 CLEARED.");
      break;
    case 0x84:
      ade_io_mount(5,0);
      select_status("SLOT 2 CLEARED.");
      break;
    case 0x85:
      ade_io_mount(6,0);
      select_status("SLOT 3 CLEARED.");
      break;
    case 0x86:
      ade_io_mount(7,0);
      select_status("SLOT 4 CLEARED.");
      break;
    default:
      select_operation_aborted();
      break;
    }
}

void select_print(void)
{
  unsigned char *p;

  select_sound_confirm();
  smartkeys_display(NULL,NULL,NULL,NULL,NULL," ABORT");
  smartkeys_status("  PRINTING FILE LIST...");

  for (unsigned char i=0;i<19;i++)
    {
      for (unsigned char j=0;j<17;j++)
	{
	  p=&blocks[(i*1024)+(j*DISPLAY_LENGTH)];
	  p++;
	 
	  select_clear_bottom();

	  if (*p != 0x00)
	    {
	      strcpy(out,(const char *)p);
	      strcat(out,"\r\n\x03");
	      smartkeys_puts(0,152,(const char *)p);
	      eos_print_buffer((const char *)out);
	    }
	}
    }

  select_smartkeys();
}
	 
void select_display_mount(unsigned char dev)
{
  select_clear_bottom();
  smartkeys_puts(0,152,ade_io_display_mount(dev));
}

void select_delete(void)
{
  select_sound_chime();
  smartkeys_display(NULL,NULL,NULL,NULL,"  YES","   NO");
  smartkeys_status("  DELETE FILE\n  ARE YOU SURE?");

  key=eos_read_keyboard();

  switch (key)
    {
    case 'Y':
    case 'y':
    case 0x85:
      ade_io_delete(fileno);
      select_status("  FILE DELETED.");
      select_clear_bottom();
      dir_loaded=false;
      repaginate=true;
      break;
    default:
      select_operation_aborted();
      break;
    }
}

void select_format(void)
{
  select_sound_chime();
  smartkeys_display(NULL,NULL,NULL,NULL,"  YES","   NO");
  smartkeys_status("  FORMAT FILE\n  ARE YOU SURE?");

  key=eos_read_keyboard();

  switch (key)
    {
    case 'Y':
    case 'y':
    case 0x85:
      /* ade_io_delete(fileno); */
      select_status("  FILE FORMATTED.");
      select_clear_bottom();
      dir_loaded=false;
      repaginate=true;
      break;
    default:
      select_operation_aborted();
      break;
    }
}

void select_create(void)
{
  unsigned char t=0;
  unsigned short b=256;

  select_sound_chime();
  smartkeys_display(NULL,NULL,NULL,"   DIR","  DISK","   DDP");
  smartkeys_status("  CREATE\n  SELECT TYPE:");

  key=eos_read_keyboard();

  switch(key)
    {
    case 0x84:
      t=3;
      break;
    case 0x85:
      t=10;
      break;
    case 0x86:
      t=11;
      break;
    default:
      select_operation_aborted();
      return;
    }

  if (t==10)
    {
      select_sound_chime();
      smartkeys_display(NULL,"  160K","  320K","  640K"," 1440K"," CUSTOM\n(# BLOCKS)");
      smartkeys_status("  DISK\n  SIZE?");

      key=eos_read_keyboard();

      switch(key)
	{
	case 0x82:
	  b=160;
	  break;
	case 0x83:
	  b=320;
	  break;
	case 0x84:
	  b=640;
	  break;
	case 0x85:
	  b=1440;
	  break;
	case 0x86:
	  select_sound_chime();
	  smartkeys_display(NULL,NULL,NULL,NULL,NULL,NULL);
	  smartkeys_status("  ENTER # OF DESIRED 1K BLOCKS.");
	  
	  if (select_input_line(out) == 0)
	    {
	      select_operation_aborted();
	      return;
	    }
	  b=atoi(out);
	  break;
	default:
	  select_operation_aborted();
	  return;
	}
    }

  select_sound_chime();
  smartkeys_display(NULL,NULL,NULL,NULL,NULL,NULL);
  smartkeys_status("  ENTER DESIRED FILENAME (WITHOUT EXTENSION)");

  if (select_input_line(out) == 0)
    {
      select_operation_aborted();
      return;
    }


  select_status("  CREATING IMAGE");
  ade_io_create_image(out,b,t);
  select_status("  IMAGE CREATED");
    
  dir_loaded=false;
  repaginate=true;
}

void select_rename(void)
{
  select_sound_chime();
  smartkeys_display(NULL,NULL,NULL,NULL,NULL,NULL);
  smartkeys_status("  ENTER THE NEW FILENAME, WITHOUT EXTENSION.\n  OR ESCAPE TO ABORT.");

  if (select_input_line(out) == 0)
    {
      select_operation_aborted();
      return;
    }


  select_status("  RENAMING FILE.");
  ade_io_rename_image(out,fileno,blocks[fileno*DISPLAY_LENGTH]);
  select_status("  FILE RENAMED");
  dir_loaded=false;
  repaginate=true;
}

void select_cut(void)
{
  ade_io_clipboard(fileno,1);
  file_on_clipboard=true;
  select_status("  FILE CUT TO CLIPBOARD.\n  WHEN READY, PRESS STORE TO MOVE THE FILE.");
}

void select_copy(void)
{
  ade_io_clipboard(fileno,2);
  file_on_clipboard=true;
  select_status("  FILE COPIED TO CLIPBOARD.\n  WHEN READY, PRESS STORE TO COPY THE FILE.");
}

void select_paste(void)
{  
  if (file_on_clipboard==true)
    {
      ade_io_clipboard(fileno,3);
      select_status("FILE PASTED.");
    }
  else
    {
      select_status("  NOTHING ON CLIPBOARD.\n  USE MOVE OR COPY TO PLACE A FILE ON CLIPBOARD.");
    }
}

void select_cant_mount_folders(void)
{
  select_sound_buzz();
  select_status("CAN'T MOUNT FOLDERS TO DRIVES. IGNORING.");
}

void select(void)
{

  select_clear();
  select_paint_attrs();

  if (dir_loaded==false)
    {
      select_display_path();
      select_load_directory();
      dir_loaded=true;
    }

  select_display_block();
  select_update_selection();
  select_smartkeys();  
  eos_start_read_keyboard();
    
  while (1)
    {
      select_input();
      fileno=((page*DISPLAY_ROWS)+offset)+1;
      
      switch(key)
	{
	case 0x0D:
	  select_return();
	  repaginate=true;
	  break;
	case 0x1B: // ESCAPE/WP
	  eos_exit_to_smartwriter();
	  break;
	case 0x80: // HOME
	  page=offset=0;
	  repaginate=true;
	  break;
	case 0x83: // III
	  if (select_file_type() > 9)
	    {
	      select_mount(4);
	      select_status("  MOUNTED TO D1:");
	    }
	  else
	    select_cant_mount_folders();
	  select_smartkeys();
	  break;
	case 0x84: // IV
	  if (select_file_type() > 9)
	    {
	      select_mount(5);
	      select_status("  MOUNTED TO D2:");
	    }
	  else
	    select_cant_mount_folders();
	  select_smartkeys();
	  break;
	case 0x85: // V
	  if (select_file_type() > 9)
	    {
	      select_mount(6);
	      select_status("  MOUNTED TO D3:");
	    }
	  else
	    select_cant_mount_folders();
	  select_smartkeys();
	  break;
	case 0x86: // VI
	  if (select_file_type() > 9)
	    {
	      select_mount(7);
	      select_status("  MOUNTED TO D4:");
	    }
	  else
	    select_cant_mount_folders();
	  select_smartkeys();
	  break;
	case 0x8B: // SHIFT-III
	  select_display_mount(4);
	  break;
	case 0x8C: // SHIFT-IV
	  select_display_mount(5);
	  break;
	case 0x8D: // SHIFT-V
	  select_display_mount(6);
	  break;
	case 0x8E: // SHIFT-VI
	  select_display_mount(7);
	  break;
	case 0x90: // WILD CARD
	  select_rename();
	  break;
	case 0x92: // COPY
	  select_copy();
	  break;
	case 0x94: // INSERT
	  select_create();
	  select_clear_bottom();
	  select_smartkeys();
	  break;
	case 0x95: // PRINT
	  select_print();
	  select_clear_bottom();
	  select_smartkeys();
	  break;
	case 0x97: // DELETE
	  select_delete();
	  select_clear_bottom();
	  select_smartkeys();
	  break;
	case 0x96: // CLEAR
	  select_clear_slot();
	  select_clear_bottom();
	  select_smartkeys();
	  break;
	case 0x9A: // MOVE
	  select_cut();
	  select_smartkeys();
	  break;
	case 0x9B: // STORE
	  select_paste();
	  select_smartkeys();
	  break;
	case 0x9C:
	  select_format();
	  select_smartkeys();
	  break;
	case 0xA0: // UP
	  select_up();
	  break;
	case 0xA2: // DOWN
	  select_down();
	  break;
	case 0xA4: // CTRL-UP
	  offset=0;
	  select_page_up();
	  repaginate=true;
	  break;
	case 0xA6: // CTRL-DOWN
	  offset=0;
	  select_page_down();
	  repaginate=true;
	  break;
	default:
	  if ((key > 1) && (key != 0x1E) && (key != 0x2A))
	    select_sound_buzz();
	  break;
	}

      if (repaginate==true)
	break; // fall through to main()

      eos_end_read_keyboard();
      eos_start_read_keyboard();
      key=0;
    }
  
}
