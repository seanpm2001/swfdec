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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <strings.h>

#include "swfdec_as_object.h"
#include "swfdec_as_context.h"
#include "swfdec_as_frame.h"
#include "swfdec_as_native_function.h"
#include "swfdec_as_stack.h"
#include "swfdec_as_strings.h"
#include "swfdec_as_super.h"
#include "swfdec_debug.h"


typedef struct _SwfdecAsVariable SwfdecAsVariable;
struct _SwfdecAsVariable {
  guint			flags;		/* SwfdecAsVariableFlag values */
  SwfdecAsValue     	value;		/* value of property */
};

G_DEFINE_TYPE (SwfdecAsObject, swfdec_as_object, G_TYPE_OBJECT)

static void
swfdec_as_object_dispose (GObject *gobject)
{
  SwfdecAsObject *object = SWFDEC_AS_OBJECT (gobject);

  g_assert (object->properties == NULL);

  G_OBJECT_CLASS (swfdec_as_object_parent_class)->dispose (gobject);
}

static void
swfdec_as_object_mark_property (gpointer key, gpointer value, gpointer unused)
{
  SwfdecAsVariable *var = value;

  swfdec_as_string_mark (key);
  swfdec_as_value_mark (&var->value);
}

static void
swfdec_as_object_do_mark (SwfdecAsObject *object)
{
  if (object->prototype)
    swfdec_as_object_mark (object->prototype);
  g_hash_table_foreach (object->properties, swfdec_as_object_mark_property, NULL);
}

static void
swfdec_as_object_do_add (SwfdecAsObject *object)
{
}

static gboolean
swfdec_as_object_lookup_case_insensitive (gpointer key, gpointer value, gpointer user_data)
{
  return strcasecmp (key, user_data) == 0;
}

static inline SwfdecAsVariable *
swfdec_as_object_hash_lookup (SwfdecAsObject *object, const char *variable)
{
  SwfdecAsVariable *var = g_hash_table_lookup (object->properties, variable);

  if (var || object->context->version >= 7)
    return var;
  var = g_hash_table_find (object->properties, swfdec_as_object_lookup_case_insensitive, (gpointer) variable);
  return var;
}

static gboolean
swfdec_as_object_do_get (SwfdecAsObject *object, const char *variable, 
    SwfdecAsValue *val, guint *flags)
{
  SwfdecAsVariable *var = swfdec_as_object_hash_lookup (object, variable);

  if (var) {
    *val = var->value;
    *flags = var->flags;
    return TRUE;
  }
  return FALSE;
}

static void
swfdec_as_object_do_set (SwfdecAsObject *object, const char *variable, 
    const SwfdecAsValue *val)
{
  SwfdecAsVariable *var;

  if (variable == SWFDEC_AS_STR___proto__) {
    if (SWFDEC_AS_VALUE_IS_OBJECT (val)) {
      object->prototype = SWFDEC_AS_VALUE_GET_OBJECT (val);
    } else {
      object->prototype = NULL;
    }
  }

  var = swfdec_as_object_hash_lookup (object, variable);
  if (var == NULL) {
    if (!swfdec_as_context_use_mem (object->context, sizeof (SwfdecAsVariable)))
      return;
    var = g_slice_new0 (SwfdecAsVariable);
    g_hash_table_insert (object->properties, (gpointer) variable, var);
  }
  var->value = *val;
}

static void
swfdec_as_object_do_set_flags (SwfdecAsObject *object, const char *variable, guint flags, guint mask)
{
  SwfdecAsVariable *var = swfdec_as_object_hash_lookup (object, variable);

  if (var)
    var->flags = (var->flags & ~mask) | flags;
}

static void
swfdec_as_object_free_property (gpointer key, gpointer value, gpointer data)
{
  SwfdecAsObject *object = data;

  swfdec_as_context_unuse_mem (object->context, sizeof (SwfdecAsVariable));
  g_slice_free (SwfdecAsVariable, value);
}

static gboolean
swfdec_as_object_do_delete (SwfdecAsObject *object, const char *variable)
{
  SwfdecAsVariable *var;

  var = g_hash_table_lookup (object->properties, variable);
  if (var == NULL)
    return FALSE;
  if (var->flags & SWFDEC_AS_VARIABLE_PERMANENT)
    return TRUE;

  swfdec_as_object_free_property (NULL, var, object);
  if (!g_hash_table_remove (object->properties, variable)) {
    g_assert_not_reached ();
  }
  return TRUE;
}

