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

#ifndef _SWFDEC_SANDBOX_H_
#define _SWFDEC_SANDBOX_H_

#include <libswfdec/swfdec_as_object.h>
#include <libswfdec/swfdec_url.h>
#include <libswfdec/swfdec_player.h>

G_BEGIN_DECLS

typedef struct _SwfdecSandbox SwfdecSandbox;
typedef struct _SwfdecSandboxClass SwfdecSandboxClass;

typedef enum {
  SWFDEC_SANDBOX_NONE,
  SWFDEC_SANDBOX_REMOTE,
  SWFDEC_SANDBOX_LOCAL_FILE,
  SWFDEC_SANDBOX_LOCAL_NETWORK,
  SWFDEC_SANDBOX_LOCAL_TRUSTED
} SwfdecSandboxType;

#define SWFDEC_TYPE_SANDBOX                    (swfdec_sandbox_get_type())
#define SWFDEC_IS_SANDBOX(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_SANDBOX))
#define SWFDEC_IS_SANDBOX_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_SANDBOX))
#define SWFDEC_SANDBOX(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_SANDBOX, SwfdecSandbox))
#define SWFDEC_SANDBOX_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_SANDBOX, SwfdecSandboxClass))
#define SWFDEC_SANDBOX_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), SWFDEC_TYPE_SANDBOX, SwfdecSandboxClass))

struct _SwfdecSandbox
{
  SwfdecAsObject      	object;

  SwfdecSandboxType	type;		/* type of this sandbox */
  SwfdecURL *		url;		/* URL this sandbox acts for */
};

struct _SwfdecSandboxClass
{
  SwfdecAsObjectClass 	object_class;
};

GType			swfdec_sandbox_get_type		(void);

SwfdecSandbox *		swfdec_sandbox_get_for_url	(SwfdecPlayer *	  	player,
							 const SwfdecURL *	url);

gboolean		swfdec_sandbox_set_allow_network(SwfdecSandbox *	sandbox,
							 gboolean		network);

void			swfdec_sandbox_use		(SwfdecSandbox *	sandbox);
void			swfdec_sandbox_unuse		(SwfdecSandbox *	sandbox);

G_END_DECLS
#endif
