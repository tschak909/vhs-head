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

#include <smartkeys.h>
#include <stdlib.h>
#include <string.h>
#include <eos.h>
#include "select.h"
#include "ade_io.h"
#include "error.h"

#define SELECT 0
#define COULD_NOT_FIND_ADE 1

unsigned char mode;    // current mode

extern ADEInfo ade_info;

void main(void)
{
  smartkeys_set_mode();
  ade_io_enable_extended_mode();
  smartkeys_sound_init();
  
  if (ade_info.ade[0] != 'A')
    mode=COULD_NOT_FIND_ADE;
  else
    mode=SELECT;
    
  while(1)
    {
      switch(mode)
	{
	case SELECT:
	  select();
	  break;
	case COULD_NOT_FIND_ADE:
	  error("COULD NOT FIND ADE. ABORTING.");
	  smartkeys_sound_play(8);
	  while (1) { } // hang.
	  break;
	}
    }
}