typedef struct {
  SwfdecAsObject *		object;
  SwfdecAsVariableForeach	func;
  gpointer			data;
  gboolean			retval;
} ForeachData;

static void
swfdec_as_object_hash_foreach (gpointer key, gpointer value, gpointer data)
{
  ForeachData *fdata = data;
  SwfdecAsVariable *var = value;

  if (!fdata->retval)
    return;

  fdata->retval = fdata->func (fdata->object, key, &var->value, var->flags, fdata->data);
}

/* FIXME: does not do Adobe Flash's order for Enumerate actions */
static gboolean
swfdec_as_object_do_foreach (SwfdecAsObject *object, SwfdecAsVariableForeach func, gpointer data)
{
  ForeachData fdata = { object, func, data, TRUE };

  g_hash_table_foreach (object->properties, swfdec_as_object_hash_foreach, &fdata);
  return fdata.retval;
}

typedef struct {
  SwfdecAsObject *		object;
  SwfdecAsVariableForeachRemove	func;
  gpointer			data;
} ForeachRemoveData;

static gboolean
swfdec_as_object_hash_foreach_remove (gpointer key, gpointer value, gpointer data)
{
  ForeachRemoveData *fdata = data;
  SwfdecAsVariable *var = value;

  if (!fdata->func (fdata->object, key, &var->value, var->flags, fdata->data))
    return FALSE;

  swfdec_as_object_free_property (NULL, var, fdata->object);
  return TRUE;
}

/**
 * swfdec_as_object_foreach_remove:
 * @object: a #SwfdecAsObject
 * @func: function that determines which object to remove
 * @data: data to pass to @func
 *
 * Removes all variables form @object where @func returns %TRUE. This is an 
 * internal function for array operations.
 *
 * Returns: he number of variables removed
 **/
guint
swfdec_as_object_foreach_remove (SwfdecAsObject *object, SwfdecAsVariableForeach func,
    gpointer data)
{
  ForeachRemoveData fdata = { object, func, data };

  g_return_val_if_fail (SWFDEC_IS_AS_OBJECT (object), 0);
  g_return_val_if_fail (func != NULL, 0);

  return g_hash_table_foreach_remove (object->properties,
      swfdec_as_object_hash_foreach_remove, &fdata);
}

typedef struct {
  SwfdecAsObject *		object;
  GHashTable *			properties_new;
  SwfdecAsVariableForeachRename	func;
  gpointer			data;
} ForeachRenameData;

static gboolean
swfdec_as_object_hash_foreach_rename (gpointer key, gpointer value, gpointer data)
{
  ForeachRenameData *fdata = data;
  SwfdecAsVariable *var = value;
  const char *key_new;

  key_new = fdata->func (fdata->object, key, &var->value, var->flags, fdata->data);
  if (key_new) {
    g_hash_table_insert (fdata->properties_new, (gpointer) key_new, var);
  } else {
    swfdec_as_object_free_property (NULL, var, fdata->object);
  }

  return TRUE;
}

/**
 * swfdec_as_object_foreach_rename:
 * @object: a #SwfdecAsObject
 * @func: function determining the new name
 * @data: data to pass to @func
 *
 * Calls @func for each variable of @object. If The function is then supposed 
 * to return the new name of the variable or %NULL if the variable should be 
 * removed. This is an internal function for array operations.
 **/
void
swfdec_as_object_foreach_rename (SwfdecAsObject *object, SwfdecAsVariableForeachRename func,
    gpointer data)
{
  GHashTable *properties_new;
  ForeachRenameData fdata = { object, properties_new, func, data };

  g_return_if_fail (SWFDEC_IS_AS_OBJECT (object));
  g_return_if_fail (func != NULL);

  properties_new = g_hash_table_new (g_direct_hash, g_direct_equal);
  g_hash_table_foreach_remove (object->properties, swfdec_as_object_hash_foreach_rename, &fdata);
  g_hash_table_destroy (object->properties);
  object->properties = properties_new;
}

static char *
swfdec_as_object_do_debug (SwfdecAsObject *object)
{
  if (G_OBJECT_TYPE (object) != SWFDEC_TYPE_AS_OBJECT)
    return g_strdup (G_OBJECT_TYPE_NAME (object));

  return g_strdup ("Object");
}

