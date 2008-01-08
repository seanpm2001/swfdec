/* Swfdec
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

#ifndef _SWFDEC_TEST_TEST_H_
#define _SWFDEC_TEST_TEST_H_

#include <libswfdec/swfdec.h>

G_BEGIN_DECLS


typedef struct _SwfdecTestTest SwfdecTestTest;
typedef struct _SwfdecTestTestClass SwfdecTestTestClass;

#define SWFDEC_TYPE_TEST_TEST                    (swfdec_test_test_get_type())
#define SWFDEC_IS_TEST_TEST(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_TEST_TEST))
#define SWFDEC_IS_TEST_TEST_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_TEST_TEST))
#define SWFDEC_TEST_TEST(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_TEST_TEST, SwfdecTestTest))
#define SWFDEC_TEST_TEST_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_TEST_TEST, SwfdecTestTestClass))
#define SWFDEC_TEST_TEST_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), SWFDEC_TYPE_TEST_TEST, SwfdecTestTestClass))

struct _SwfdecTestTest
{
  SwfdecAsObject	as_object;

  char *		filename;	/* file the player should be loaded from */
  SwfdecPlayer *	player;		/* the player or %NULL if none */
  gboolean		player_quit;	/* the player has called fscommand:quit */

  /* trace stuff */
  char *		trace_filename;	/* file we're parsing */
  SwfdecBuffer *	trace_buffer;	/* buffer containing the file */
  guchar *		trace_offset;	/* how far we've parsed the trace data */
  gboolean		trace_failed;	/* TRUE if the tacing failed */
  GSList *		trace_captured;	/* captured trace strings (newest first) */
};

struct _SwfdecTestTestClass
{
  SwfdecAsObjectClass	as_object_class;
};

GType		swfdec_test_test_get_type	(void);


G_END_DECLS
#endif
