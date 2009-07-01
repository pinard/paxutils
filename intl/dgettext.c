/* dgettext.c -- implementation of the dgettext(3) function
   Copyright (C) 1995 Software Foundation, Inc.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#ifdef HAVE_LOCALE_H
# include <locale.h>
#endif

#include "libgettext.h"

/* @@ end of prolog @@ */

#undef dgettext

/* Look up MSGID in the DOMAINNAME message catalog of the current
   LC_MESSAGES locale.  */
char *
dgettext (domainname, msgid)
     const char *domainname;
     const char *msgid;
{
  return dcgettext (domainname, msgid, LC_MESSAGES);
}