static void
swfdec_as_object_class_init (SwfdecAsObjectClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = swfdec_as_object_dispose;

  klass->mark = swfdec_as_object_do_mark;
  klass->add = swfdec_as_object_do_add;
  klass->get = swfdec_as_object_do_get;
  klass->set = swfdec_as_object_do_set;
  klass->set_flags = swfdec_as_object_do_set_flags;
  klass->delete = swfdec_as_object_do_delete;
  klass->foreach = swfdec_as_object_do_foreach;
  klass->debug = swfdec_as_object_do_debug;
}

static void
swfdec_as_object_init (SwfdecAsObject *object)
{
}

/**
 * swfdec_as_object_new_empty:
 * @context: a #SwfdecAsContext
 *
 * Creates an empty object. The prototype and constructor properties of the
 * returned object will not be set. You probably want to call 
 * swfdec_as_object_set_constructor() on the returned object yourself.
 * You may want to use swfdec_as_object_new() instead.
 *
 * Returns: A new #SwfdecAsObject adde to @context or %NULL on OOM.
 **/
SwfdecAsObject *
swfdec_as_object_new_empty (SwfdecAsContext *context)
{
  SwfdecAsObject *object;

  g_return_val_if_fail (SWFDEC_IS_AS_CONTEXT (context), NULL);
  
  if (!swfdec_as_context_use_mem (context, sizeof (SwfdecAsObject)))
    return NULL;
  object = g_object_new (SWFDEC_TYPE_AS_OBJECT, NULL);
  swfdec_as_object_add (object, context, sizeof (SwfdecAsObject));
  return object;
}

/**
 * swfdec_as_object_new:
 * @context: a #SwfdecAsContext
 *
 * Allocates a new Object. This does the same as the Actionscript code 
 * "new Object()".
 *
 * Returns: the new object or NULL on out of memory.
 **/
SwfdecAsObject *
swfdec_as_object_new (SwfdecAsContext *context)
{
  SwfdecAsObject *object;
  SwfdecAsValue val;

  g_return_val_if_fail (SWFDEC_IS_AS_CONTEXT (context), NULL);
  g_assert (context->Object);
  g_assert (context->Object_prototype);
  
  object = swfdec_as_object_new_empty (context);
  SWFDEC_AS_VALUE_SET_OBJECT (&val, context->Object_prototype);
  swfdec_as_object_set_variable (object, SWFDEC_AS_STR___proto__, &val);
  SWFDEC_AS_VALUE_SET_OBJECT (&val, context->Object);
  swfdec_as_object_set_variable (object, SWFDEC_AS_STR_constructor, &val);
  return object;
}

/**
 * swfdec_as_object_add:
 * @object: #SwfdecAsObject to make garbage-collected
 * @context: #SwfdecAsContext that should manage the object
 * @size: size the object currently uses
 *
 * Takes over the reference to @object for the garbage collector of @context. 
 * The object may not already be part of a different context. The given @size 
 * must have been allocated before with swfdec_as_context_use_mem ().
 * Note that after swfdec_as_object_add() the garbage collector might hold the
 * only reference to @object.
 **/
void
swfdec_as_object_add (SwfdecAsObject *object, SwfdecAsContext *context, gsize size)
{
  SwfdecAsObjectClass *klass;

  g_return_if_fail (SWFDEC_IS_AS_OBJECT (object));
  g_return_if_fail (SWFDEC_IS_AS_CONTEXT (context));
  g_return_if_fail (object->properties == NULL);

  object->context = context;
  object->size = size;
  g_hash_table_insert (context->objects, object, object);
  object->properties = g_hash_table_new (g_direct_hash, g_direct_equal);
  klass = SWFDEC_AS_OBJECT_GET_CLASS (object);
  g_return_if_fail (klass->add);
  klass->add (object);
}

void
swfdec_as_object_collect (SwfdecAsObject *object)
{
  g_return_if_fail (SWFDEC_IS_AS_OBJECT (object));
  g_return_if_fail (object->properties != NULL);

  g_hash_table_foreach (object->properties, swfdec_as_object_free_property, object);
  g_hash_table_destroy (object->properties);
  object->properties = NULL;
  if (object->size)
    swfdec_as_context_unuse_mem (object->context, object->size);
  g_object_unref (object);
}

