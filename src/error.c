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

void error(const char *c)
{
  smartkeys_display(NULL,NULL,NULL,NULL,NULL,NULL);
  smartkeys_status(c);
}
