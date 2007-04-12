/* SwfdecAs
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

#include "swfdec_as_function.h"
#include "swfdec_as_context.h"
#include "swfdec_as_frame.h"
#include "swfdec_as_stack.h"
#include "swfdec_debug.h"

G_DEFINE_TYPE (SwfdecAsFunction, swfdec_as_function, SWFDEC_TYPE_AS_OBJECT)

static void
swfdec_as_function_dispose (GObject *object)
{
  SwfdecAsFunction *function = SWFDEC_AS_FUNCTION (object);

  if (function->script) {
    swfdec_script_unref (function->script);
    function->script = NULL;
  }

  G_OBJECT_CLASS (swfdec_as_function_parent_class)->dispose (object);
}

static void
swfdec_as_function_mark (SwfdecAsObject *object)
{
  SwfdecAsFunction *function = SWFDEC_AS_FUNCTION (object);

  if (function->scope)
    swfdec_as_object_mark (SWFDEC_AS_OBJECT (function->scope));

  SWFDEC_AS_OBJECT_CLASS (swfdec_as_function_parent_class)->mark (object);
}

static void
swfdec_as_function_class_init (SwfdecAsFunctionClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  SwfdecAsObjectClass *asobject_class = SWFDEC_AS_OBJECT_CLASS (klass);

  object_class->dispose = swfdec_as_function_dispose;

  asobject_class->mark = swfdec_as_function_mark;
}

static void
swfdec_as_function_init (SwfdecAsFunction *function)
{
}

SwfdecAsFunction *
swfdec_as_function_new (SwfdecAsFrame *scope, SwfdecScript *script)
{
  SwfdecAsFunction *fun;
  SwfdecAsContext *context;

  g_return_val_if_fail (SWFDEC_IS_AS_FRAME (scope), NULL);
  g_return_val_if_fail (script != NULL, NULL);

  context = SWFDEC_AS_OBJECT (scope)->context;
  if (!swfdec_as_context_use_mem (context, sizeof (SwfdecAsFunction)))
    return NULL;
  fun = g_object_new (SWFDEC_TYPE_AS_FUNCTION, NULL);
  swfdec_as_object_add (SWFDEC_AS_OBJECT (fun), context, sizeof (SwfdecAsFunction));
  fun->scope = scope;
  fun->script = script;

  return fun;
}

SwfdecAsFunction *
swfdec_as_function_new_native (SwfdecAsContext *context, SwfdecAsNative native,
    guint min_args)
{
  SwfdecAsFunction *fun;

  g_return_val_if_fail (SWFDEC_IS_AS_CONTEXT (context), NULL);
  g_return_val_if_fail (native != NULL, NULL);

  if (!swfdec_as_context_use_mem (context, sizeof (SwfdecAsFunction)))
    return NULL;
  fun = g_object_new (SWFDEC_TYPE_AS_FUNCTION, NULL);
  swfdec_as_object_add (SWFDEC_AS_OBJECT (fun), context, sizeof (SwfdecAsFunction));
  fun->native = native;
  fun->min_args = min_args;

  return fun;
}

void
swfdec_as_function_call (SwfdecAsFunction *function, SwfdecAsObject *thisp, guint n_args,
    SwfdecAsValue *args, SwfdecAsValue *return_value)
{
  SwfdecAsContext *context;
  SwfdecAsFrame *frame;

  g_return_if_fail (SWFDEC_IS_AS_FUNCTION (function));
  g_return_if_fail (SWFDEC_IS_AS_OBJECT (thisp));
  g_return_if_fail (n_args == 0 || args != NULL);
  g_return_if_fail (return_value != NULL);

  context = thisp->context;
  if (context->frame) {
    guint available_args = swfdec_as_stack_get_size (context->frame->stack);
    n_args = MIN (available_args, n_args);
  } else {
    n_args = 0;
  }
  SWFDEC_AS_VALUE_SET_UNDEFINED (return_value);
  frame->argc = n_args;
  frame->argv = args;
  frame->return_value = return_value;
  frame->function = function;
  /* now do different things depending on if we're a native function or not */
  if (function->native) {
    frame = swfdec_as_frame_new_native (thisp);
    g_assert (function->name);
    frame->function_name = function->name;
  } else {
    frame = swfdec_as_frame_new (thisp, function->script);
    /* FIXME: do the preloading here */
  }
}