void
swfdec_as_object_set_variable (SwfdecAsObject *object,
    const char *variable, const SwfdecAsValue *value)
{
  SwfdecAsObjectClass *klass;

  g_return_if_fail (SWFDEC_IS_AS_OBJECT (object));
  g_return_if_fail (variable != NULL);
  g_return_if_fail (SWFDEC_IS_AS_VALUE (value));

  klass = SWFDEC_AS_OBJECT_GET_CLASS (object);
  klass->set (object, variable, value);
}

gboolean
swfdec_as_object_get_variable_and_flags (SwfdecAsObject *object, 
    const char *variable, SwfdecAsValue *value, guint *flags, SwfdecAsObject **pobject)
{
  SwfdecAsObjectClass *klass;
  guint i;
  SwfdecAsValue tmp_val;
  guint tmp_flags;
  SwfdecAsObject *tmp_pobject;

  g_return_val_if_fail (SWFDEC_IS_AS_OBJECT (object), FALSE);
  g_return_val_if_fail (variable != NULL, FALSE);

  if (value == NULL)
    value = &tmp_val;
  if (flags == NULL)
    flags = &tmp_flags;
  if (pobject == NULL)
    pobject = &tmp_pobject;

  for (i = 0; i < 256 && object != NULL; i++) {
    klass = SWFDEC_AS_OBJECT_GET_CLASS (object);
    if (klass->get (object, variable, value, flags)) {
      *pobject = object;
      return TRUE;
    }
    object = object->prototype;
  }
  if (i == 256) {
    swfdec_as_context_abort (object->context, "Prototype recursion limit exceeded");
    *flags = 0;
    *pobject = NULL;
    return FALSE;
  }
  //SWFDEC_WARNING ("no such variable %s", variable);
  SWFDEC_AS_VALUE_SET_UNDEFINED (value);
  *flags = 0;
  *pobject = NULL;
  return FALSE;
}

gboolean
swfdec_as_object_delete_variable (SwfdecAsObject *object, const char *variable)
{
  SwfdecAsObjectClass *klass;

  g_return_val_if_fail (SWFDEC_IS_AS_OBJECT (object), FALSE);
  g_return_val_if_fail (variable != NULL, FALSE);

  klass = SWFDEC_AS_OBJECT_GET_CLASS (object);
  return klass->delete (object, variable);
}

/**
 * swfdec_as_object_set_variable_flags:
 * @object: a #SwfdecAsObject
 * @variable: the variable to modify
 * @flags: flags to set
 *
 * Sets the given flags for the given variable.
 **/
void
swfdec_as_object_set_variable_flags (SwfdecAsObject *object, 
    const char *variable, SwfdecAsVariableFlag flags)
{
  SwfdecAsObjectClass *klass;

  g_return_if_fail (SWFDEC_IS_AS_OBJECT (object));
  g_return_if_fail (variable != NULL);

  klass = SWFDEC_AS_OBJECT_GET_CLASS (object);
  klass->set_flags (object, variable, flags, flags);
}

/**
 * swfdec_as_object_unset_variable_flags:
 * @object: a #SwfdecAsObject
 * @variable: the variable to modify
 * @flags: flags to unset
 *
 * Unsets the given flags for the given variable. The variable must exist in 
 * @object.
 **/
void
swfdec_as_object_unset_variable_flags (SwfdecAsObject *object,
    const char *variable, SwfdecAsVariableFlag flags)
{
  SwfdecAsObjectClass *klass;

  g_return_if_fail (SWFDEC_IS_AS_OBJECT (object));
  g_return_if_fail (variable != NULL);

  klass = SWFDEC_AS_OBJECT_GET_CLASS (object);
  klass->set_flags (object, variable, 0, flags);
}

gboolean
swfdec_as_object_foreach (SwfdecAsObject *object, SwfdecAsVariableForeach func,
    gpointer data)
{
  SwfdecAsObjectClass *klass;

  g_return_val_if_fail (SWFDEC_IS_AS_OBJECT (object), FALSE);
  g_return_val_if_fail (func != NULL, FALSE);

  klass = SWFDEC_AS_OBJECT_GET_CLASS (object);
  g_return_val_if_fail (klass->foreach != NULL, FALSE);
  return klass->foreach (object, func, data);
}

/*** SIMPLIFICATIONS ***/

static void
swfdec_as_object_do_nothing (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *retval)
{
}

