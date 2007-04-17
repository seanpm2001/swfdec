/* Swfdec
 * Copyright (C) 2006-2007 Benjamin Otte <otte@gnome.org>
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

#include <pango/pangocairo.h>
#include <string.h>
#include "swfdec_edittext.h"
#include "swfdec_debug.h"
#include "swfdec_edittext_movie.h"
#include "swfdec_font.h"
#include "swfdec_player_internal.h"
#include "swfdec_swf_decoder.h"

G_DEFINE_TYPE (SwfdecEditText, swfdec_edit_text, SWFDEC_TYPE_GRAPHIC)

static gboolean
swfdec_edit_text_mouse_in (SwfdecGraphic *graphic, double x, double y)
{
  return swfdec_rect_contains (&graphic->extents, x, y);
}

static SwfdecMovie *
swfdec_edit_text_create_movie (SwfdecGraphic *graphic, gsize *size)
{
  SwfdecEditText *text = SWFDEC_EDIT_TEXT (graphic);
  SwfdecEditTextMovie *ret = g_object_new (SWFDEC_TYPE_EDIT_TEXT_MOVIE, NULL);

  ret->text = text;
  if (text->text)
    swfdec_edit_text_movie_set_text (ret, text->text);
  *size = sizeof (SwfdecEditTextMovie);

  return SWFDEC_MOVIE (ret);
}

static void
swfdec_edit_text_dispose (GObject *object)
{
  SwfdecEditText *text = SWFDEC_EDIT_TEXT (object);

  g_free (text->text);
  text->text = NULL;
  g_free (text->variable);
  text->variable = NULL;
  
  G_OBJECT_CLASS (swfdec_edit_text_parent_class)->dispose (object);
}

static void
swfdec_edit_text_class_init (SwfdecEditTextClass * g_class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (g_class);
  SwfdecGraphicClass *graphic_class = SWFDEC_GRAPHIC_CLASS (g_class);

  object_class->dispose = swfdec_edit_text_dispose;
  graphic_class->create_movie = swfdec_edit_text_create_movie;
  graphic_class->mouse_in = swfdec_edit_text_mouse_in;
}

static void
swfdec_edit_text_init (SwfdecEditText * text)
{
  text->max_length = G_MAXUINT;
}

int
tag_func_define_edit_text (SwfdecSwfDecoder * s)
{
  SwfdecEditText *text;
  guint id;
  int reserved, use_outlines;
  gboolean has_font, has_color, has_max_length, has_layout, has_text;
  SwfdecBits *b = &s->b;
  
  id = swfdec_bits_get_u16 (b);
  SWFDEC_LOG ("  id = %u", id);
  text = swfdec_swf_decoder_create_character (s, id, SWFDEC_TYPE_EDIT_TEXT);
  if (text == NULL)
    return SWFDEC_STATUS_OK;

  swfdec_bits_get_rect (b, &SWFDEC_GRAPHIC (text)->extents);
  SWFDEC_LOG ("  extents: %g %g  %g %g", 
      SWFDEC_GRAPHIC (text)->extents.x0, SWFDEC_GRAPHIC (text)->extents.y0,
      SWFDEC_GRAPHIC (text)->extents.x1, SWFDEC_GRAPHIC (text)->extents.y1);
  swfdec_bits_syncbits (b);
  has_text = swfdec_bits_getbit (b);
  text->wrap = swfdec_bits_getbit (b);
  text->multiline = swfdec_bits_getbit (b);
  text->password = swfdec_bits_getbit (b);
  text->readonly = swfdec_bits_getbit (b);
  has_color = swfdec_bits_getbit (b);
  has_max_length = swfdec_bits_getbit (b);
  has_font = swfdec_bits_getbit (b);
  reserved = swfdec_bits_getbit (b);
  text->autosize = swfdec_bits_getbit (b);
  has_layout = swfdec_bits_getbit (b);
  text->selectable = !swfdec_bits_getbit (b);
  text->border = swfdec_bits_getbit (b);
  reserved = swfdec_bits_getbit (b);
  text->html = swfdec_bits_getbit (b);
  use_outlines = swfdec_bits_getbit (b); /* FIXME: what's this? */
  if (has_font) {
    SwfdecCharacter *font;

    id = swfdec_bits_get_u16 (b);
    font = swfdec_swf_decoder_get_character (s, id);
    if (SWFDEC_IS_FONT (font)) {
      SWFDEC_LOG ("  font = %u", id);
      text->font = SWFDEC_FONT (font);
    } else {
      SWFDEC_ERROR ("id %u does not specify a font", id);
    }
    text->height = swfdec_bits_get_u16 (b);
    SWFDEC_LOG ("  height = %u", text->height);
  }
  if (has_color) {
    text->color = swfdec_bits_get_rgba (b);
    SWFDEC_LOG ("  color = %u", text->color);
  } else {
    SWFDEC_WARNING ("FIXME: figure out default color");
    text->color = SWFDEC_COLOR_COMBINE (255, 255, 255, 255);
  }
  if (has_max_length) {
    text->max_length = swfdec_bits_get_u16 (b);
  }
  if (has_layout) {
    guint align = swfdec_bits_get_u8 (b);
    switch (align) {
      case 0:
	text->align = PANGO_ALIGN_LEFT;
	break;
      case 1:
	text->align = PANGO_ALIGN_RIGHT;
	break;
      case 2:
	text->align = PANGO_ALIGN_CENTER;
	break;
      case 3:
	text->justify = TRUE;
	break;
      default:
	SWFDEC_ERROR ("undefined align value %u", align);
	break;
    }
    text->left_margin = swfdec_bits_get_u16 (b);
    text->right_margin = swfdec_bits_get_u16 (b);
    text->indent = swfdec_bits_get_u16 (b);
    text->spacing = swfdec_bits_get_s16 (b);
  }
  text->variable = swfdec_bits_get_string (b);
  if (has_text)
    text->text = swfdec_bits_get_string (b);

  return SWFDEC_STATUS_OK;
}
