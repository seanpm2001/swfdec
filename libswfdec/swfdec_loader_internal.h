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

#ifndef _SWFDEC_LOADER_INTERNAL_H_
#define _SWFDEC_LOADER_INTERNAL_H_

#include "swfdec_loader.h"
#include "swfdec_loadertarget.h"
#include "swfdec_stream_target.h"

G_BEGIN_DECLS

typedef enum {
  SWFDEC_LOADER_STATE_NEW = 0,		/* loader is new and has not been opened yet */
  SWFDEC_LOADER_STATE_OPEN,		/* loader is opened and has got the HTTP headers */
  SWFDEC_LOADER_STATE_READING,		/* loader has read some bytes of data and is still reading */
  SWFDEC_LOADER_STATE_EOF,		/* swfdec_loader_eof() has been called */
  SWFDEC_LOADER_STATE_CLOSED,		/* loader has been closed by Swfdec */
  SWFDEC_LOADER_STATE_ERROR		/* loader is in error state */
} SwfdecLoaderState;

/* swfdec_stream.c */
const char *		swfdec_stream_describe		(SwfdecStream *		stream);
void			swfdec_stream_close		(SwfdecStream *		stream);
void			swfdec_stream_set_target	(SwfdecStream *		stream,
							 SwfdecStreamTarget *	target);

/* swfdec_loader.c */
SwfdecLoader *		swfdec_loader_load		(SwfdecLoader *		loader,
							 const SwfdecURL *    	url,
							 SwfdecLoaderRequest	request,
							 const char *		data,
							 gsize			data_len);
void			swfdec_loader_close		(SwfdecLoader *		loader);
void			swfdec_loader_set_target	(SwfdecLoader *		loader,
							 SwfdecLoaderTarget *	target);
void			swfdec_loader_set_data_type	(SwfdecLoader *		loader,
							 SwfdecLoaderDataType	type);
char *			swfdec_loader_get_text		(SwfdecLoader *		loader,
							 guint			version);

void			swfdec_string_append_urlencoded	(GString *		str,
							 const char *		name,
							 const char *		value);

G_END_DECLS
#endif