/**
 * swfdec_as_object_add_function:
 * @object: a #SwfdecAsObject
 * @name: name of the function. The string does not have to be 
 *        garbage-collected.
 * @type: the required type of the this Object to make this function execute.
 *        May be 0 to accept any type.
 * @native: a native function or %NULL to just not do anything
 * @min_args: minimum number of arguments to pass to @native
 *
 * Adds @native as a variable named @name to @object. The newly added variable
 * will not be enumerated.
 *
 * Returns: the newly created #SwfdecAsFunction or %NULL on error.
 **/
SwfdecAsFunction *
swfdec_as_object_add_function (SwfdecAsObject *object, const char *name, GType type,
    SwfdecAsNative native, guint min_args)
{
  SwfdecAsFunction *function;
  SwfdecAsValue val;

  g_return_val_if_fail (SWFDEC_IS_AS_OBJECT (object), NULL);
  g_return_val_if_fail (name != NULL, NULL);
  g_return_val_if_fail (type == 0 || g_type_is_a (type, SWFDEC_TYPE_AS_OBJECT), NULL);

  if (!native)
    native = swfdec_as_object_do_nothing;
  function = swfdec_as_native_function_new (object->context, name, native, min_args);
  if (function == NULL)
    return NULL;
  if (type != 0)
    swfdec_as_native_function_set_object_type (SWFDEC_AS_NATIVE_FUNCTION (function), type);
  name = swfdec_as_context_get_string (object->context, name);
  SWFDEC_AS_VALUE_SET_OBJECT (&val, SWFDEC_AS_OBJECT (function));
  /* FIXME: I'd like to make sure no such property exists yet */
  swfdec_as_object_set_variable (object, name, &val);
  swfdec_as_object_set_variable_flags (object, name, SWFDEC_AS_VARIABLE_DONT_ENUM);
  return function;
}

/**
 * swfdec_as_object_run:
 * @object: a #SwfdecAsObject
 * @script: script to execute
 *
 * Executes the given @script with @object as this pointer.
 **/
void
swfdec_as_object_run (SwfdecAsObject *object, SwfdecScript *script)
{
  SwfdecAsContext *context;
  SwfdecAsFrame *frame;

  g_return_if_fail (SWFDEC_IS_AS_OBJECT (object));
  g_return_if_fail (script != NULL);

  context = object->context;
  frame = swfdec_as_frame_new (context, script);
  if (frame == NULL)
    return;
  swfdec_as_frame_set_this (frame, object);
  swfdec_as_frame_preload (frame);
  swfdec_as_context_run (context);
  swfdec_as_stack_pop (context);
}

/**
 * swfdec_as_object_call:
 * @object: a #SwfdecAsObject
 * @name: garbage-collected string naming the function to call. 
 * @argc: number of arguments to provide to function
 * @argv: arguments or %NULL when @argc is 0
 * @return_value: location to take the return value of the call or %NULL to 
 *                ignore the return value.
 *
 * Calls the function named @name on the given object. This function is 
 * essentially equal to the folloeing Actionscript code: 
 * <informalexample><programlisting>
 * @return_value = @object.@name (@argv[0], ..., @argv[argc-1]);
 * </programlisting></informalexample>
 **/
void
swfdec_as_object_call (SwfdecAsObject *object, const char *name, guint argc, 
    SwfdecAsValue *argv, SwfdecAsValue *return_value)
{
  static SwfdecAsValue tmp; /* ignored */
  SwfdecAsFunction *fun;

  g_return_if_fail (SWFDEC_IS_AS_OBJECT (object));
  g_return_if_fail (name != NULL);
  g_return_if_fail (argc == 0 || argv != NULL);

  if (return_value)
    SWFDEC_AS_VALUE_SET_UNDEFINED (return_value);
  swfdec_as_object_get_variable (object, name, &tmp);
  if (!SWFDEC_AS_VALUE_IS_OBJECT (&tmp))
    return;
  fun = (SwfdecAsFunction *) SWFDEC_AS_VALUE_GET_OBJECT (&tmp);
  if (!SWFDEC_IS_AS_FUNCTION (fun))
    return;
  swfdec_as_function_call (fun, object, argc, argv, return_value ? return_value : &tmp);
  swfdec_as_context_run (object->context);
}

