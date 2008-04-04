/* Swfdec
 * Copyright (C) 2007 Pekka Lampila <pekka.lampila@iki.fi>
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

#include "swfdec_as_internal.h"
#include "swfdec_debug.h"

// static
SWFDEC_AS_NATIVE (1100, 40, swfdec_bitmap_data_loadBitmap)
void
swfdec_bitmap_data_loadBitmap (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("BitmapData.loadBitmap (static)");
}

// properties
SWFDEC_AS_NATIVE (1100, 100, swfdec_bitmap_data_get_width)
void
swfdec_bitmap_data_get_width (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("BitmapData.width (get)");
}

SWFDEC_AS_NATIVE (1100, 101, swfdec_bitmap_data_set_width)
void
swfdec_bitmap_data_set_width (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("BitmapData.width (set)");
}

SWFDEC_AS_NATIVE (1100, 102, swfdec_bitmap_data_get_height)
void
swfdec_bitmap_data_get_height (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("BitmapData.height (get)");
}

SWFDEC_AS_NATIVE (1100, 103, swfdec_bitmap_data_set_height)
void
swfdec_bitmap_data_set_height (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("BitmapData.height (set)");
}

SWFDEC_AS_NATIVE (1100, 104, swfdec_bitmap_data_get_rectangle)
void
swfdec_bitmap_data_get_rectangle (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("BitmapData.rectangle (get)");
}

SWFDEC_AS_NATIVE (1100, 105, swfdec_bitmap_data_set_rectangle)
void
swfdec_bitmap_data_set_rectangle (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("BitmapData.rectangle (set)");
}

SWFDEC_AS_NATIVE (1100, 106, swfdec_bitmap_data_get_transparent)
void
swfdec_bitmap_data_get_transparent (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SWFDEC_STUB ("BitmapData.transparent (get)");
}

SWFDEC_AS_NATIVE (1100, 107, swfdec_bitmap_data_set_transparent)
void
swfdec_bitmap_data_set_transparent (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SWFDEC_STUB ("BitmapData.transparent (set)");
}

// normal
SWFDEC_AS_NATIVE (1100, 1, swfdec_bitmap_data_getPixel)
void
swfdec_bitmap_data_getPixel (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("BitmapData.getPixel");
}

SWFDEC_AS_NATIVE (1100, 2, swfdec_bitmap_data_setPixel)
void
swfdec_bitmap_data_setPixel (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("BitmapData.setPixel");
}

SWFDEC_AS_NATIVE (1100, 3, swfdec_bitmap_data_fillRect)
void
swfdec_bitmap_data_fillRect (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("BitmapData.fillRect");
}

SWFDEC_AS_NATIVE (1100, 4, swfdec_bitmap_data_copyPixels)
void
swfdec_bitmap_data_copyPixels (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("BitmapData.copyPixels");
}

SWFDEC_AS_NATIVE (1100, 5, swfdec_bitmap_data_applyFilter)
void
swfdec_bitmap_data_applyFilter (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("BitmapData.applyFilter");
}

SWFDEC_AS_NATIVE (1100, 6, swfdec_bitmap_data_scroll)
void
swfdec_bitmap_data_scroll (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("BitmapData.scroll");
}

SWFDEC_AS_NATIVE (1100, 7, swfdec_bitmap_data_threshold)
void
swfdec_bitmap_data_threshold (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("BitmapData.threshold");
}

SWFDEC_AS_NATIVE (1100, 8, swfdec_bitmap_data_draw)
void
swfdec_bitmap_data_draw (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("BitmapData.draw");
}

SWFDEC_AS_NATIVE (1100, 9, swfdec_bitmap_data_pixelDissolve)
void
swfdec_bitmap_data_pixelDissolve (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("BitmapData.pixelDissolve");
}

SWFDEC_AS_NATIVE (1100, 10, swfdec_bitmap_data_getPixel32)
void
swfdec_bitmap_data_getPixel32 (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("BitmapData.getPixel32");
}

SWFDEC_AS_NATIVE (1100, 11, swfdec_bitmap_data_setPixel32)
void
swfdec_bitmap_data_setPixel32 (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("BitmapData.setPixel32");
}

SWFDEC_AS_NATIVE (1100, 12, swfdec_bitmap_data_floodFill)
void
swfdec_bitmap_data_floodFill (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("BitmapData.floodFill");
}

SWFDEC_AS_NATIVE (1100, 13, swfdec_bitmap_data_getColorBoundsRect)
void
swfdec_bitmap_data_getColorBoundsRect (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SWFDEC_STUB ("BitmapData.getColorBoundsRect");
}

SWFDEC_AS_NATIVE (1100, 14, swfdec_bitmap_data_perlinNoise)
void
swfdec_bitmap_data_perlinNoise (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("BitmapData.perlinNoise");
}

SWFDEC_AS_NATIVE (1100, 15, swfdec_bitmap_data_colorTransform)
void
swfdec_bitmap_data_colorTransform (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("BitmapData.colorTransform");
}

SWFDEC_AS_NATIVE (1100, 16, swfdec_bitmap_data_hitTest)
void
swfdec_bitmap_data_hitTest (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("BitmapData.hitTest");
}

SWFDEC_AS_NATIVE (1100, 17, swfdec_bitmap_data_paletteMap)
void
swfdec_bitmap_data_paletteMap (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("BitmapData.paletteMap");
}

SWFDEC_AS_NATIVE (1100, 18, swfdec_bitmap_data_merge)
void
swfdec_bitmap_data_merge (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("BitmapData.merge");
}

SWFDEC_AS_NATIVE (1100, 19, swfdec_bitmap_data_noise)
void
swfdec_bitmap_data_noise (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("BitmapData.noise");
}

SWFDEC_AS_NATIVE (1100, 20, swfdec_bitmap_data_copyChannel)
void
swfdec_bitmap_data_copyChannel (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("BitmapData.copyChannel");
}

SWFDEC_AS_NATIVE (1100, 21, swfdec_bitmap_data_clone)
void
swfdec_bitmap_data_clone (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("BitmapData.clone");
}

SWFDEC_AS_NATIVE (1100, 22, swfdec_bitmap_data_dispose)
void
swfdec_bitmap_data_dispose (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("BitmapData.dispose");
}

SWFDEC_AS_NATIVE (1100, 23, swfdec_bitmap_data_generateFilterRect)
void
swfdec_bitmap_data_generateFilterRect (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SWFDEC_STUB ("BitmapData.generateFilterRect");
}

SWFDEC_AS_NATIVE (1100, 24, swfdec_bitmap_data_compare)
void
swfdec_bitmap_data_compare (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SWFDEC_STUB ("BitmapData.compare");
}

// constructor
SWFDEC_AS_NATIVE (1100, 0, swfdec_bitmap_data_construct)
void
swfdec_bitmap_data_construct (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("BitmapData");
}