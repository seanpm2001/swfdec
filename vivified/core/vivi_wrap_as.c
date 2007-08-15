/* Vivified
 * Copyright (C) 2007 Benjamin Otte <otte@gnome.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, 
 * Boston, MA  02110-1301  USA
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "vivi_wrap.h"
#include "vivi_application.h"
#include "vivi_function.h"

VIVI_FUNCTION ("wrap_toString", vivi_wrap_toString)
void
vivi_wrap_toString (SwfdecAsContext *cx, SwfdecAsObject *this,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *retval)
{
  ViviWrap *wrap;
  char *s;

  if (!VIVI_IS_WRAP (this))
    return;
  
  wrap = VIVI_WRAP (this);
  if (wrap->wrap == NULL)
    return;
  
  s = swfdec_as_object_get_debug (wrap->wrap);
  SWFDEC_AS_VALUE_SET_STRING (retval, swfdec_as_context_give_string (cx, s));
}

/*** FRAME specific code ***/

VIVI_FUNCTION ("frame_name_get", vivi_wrap_name_get)
void
vivi_wrap_name_get (SwfdecAsContext *cx, SwfdecAsObject *this,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *retval)
{
  ViviWrap *wrap;
  const char *s;

  g_print ("1\n");
  if (!VIVI_IS_WRAP (this))
    return;
  
  g_print ("2\n");
  wrap = VIVI_WRAP (this);
  if (!SWFDEC_IS_AS_FRAME (wrap->wrap))
    return;
  
  g_print ("3\n");
  s = swfdec_as_frame_get_function_name (SWFDEC_AS_FRAME (wrap->wrap));
  SWFDEC_AS_VALUE_SET_STRING (retval, swfdec_as_context_get_string (cx, s));
}