gboolean
swfdec_as_object_has_function (SwfdecAsObject *object, const char *name)
{
  SwfdecAsValue val;

  g_return_val_if_fail (SWFDEC_IS_AS_OBJECT (object), FALSE);
  g_return_val_if_fail (name != NULL, FALSE);

  swfdec_as_object_get_variable (object, name, &val);
  if (!SWFDEC_AS_VALUE_IS_OBJECT (&val))
    return FALSE;
  return SWFDEC_IS_AS_FUNCTION (SWFDEC_AS_VALUE_GET_OBJECT (&val));
}

/**
 * swfdec_as_object_create:
 * @fun: constructor
 * @n_args: number of arguments
 * @args: arguments to pass to constructor
 *
 * Creates a new object for the given constructor and pushes the constructor on
 * top of the stack. To actually run the constructor, you need to call 
 * swfdec_as_context_run(). After the constructor has been run, the new object 
 * will be pushed on top of the stack.
 **/
void
swfdec_as_object_create (SwfdecAsFunction *fun, guint n_args, 
    const SwfdecAsValue *args)
{
  SwfdecAsObject *new;
  SwfdecAsContext *context;
  SwfdecAsFunction *cur;
  guint size;
  GType type = 0;

  g_return_if_fail (SWFDEC_IS_AS_FUNCTION (fun));

  context = SWFDEC_AS_OBJECT (fun)->context;
  cur = fun;
  while (type == 0 && cur != NULL) {
    if (SWFDEC_IS_AS_NATIVE_FUNCTION (cur)) {
      SwfdecAsNativeFunction *native = SWFDEC_AS_NATIVE_FUNCTION (cur);
      if (native->construct_size) {
	type = native->construct_type;
	size = native->construct_size;
	break;
      }
    }
#if 0
    This doesn't work. It's supposed to figure out the last native object in the inheritance chain.
    swfdec_as_object_get_variable (SWFDEC_AS_OBJECT (cur), SWFDEC_AS_STR___constructor__, &val);
    if (SWFDEC_AS_VALUE_IS_OBJECT (&val)) {
      cur = (SwfdecAsFunction *) SWFDEC_AS_VALUE_GET_OBJECT (&val);
      if (!SWFDEC_IS_AS_FUNCTION (cur))
	cur = NULL;
    } else {
      cur = NULL;
    }
#else
    cur = NULL;
#endif
  }
  if (type == 0) {
    type = SWFDEC_TYPE_AS_OBJECT;
    size = sizeof (SwfdecAsObject);
  }
  if (!swfdec_as_context_use_mem (context, size))
    return;
  new = g_object_new (type, NULL);
  swfdec_as_object_add (new, context, size);
  swfdec_as_object_set_constructor (new, SWFDEC_AS_OBJECT (fun), FALSE);
  swfdec_as_function_call (fun, new, n_args, args, NULL);
  context->frame->construct = TRUE;
}

/**
 * swfdec_as_object_set_constructor:
 * @object: a #SwfdecAsObject
 * @construct: the constructor of @object
 * @scripted: %TRUE if this object was created by a script. Flash sets the 
 *            property named "__constructor__" on script-created objects, but
 *            "constructor" on native ones.
 *
 * Sets the constructor variables for @object. Most objects get these 
 * variables set automatically, but for objects you created yourself, you want
 * to call this function. This is essentially the same as the following script
 * code:
 * |[ object.__constructor__ = construct;
 * object.__proto__ = construct.prototype; ]|
 **/
void
swfdec_as_object_set_constructor (SwfdecAsObject *object, SwfdecAsObject *construct, gboolean scripted)
{
  SwfdecAsValue val;
  SwfdecAsObject *proto;

  g_return_if_fail (SWFDEC_IS_AS_OBJECT (object));
  g_return_if_fail (SWFDEC_IS_AS_OBJECT (construct));

  swfdec_as_object_get_variable (SWFDEC_AS_OBJECT (construct), SWFDEC_AS_STR_prototype, &val);
  if (SWFDEC_AS_VALUE_IS_OBJECT (&val)) {
    proto = SWFDEC_AS_VALUE_GET_OBJECT (&val);
  } else {
    SWFDEC_WARNING ("constructor has no prototype, using Object.prototype");
    proto = object->context->Object_prototype;
  }
  SWFDEC_AS_VALUE_SET_OBJECT (&val, proto);
  swfdec_as_object_set_variable (object, SWFDEC_AS_STR___proto__, &val);
  SWFDEC_AS_VALUE_SET_OBJECT (&val, construct);
  swfdec_as_object_set_variable (object, scripted ? SWFDEC_AS_STR_constructor : SWFDEC_AS_STR___constructor__, &val);
}

/*** AS CODE ***/

static void
swfdec_as_object_hasOwnProperty (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *retval)
{
  SwfdecAsObjectClass *klass;
  const char *name;
  guint flags;
  SwfdecAsValue value;

  name = swfdec_as_value_to_string (object->context, &argv[0]);
  
  klass = SWFDEC_AS_OBJECT_GET_CLASS (object);
  if (klass->get (object, name, &value, &flags) &&
      (flags & SWFDEC_AS_VARIABLE_NATIVE) == 0)
    SWFDEC_AS_VALUE_SET_BOOLEAN (retval, TRUE);
  else
    SWFDEC_AS_VALUE_SET_BOOLEAN (retval, FALSE);
}

static void
swfdec_as_object_valueOf (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *retval)
{
  SWFDEC_AS_VALUE_SET_OBJECT (retval, object);
}

static void
swfdec_as_object_toString (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *retval)
{
  if (SWFDEC_IS_AS_FUNCTION (object) && !SWFDEC_IS_AS_SUPER (object)) {
    SWFDEC_AS_VALUE_SET_STRING (retval, SWFDEC_AS_STR__type_Function_);
  } else {
    SWFDEC_AS_VALUE_SET_STRING (retval, SWFDEC_AS_STR__object_Object_);
  }
}

void
swfdec_as_object_init_context (SwfdecAsContext *context, guint version)
{
  SwfdecAsValue val;
  SwfdecAsObject *object, *proto;

  object = SWFDEC_AS_OBJECT (swfdec_as_object_add_function (context->global, 
      SWFDEC_AS_STR_Object, 0, NULL, 0));
  if (!object)
    return;
  proto = swfdec_as_object_new_empty (context);
  if (!proto)
    return;
  context->Object = object;
  context->Object_prototype = proto;
  SWFDEC_AS_VALUE_SET_OBJECT (&val, proto);
  if (context->Function_prototype) {
    /* first, finish the function prototype */
    swfdec_as_object_set_variable (context->Function_prototype, SWFDEC_AS_STR___proto__, &val);
  }
  /* now, set our own */
  swfdec_as_object_set_variable (object, SWFDEC_AS_STR_prototype, &val);

  if (version > 5) {
    swfdec_as_object_add_function (proto, SWFDEC_AS_STR_hasOwnProperty, 
	SWFDEC_TYPE_AS_OBJECT, swfdec_as_object_hasOwnProperty, 1);
  }
  swfdec_as_object_add_function (proto, SWFDEC_AS_STR_valueOf, 
      SWFDEC_TYPE_AS_OBJECT, swfdec_as_object_valueOf, 0);
  swfdec_as_object_add_function (proto, SWFDEC_AS_STR_toString, 
      SWFDEC_TYPE_AS_OBJECT, swfdec_as_object_toString, 0);
}

void
swfdec_as_variable_set (SwfdecAsVariable *var, const SwfdecAsValue *value)
{
  g_return_if_fail (var != NULL);
  g_return_if_fail (SWFDEC_IS_AS_VALUE (value));

  if (var->flags & SWFDEC_AS_VARIABLE_READONLY)
    return;
  var->value = *value;
}

char *
swfdec_as_object_get_debug (SwfdecAsObject *object)
{
  SwfdecAsObjectClass *klass;

  g_return_val_if_fail (SWFDEC_IS_AS_OBJECT (object), NULL);

  klass = SWFDEC_AS_OBJECT_GET_CLASS (object);
  return klass->debug (object);
}

/**
 * swfdec_as_object_resolve:
 * @object: a #SwfdecAsObject
 *
 * Resolves the object to its real object. Some internal objects should not be
 * exposed to scripts, for example #SwfdecAsFrame objects. If an object you want
 * to expose might be internal, call this function to resolve it to an object
 * that is safe to expose.
 *
 * Returns: a non-internal object
 **/
SwfdecAsObject *
swfdec_as_object_resolve (SwfdecAsObject *object)
{
  SwfdecAsObjectClass *klass;

  g_return_val_if_fail (SWFDEC_IS_AS_OBJECT (object), NULL);

  klass = SWFDEC_AS_OBJECT_GET_CLASS (object);
  if (klass->resolve == NULL)
    return object;

  return klass->resolve (object);
}
