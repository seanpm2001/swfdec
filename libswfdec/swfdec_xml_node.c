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

#include <string.h>

#include "swfdec_xml_node.h"
#include "swfdec_xml.h"
#include "swfdec_as_array.h"
#include "swfdec_as_native_function.h"
#include "swfdec_as_object.h"
#include "swfdec_as_strings.h"
#include "swfdec_debug.h"
#include "swfdec_internal.h"
#include "swfdec_player_internal.h"
#include "swfdec_load_object.h"

G_DEFINE_TYPE (SwfdecXmlNode, swfdec_xml_node, SWFDEC_TYPE_AS_OBJECT)

static void
swfdec_xml_node_do_mark (SwfdecAsObject *object)
{
  SwfdecXmlNode *node = SWFDEC_XML_NODE (object);

  if (node->name != NULL)
    swfdec_as_string_mark (node->name);
  if (node->value != NULL)
    swfdec_as_string_mark (node->value);
  if (node->parent != NULL)
    swfdec_as_object_mark (SWFDEC_AS_OBJECT (node->parent));
  if (node->children != NULL)
    swfdec_as_object_mark (SWFDEC_AS_OBJECT (node->children));
  if (node->attributes != NULL)
    swfdec_as_object_mark (SWFDEC_AS_OBJECT (node->attributes));
  if (node->childNodes != NULL)
    swfdec_as_object_mark (SWFDEC_AS_OBJECT (node->childNodes));

  SWFDEC_AS_OBJECT_CLASS (swfdec_xml_node_parent_class)->mark (object);
}

static void
swfdec_xml_node_class_init (SwfdecXmlNodeClass *klass)
{
  SwfdecAsObjectClass *asobject_class = SWFDEC_AS_OBJECT_CLASS (klass);

  asobject_class->mark = swfdec_xml_node_do_mark;
}

static void
swfdec_xml_node_init (SwfdecXmlNode *xml_node)
{
}

/*** AS CODE ***/

gint32
swfdec_xml_node_num_children (SwfdecXmlNode *node)
{
  g_return_val_if_fail (SWFDEC_IS_VALID_XML_NODE (node), 0);

  return swfdec_as_array_length (node->children);
}

static SwfdecXmlNode *
swfdec_xml_node_get_child (SwfdecXmlNode *node, gint32 ind)
{
  SwfdecAsValue val;

  g_return_val_if_fail (SWFDEC_IS_VALID_XML_NODE (node), NULL);
  g_return_val_if_fail (ind >= 0, NULL);

  if (ind >= swfdec_xml_node_num_children (node))
    return NULL;

  swfdec_as_array_get_value (node->children, ind, &val);

  g_return_val_if_fail (SWFDEC_AS_VALUE_IS_OBJECT (&val), NULL);
  g_return_val_if_fail (SWFDEC_IS_VALID_XML_NODE (
	SWFDEC_AS_VALUE_GET_OBJECT (&val)), NULL);

  return SWFDEC_XML_NODE (SWFDEC_AS_VALUE_GET_OBJECT (&val));
}

static gint32
swfdec_xml_node_index_of_child (SwfdecXmlNode *node, SwfdecXmlNode *child)
{
  gint32 num, i;

  g_return_val_if_fail (SWFDEC_IS_VALID_XML_NODE (node), -1);
  g_return_val_if_fail (SWFDEC_IS_VALID_XML_NODE (child), -1);

  num = swfdec_xml_node_num_children (node);
  for (i = 0; i < num; i++) {
    if (swfdec_xml_node_get_child (node, i) == child)
      return i;
  }

  return -1;
}

static void
swfdec_xml_node_update_childNodes (SwfdecXmlNode *node)
{
  SwfdecAsValue val;
  SwfdecAsValue *vals;
  gint32 num, i;

  g_return_if_fail (SWFDEC_IS_VALID_XML_NODE (node));

  // remove old
  SWFDEC_AS_VALUE_SET_INT (&val, 0);
  swfdec_as_object_set_variable (SWFDEC_AS_OBJECT (node->childNodes),
      SWFDEC_AS_STR_length, &val);

  // add everything
  num = swfdec_xml_node_num_children (node);
  vals = g_malloc (sizeof (SwfdecAsValue) * num);
  for (i = 0; i < num; i++) {
    SWFDEC_AS_VALUE_SET_OBJECT (&vals[i],
	SWFDEC_AS_OBJECT (swfdec_xml_node_get_child (node, i)));
  }
  swfdec_as_array_append (node->childNodes, num, vals);
  g_free (vals);
}

static const char *
swfdec_xml_node_getNamespaceForPrefix (SwfdecXmlNode *node, const char *prefix)
{
  const char *var;
  SwfdecAsValue val;

  g_return_val_if_fail (SWFDEC_IS_VALID_XML_NODE (node), NULL);

  if (prefix == NULL || strlen (prefix) == 0) {
    var = swfdec_as_context_get_string (SWFDEC_AS_OBJECT (node)->context,
	"xmlns");
  } else {
    var = swfdec_as_context_give_string (SWFDEC_AS_OBJECT (node)->context,
	g_strconcat ("xmlns:", prefix, NULL));
  }

  do {
    swfdec_as_object_get_variable (node->attributes, var, &val);
    if (!SWFDEC_AS_VALUE_IS_UNDEFINED (&val)) {
      return swfdec_as_value_to_string (SWFDEC_AS_OBJECT (node)->context, &val);
    }
    node = node->parent;
  } while (node != NULL);

  return NULL;
}

typedef struct {
  const char	*namespace;
  const char	*variable;
} ForeachFindNamespaceData;

static gboolean
swfdec_xml_node_foreach_find_namespace (SwfdecAsObject *object,
    const char *variable, SwfdecAsValue *value, guint flags, gpointer data)
{
  const char *uri;
  ForeachFindNamespaceData *fdata = data;

  // check whether it's namespace variable (xmlns or xmlns:*)
  if (strlen (variable) < strlen("xmlns"))
    return TRUE;

  if (g_ascii_strncasecmp (variable, "xmlns", strlen("xmlns")))
    return TRUE;

  if (variable[strlen("xmlns")] != '\0' && variable[strlen("xmlns")] != ':')
    return TRUE;

  // ok, now check if the uri is the one we are searching for
  uri = swfdec_as_value_to_string (object->context, value);
  if (!g_ascii_strcasecmp (uri, fdata->namespace)) {
    fdata->variable = variable;
    return FALSE;
  } else {
    return TRUE;
  }
}

static const char *
swfdec_xml_node_getPrefixForNamespace (SwfdecXmlNode *node,
    const char *namespace)
{
  ForeachFindNamespaceData fdata;

  g_return_val_if_fail (SWFDEC_IS_VALID_XML_NODE (node), NULL);
  g_return_val_if_fail (namespace != NULL, NULL);

  fdata.namespace = namespace;
  fdata.variable = NULL;

  do {
    swfdec_as_object_foreach (node->attributes,
	swfdec_xml_node_foreach_find_namespace, &fdata);
    node = node->parent;
  } while (node != NULL && fdata.variable == NULL);

  if (fdata.variable != NULL) {
    const char *p;

    p = strchr (fdata.variable, ':');
    if (p == NULL || *(p + 1) == '\0')
      return SWFDEC_AS_STR_EMPTY;

    return swfdec_as_context_get_string (SWFDEC_AS_OBJECT (node)->context,
	p + 1);
  } else {
    return NULL;
  }
}

static void
swfdec_xml_node_get_nodeType (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  if (!SWFDEC_IS_VALID_XML_NODE (object))
    return;

  SWFDEC_AS_VALUE_SET_INT (ret, SWFDEC_XML_NODE (object)->type);
}

static void
swfdec_xml_node_get_nodeValue (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  if (!SWFDEC_IS_VALID_XML_NODE (object))
    return;

  if (SWFDEC_XML_NODE (object)->value != NULL) {
    SWFDEC_AS_VALUE_SET_STRING (ret, SWFDEC_XML_NODE (object)->value);
  } else {
    SWFDEC_AS_VALUE_SET_NULL (ret);
  }
}

static void
swfdec_xml_node_set_nodeValue (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  const char *value;

  if (!SWFDEC_IS_VALID_XML_NODE (object))
    return;

  if (argc < 1)
    return;

  value = swfdec_as_value_to_string (cx, &argv[0]);

  SWFDEC_XML_NODE (object)->value = value;
  SWFDEC_AS_VALUE_SET_STRING (ret, value);
}

static void
swfdec_xml_node_get_nodeName (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  if (!SWFDEC_IS_VALID_XML_NODE (object))
    return;

  if (SWFDEC_XML_NODE (object)->name != NULL) {
    SWFDEC_AS_VALUE_SET_STRING (ret, SWFDEC_XML_NODE (object)->name);
  } else {
    SWFDEC_AS_VALUE_SET_NULL (ret);
  }
}

static void
swfdec_xml_node_set_nodeName (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  const char *name;

  if (!SWFDEC_IS_VALID_XML_NODE (object))
    return;

  if (argc < 1)
    return;

  // special case
  if (SWFDEC_AS_VALUE_IS_UNDEFINED (&argv[0]))
    return;

  name = swfdec_as_value_to_string (cx, &argv[0]);

  SWFDEC_XML_NODE (object)->name = name;
  SWFDEC_AS_VALUE_SET_STRING (ret, name);
}

static const char *
swfdec_xml_node_get_prefix (SwfdecXmlNode *node)
{
  const char *p;

  g_return_val_if_fail (SWFDEC_IS_VALID_XML_NODE (node), NULL);

  if (node->name == NULL)
    return NULL;

  p = strchr (node->name, ':');
  if (p == NULL || *(p + 1) == '\0')
    return NULL;

  return swfdec_as_context_give_string (SWFDEC_AS_OBJECT (node)->context,
      g_strndup (node->name, p - node->name));
}

static void
swfdec_xml_node_do_get_prefix (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  const char *prefix;

  if (!SWFDEC_IS_VALID_XML_NODE (object))
    return;

  if (SWFDEC_XML_NODE (object)->name == NULL) {
    SWFDEC_AS_VALUE_SET_NULL (ret);
    return;
  }

  prefix = swfdec_xml_node_get_prefix (SWFDEC_XML_NODE (object));
  if (prefix != NULL) {
    SWFDEC_AS_VALUE_SET_STRING (ret, prefix);
  } else {
    SWFDEC_AS_VALUE_SET_STRING (ret, SWFDEC_AS_STR_EMPTY);
  }
}

static void
swfdec_xml_node_get_localName (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  const char *p, *name;

  if (!SWFDEC_IS_VALID_XML_NODE (object))
    return;

  if (SWFDEC_XML_NODE (object)->name == NULL) {
    SWFDEC_AS_VALUE_SET_NULL (ret);
    return;
  }

  name = SWFDEC_XML_NODE (object)->name;
  p = strchr (name, ':');
  if (p == NULL || *(p + 1) == '\0') {
    SWFDEC_AS_VALUE_SET_STRING (ret, name);
    return;
  }
  p++;

  SWFDEC_AS_VALUE_SET_STRING (ret,
      swfdec_as_context_give_string (cx, g_strdup (p)));
}

static void
swfdec_xml_node_get_namespaceURI (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  const char *uri;

  if (!SWFDEC_IS_VALID_XML_NODE (object))
    return;

  if (SWFDEC_XML_NODE (object)->name == NULL) {
    SWFDEC_AS_VALUE_SET_NULL (ret);
    return;
  }

  uri = swfdec_xml_node_getNamespaceForPrefix (SWFDEC_XML_NODE (object),
      swfdec_xml_node_get_prefix (SWFDEC_XML_NODE (object)));
  if (uri != NULL) {
    SWFDEC_AS_VALUE_SET_STRING (ret, uri);
  } else {
    SWFDEC_AS_VALUE_SET_STRING (ret, SWFDEC_AS_STR_EMPTY);
  }
}

static void
swfdec_xml_node_get_attributes (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  if (!SWFDEC_IS_VALID_XML_NODE (object))
    return;

  SWFDEC_AS_VALUE_SET_OBJECT (ret, SWFDEC_XML_NODE (object)->attributes);
}

static void
swfdec_xml_node_get_parentNode (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  if (!SWFDEC_IS_VALID_XML_NODE (object))
    return;

  if (SWFDEC_XML_NODE (object)->parent != NULL) {
    SWFDEC_AS_VALUE_SET_OBJECT (ret,
	SWFDEC_AS_OBJECT (SWFDEC_XML_NODE (object)->parent));
  } else {
    SWFDEC_AS_VALUE_SET_NULL (ret);
  }
}

static SwfdecXmlNode *
swfdec_xml_node_previousSibling (SwfdecXmlNode *node)
{
  gint32 i;

  if (node->parent == NULL)
    return NULL;

  i = swfdec_xml_node_index_of_child (node->parent, node);
  g_assert (i >= 0);

  if (i <= 0)
    return NULL;

  return swfdec_xml_node_get_child (node->parent, i - 1);
}

static void
swfdec_xml_node_get_previousSibling (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecXmlNode *sibling;

  if (!SWFDEC_IS_VALID_XML_NODE (object))
    return;

  sibling = swfdec_xml_node_previousSibling (SWFDEC_XML_NODE (object));
  if (sibling != NULL) {
    SWFDEC_AS_VALUE_SET_OBJECT (ret, SWFDEC_AS_OBJECT (sibling));
  } else {
    SWFDEC_AS_VALUE_SET_NULL (ret);
  }
}

static SwfdecXmlNode *
swfdec_xml_node_nextSibling (SwfdecXmlNode *node)
{
  gint32 i;

  if (node->parent == NULL)
    return NULL;

  i = swfdec_xml_node_index_of_child (node->parent, node);
  g_assert (i >= 0);

  return swfdec_xml_node_get_child (node->parent, i + 1);
}

static void
swfdec_xml_node_get_nextSibling (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecXmlNode *sibling;

  if (!SWFDEC_IS_VALID_XML_NODE (object))
    return;

  sibling = swfdec_xml_node_nextSibling (SWFDEC_XML_NODE (object));
  if (sibling != NULL) {
    SWFDEC_AS_VALUE_SET_OBJECT (ret, SWFDEC_AS_OBJECT (sibling));
  } else {
    SWFDEC_AS_VALUE_SET_NULL (ret);
  }
}

static void
swfdec_xml_node_get_firstChild (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecXmlNode *child;

  if (!SWFDEC_IS_VALID_XML_NODE (object))
    return;

  child = swfdec_xml_node_get_child (SWFDEC_XML_NODE (object), 0);
  if (child != NULL) {
    SWFDEC_AS_VALUE_SET_OBJECT (ret, SWFDEC_AS_OBJECT (child));
  } else {
    SWFDEC_AS_VALUE_SET_NULL (ret);
  }
}

static void
swfdec_xml_node_get_lastChild (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  gint32 num;
  SwfdecXmlNode *child;

  if (!SWFDEC_IS_VALID_XML_NODE (object))
    return;

  num = swfdec_xml_node_num_children (SWFDEC_XML_NODE (object));
  if (num == 0) {
    SWFDEC_AS_VALUE_SET_NULL (ret);
    return;
  }

  child = swfdec_xml_node_get_child (SWFDEC_XML_NODE (object), num - 1);
  g_assert (child != NULL);

  SWFDEC_AS_VALUE_SET_OBJECT (ret, SWFDEC_AS_OBJECT (child));
}

static void
swfdec_xml_node_get_childNodes (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  if (!SWFDEC_IS_VALID_XML_NODE (object))
    return;

  SWFDEC_AS_VALUE_SET_OBJECT (ret,
      SWFDEC_AS_OBJECT (SWFDEC_XML_NODE (object)->childNodes));
}

SWFDEC_AS_NATIVE (253, 7, swfdec_xml_node_do_getNamespaceForPrefix)
void
swfdec_xml_node_do_getNamespaceForPrefix (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  const char *namespace;

  if (!SWFDEC_IS_VALID_XML_NODE (object))
    return;

  if (argc < 1) {
    SWFDEC_AS_VALUE_SET_NULL (ret);
    return;
  }

  namespace = swfdec_xml_node_getNamespaceForPrefix (SWFDEC_XML_NODE (object),
      swfdec_as_value_to_string (cx, &argv[0]));

  if (namespace != NULL) {
    SWFDEC_AS_VALUE_SET_STRING (ret, namespace);
  } else {
    SWFDEC_AS_VALUE_SET_NULL (ret);
  }
}

SWFDEC_AS_NATIVE (253, 8, swfdec_xml_node_do_getPrefixForNamespace)
void
swfdec_xml_node_do_getPrefixForNamespace (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  const char *prefix;

  if (!SWFDEC_IS_VALID_XML_NODE (object))
    return;

  if (argc < 1) {
    SWFDEC_AS_VALUE_SET_NULL (ret);
    return;
  }

  prefix = swfdec_xml_node_getPrefixForNamespace (SWFDEC_XML_NODE (object),
      swfdec_as_value_to_string (cx, &argv[0]));

  if (prefix != NULL) {
    SWFDEC_AS_VALUE_SET_STRING (ret, prefix);
  } else {
    SWFDEC_AS_VALUE_SET_NULL (ret);
  }
}

static gboolean
swfdec_xml_node_foreach_copy_attributes (SwfdecAsObject *object,
    const char *variable, SwfdecAsValue *value, guint flags, gpointer data)
{
  SwfdecAsObject *target = data;
  swfdec_as_object_set_variable (target, variable, value);
  return TRUE;
}

static void
swfdec_xml_node_copy_attributes (SwfdecXmlNode *node, SwfdecXmlNode *target)
{
  swfdec_as_object_foreach (node->attributes,
      swfdec_xml_node_foreach_copy_attributes, target->attributes);
}

static SwfdecXmlNode *
swfdec_xml_node_clone (SwfdecAsContext *cx, SwfdecXmlNode *node, gboolean deep)
{
  SwfdecXmlNode *new;

  g_assert (SWFDEC_IS_AS_CONTEXT (cx));
  g_assert (SWFDEC_IS_VALID_XML_NODE (node));

  new = swfdec_xml_node_new (cx, SWFDEC_XML_NODE_ELEMENT, SWFDEC_AS_STR_EMPTY);

  new->valid = TRUE;
  new->type = node->type;
  new->name = node->name;
  new->value = node->value;

  swfdec_xml_node_copy_attributes (node, new);

  if (deep) {
    SwfdecAsValue val;
    SwfdecXmlNode *child, *child_new;
    gint32 num, i;

    num = swfdec_xml_node_num_children (node);

    for (i = 0; i < num; i++) {
      child = swfdec_xml_node_get_child (node, i);
      child_new = swfdec_xml_node_clone (cx, child, TRUE);
      child_new->parent = new;
      SWFDEC_AS_VALUE_SET_OBJECT (&val, SWFDEC_AS_OBJECT (child_new));
      swfdec_as_array_push (new->children, &val);
    }

    swfdec_xml_node_update_childNodes (new);
  }

  return new;
}

SWFDEC_AS_NATIVE (253, 1, swfdec_xml_node_cloneNode)
void
swfdec_xml_node_cloneNode (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  gboolean deep;
  SwfdecXmlNode *new;

  if (!SWFDEC_IS_VALID_XML_NODE (object))
    return;

  if (argc >= 1) {
    deep = swfdec_as_value_to_boolean (cx, &argv[0]);
  } else {
    deep = FALSE;
  }

  new = swfdec_xml_node_clone (cx, SWFDEC_XML_NODE (object), deep);
  SWFDEC_AS_VALUE_SET_OBJECT (ret, SWFDEC_AS_OBJECT (new));
}

void
swfdec_xml_node_removeNode (SwfdecXmlNode *node)
{
  gint32 i;

  g_return_if_fail (SWFDEC_IS_VALID_XML_NODE (node));

  if (node->parent == NULL)
    return;

  i = swfdec_xml_node_index_of_child (node->parent, node);
  g_assert (i >= 0);

  swfdec_as_array_remove (node->parent->children, i);
  swfdec_xml_node_update_childNodes (node->parent);
  node->parent = NULL;
}

SWFDEC_AS_NATIVE (253, 2, swfdec_xml_node_do_removeNode)
void
swfdec_xml_node_do_removeNode (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  if (!SWFDEC_IS_VALID_XML_NODE (object))
    return;

  swfdec_xml_node_removeNode (SWFDEC_XML_NODE (object));
}

void
swfdec_xml_node_removeChildren (SwfdecXmlNode *node)
{
  gint32 num, i;

  g_return_if_fail (SWFDEC_IS_VALID_XML_NODE (node));

  num = swfdec_xml_node_num_children (node);

  for (i = 0; i < num; i++) {
    swfdec_xml_node_removeNode (swfdec_xml_node_get_child (node, 0));
  }
}

static void
swfdec_xml_node_insertAt (SwfdecXmlNode *node, SwfdecXmlNode *child, gint32 ind)
{
  SwfdecAsValue val;

  g_assert (SWFDEC_IS_VALID_XML_NODE (node));
  g_assert (SWFDEC_IS_VALID_XML_NODE (child));
  g_assert (ind >= 0);

  // remove the previous parent of the child
  swfdec_xml_node_removeNode (child);

  // insert child to node's childNodes array
  SWFDEC_AS_VALUE_SET_OBJECT (&val, SWFDEC_AS_OBJECT (child));
  swfdec_as_array_insert (node->children, ind, &val);
  swfdec_xml_node_update_childNodes (node);

  // set node as parent of child
  child->parent = node;
}

SWFDEC_AS_NATIVE (253, 3, swfdec_xml_node_insertBefore)
void
swfdec_xml_node_insertBefore (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  gint32 i;
  SwfdecAsObject *child, *point;

  if (!SWFDEC_IS_VALID_XML_NODE (object))
    return;

  if (argc < 2)
    return;

  if (!SWFDEC_AS_VALUE_IS_OBJECT (&argv[0]))
    return;

  child = SWFDEC_AS_VALUE_GET_OBJECT (&argv[0]);
  if (!SWFDEC_IS_VALID_XML_NODE (child))
    return;

  // special case
  if (swfdec_xml_node_index_of_child (SWFDEC_XML_NODE (object),
	SWFDEC_XML_NODE (child)) != -1)
    return;

  if (!SWFDEC_AS_VALUE_IS_OBJECT (&argv[1]))
    return;

  point = SWFDEC_AS_VALUE_GET_OBJECT (&argv[1]);
  if (!SWFDEC_IS_VALID_XML_NODE (point))
    return;

  i = swfdec_xml_node_index_of_child (SWFDEC_XML_NODE (object),
      SWFDEC_XML_NODE (point));

  if (i != -1) {
    swfdec_xml_node_insertAt (SWFDEC_XML_NODE (object),
	SWFDEC_XML_NODE (child), i);
  }
}

void
swfdec_xml_node_appendChild (SwfdecXmlNode *node, SwfdecXmlNode *child)
{
  SwfdecAsValue val;

  g_return_if_fail (SWFDEC_IS_VALID_XML_NODE (node));
  g_return_if_fail (SWFDEC_IS_VALID_XML_NODE (child));
  g_return_if_fail (node->children != NULL);

  // remove the previous parent of the child
  swfdec_xml_node_removeNode (child);

  // append child to node's childNodes array
  SWFDEC_AS_VALUE_SET_OBJECT (&val, SWFDEC_AS_OBJECT (child));
  swfdec_as_array_push (node->children, &val);
  swfdec_xml_node_update_childNodes (node);

  // set node as parent of child
  child->parent = node;
}

SWFDEC_AS_NATIVE (253, 4, swfdec_xml_node_do_appendChild)
void
swfdec_xml_node_do_appendChild (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecAsObject *child;

  if (!SWFDEC_IS_VALID_XML_NODE (object))
    return;

  if (argc < 1)
    return;

  if (!SWFDEC_AS_VALUE_IS_OBJECT (&argv[0]))
    return;

  child = SWFDEC_AS_VALUE_GET_OBJECT (&argv[0]);
  if (!SWFDEC_IS_VALID_XML_NODE (child))
    return;

  // special case
  if (swfdec_xml_node_index_of_child (SWFDEC_XML_NODE (object),
	SWFDEC_XML_NODE (child)) != -1)
    return;

  swfdec_xml_node_appendChild (SWFDEC_XML_NODE (object),
      SWFDEC_XML_NODE (child));
}

SWFDEC_AS_NATIVE (253, 5, swfdec_xml_node_hasChildNodes)
void
swfdec_xml_node_hasChildNodes (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  if (!SWFDEC_IS_VALID_XML_NODE (object))
    return;

  if (swfdec_xml_node_num_children (SWFDEC_XML_NODE (object)) > 0) {
    SWFDEC_AS_VALUE_SET_BOOLEAN (ret, TRUE);
  } else {
    SWFDEC_AS_VALUE_SET_BOOLEAN (ret, FALSE);
  }
}

static gboolean
swfdec_xml_node_foreach_string_append_attribute (SwfdecAsObject *object,
    const char *variable, SwfdecAsValue *value, guint flags, gpointer data)
{
  GString *string = *(GString **)data;
  char *escaped;

  string = g_string_append (string, " ");
  string = g_string_append (string, variable);
  string = g_string_append (string, "=\"");
  escaped =
    swfdec_xml_escape (swfdec_as_value_to_string (object->context, value));
  string = g_string_append (string, escaped);
  g_free (escaped);
  string = g_string_append (string, "\"");

  return TRUE;
}

static const char *
swfdec_xml_node_toString (SwfdecXmlNode *node)
{
  GString *string;
  SwfdecAsObject *object;

  g_assert (SWFDEC_IS_VALID_XML_NODE (node));

  object = SWFDEC_AS_OBJECT (node);

  string = g_string_new ("");
  if (SWFDEC_IS_XML (node)) {
    if (SWFDEC_XML (node)->xmlDecl != NULL)
      string = g_string_append (string, SWFDEC_XML (node)->xmlDecl);
    if (SWFDEC_XML (node)->docTypeDecl != NULL)
      string = g_string_append (string, SWFDEC_XML (node)->docTypeDecl);
  }

  switch (node->type) {
    case SWFDEC_XML_NODE_ELEMENT:
      {
	SwfdecXmlNode *child;
	gint32 i, num;
	gboolean visible;

	if (node->name == NULL) {
	  visible = FALSE;
	} else {
	  visible = TRUE;
	}

	if (visible) {
	  string = g_string_append (string, "<");
	  string = g_string_append (string, node->name);

	  swfdec_as_object_foreach (node->attributes,
	      swfdec_xml_node_foreach_string_append_attribute, &string);
	}

	num = swfdec_xml_node_num_children (node);

	if (num > 0) {
	  if (visible)
	    string = g_string_append (string, ">");

	  for (i = 0; i < num; i++) {
	    child = swfdec_xml_node_get_child (node, i);
	    g_assert (child != NULL);
	    string = g_string_append (string, swfdec_xml_node_toString (child));
	  }

	  if (visible) {
	    string = g_string_append (string, "</");
	    string = g_string_append (string, node->name);
	    string = g_string_append (string, ">");
	  }
	} else {
	  if (visible)
	    string = g_string_append (string, " />");
	}

	break;
      }
    case SWFDEC_XML_NODE_TEXT:
    default:
      {
	char *escaped = swfdec_xml_escape (node->value);
	string = g_string_append (string, escaped);
	g_free (escaped);
	break;
      }
  }

  return swfdec_as_context_give_string (object->context,
      g_string_free (string, FALSE));
}

SWFDEC_AS_NATIVE (253, 6, swfdec_xml_node_do_toString)
void
swfdec_xml_node_do_toString (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  if (!SWFDEC_IS_VALID_XML_NODE (object))
    return;

  SWFDEC_AS_VALUE_SET_STRING (ret,
      swfdec_xml_node_toString (SWFDEC_XML_NODE (object)));
}

void
swfdec_xml_node_init_properties (SwfdecXmlNode *node, int type,
    const char* value)
{
  SwfdecAsObject *object;

  g_return_if_fail (SWFDEC_IS_XML_NODE (node));
  g_return_if_fail (value != NULL);

  object = SWFDEC_AS_OBJECT (node);

  node->valid = TRUE;
  node->parent = NULL;
  // FIXME: use _global.Array constructor?
  node->children = SWFDEC_AS_ARRAY (swfdec_as_array_new (object->context));
  node->attributes = swfdec_as_object_new_empty (object->context);
  node->type = type;
  if (node->type == SWFDEC_XML_NODE_ELEMENT) {
    node->name = value;
  } else {
    node->value = value;
  }

  node->childNodes = SWFDEC_AS_ARRAY (swfdec_as_array_new (object->context));
}

/**
 * swfdec_xml_node_new:
 * @player: a #SwfdecPlayer
 * @type: the #SwfdecXmlNodeType
 * @value: initial value of the node
 *
 * Creates a new #SwfdecXmlNode.
 *
 * Returns: The new XML node or %NULL on OOM
 **/
SwfdecXmlNode *
swfdec_xml_node_new (SwfdecAsContext *context, SwfdecXmlNodeType type,
    const char* value)
{
  SwfdecAsValue val;
  SwfdecXmlNode *node;
  guint size;

  g_return_val_if_fail (SWFDEC_IS_AS_CONTEXT (context), NULL);

  size = sizeof (SwfdecXmlNode);
  if (!swfdec_as_context_use_mem (context, size))
    return NULL;
  node = g_object_new (SWFDEC_TYPE_XML_NODE, NULL);
  swfdec_as_object_add (SWFDEC_AS_OBJECT (node), context, size);
  swfdec_as_object_get_variable (context->global, SWFDEC_AS_STR_XMLNode, &val);
  if (!SWFDEC_AS_VALUE_IS_OBJECT (&val))
    return NULL;
  swfdec_as_object_set_constructor (SWFDEC_AS_OBJECT (node), SWFDEC_AS_VALUE_GET_OBJECT (&val));

  swfdec_xml_node_init_properties (node, type,
      swfdec_as_context_get_string (SWFDEC_AS_OBJECT (node)->context, value));

  return node;
}

SWFDEC_AS_CONSTRUCTOR (253, 0, swfdec_xml_node_construct, swfdec_xml_node_get_type)
void
swfdec_xml_node_construct (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  if (!swfdec_as_context_is_constructing (cx)) {
    SWFDEC_FIXME ("What do we do if not constructing?");
    return;
  }

  if (argc < 2)
    return;

  // special case
  if (SWFDEC_AS_VALUE_IS_UNDEFINED (&argv[0]) ||
      SWFDEC_AS_VALUE_IS_UNDEFINED (&argv[1]))
    return;

  g_assert (SWFDEC_IS_XML_NODE (object));

  swfdec_xml_node_init_properties (SWFDEC_XML_NODE (object),
      swfdec_as_value_to_integer (cx, &argv[0]),
      swfdec_as_value_to_string (cx, &argv[1]));

  SWFDEC_AS_VALUE_SET_OBJECT (ret, object);
}

static void
swfdec_xml_node_add_variable (SwfdecAsObject *object, const char *variable,
    SwfdecAsNative get, SwfdecAsNative set)
{
  SwfdecAsFunction *get_func, *set_func;

  g_return_if_fail (SWFDEC_IS_AS_OBJECT (object));
  g_return_if_fail (variable != NULL);
  g_return_if_fail (get != NULL);

  get_func =
    swfdec_as_native_function_new (object->context, variable, get, 0, NULL);
  if (get_func == NULL)
    return;

  if (set != NULL) {
    set_func =
      swfdec_as_native_function_new (object->context, variable, set, 0, NULL);
  } else {
    set_func = NULL;
  }

  swfdec_as_object_add_variable (object, variable, get_func, set_func);
}

void
swfdec_xml_node_init_native (SwfdecPlayer *player, guint version)
{
  SwfdecAsContext *context;
  SwfdecAsValue val;
  SwfdecAsObject *node;
  SwfdecAsObject *proto;

  g_return_if_fail (SWFDEC_IS_PLAYER (player));

  context = SWFDEC_AS_CONTEXT (player);
  swfdec_as_object_get_variable (context->global, SWFDEC_AS_STR_XMLNode, &val);
  g_return_if_fail (SWFDEC_AS_VALUE_IS_OBJECT (&val));
  node = SWFDEC_AS_VALUE_GET_OBJECT (&val);

  swfdec_as_object_get_variable (node, SWFDEC_AS_STR_prototype, &val);
  g_return_if_fail (SWFDEC_AS_VALUE_IS_OBJECT (&val));
  proto = SWFDEC_AS_VALUE_GET_OBJECT (&val);

  swfdec_xml_node_add_variable (proto, SWFDEC_AS_STR_nodeType,
      swfdec_xml_node_get_nodeType, NULL);
  swfdec_xml_node_add_variable (proto, SWFDEC_AS_STR_nodeValue,
      swfdec_xml_node_get_nodeValue, swfdec_xml_node_set_nodeValue);
  swfdec_xml_node_add_variable (proto, SWFDEC_AS_STR_nodeName,
      swfdec_xml_node_get_nodeName, swfdec_xml_node_set_nodeName);
  swfdec_xml_node_add_variable (proto, SWFDEC_AS_STR_prefix,
      swfdec_xml_node_do_get_prefix, NULL);
  swfdec_xml_node_add_variable (proto, SWFDEC_AS_STR_localName,
      swfdec_xml_node_get_localName, NULL);
  swfdec_xml_node_add_variable (proto, SWFDEC_AS_STR_namespaceURI,
      swfdec_xml_node_get_namespaceURI, NULL);
  swfdec_xml_node_add_variable (proto, SWFDEC_AS_STR_attributes,
      swfdec_xml_node_get_attributes, NULL);
  swfdec_xml_node_add_variable (proto, SWFDEC_AS_STR_parentNode,
      swfdec_xml_node_get_parentNode, NULL);
  swfdec_xml_node_add_variable (proto, SWFDEC_AS_STR_previousSibling,
      swfdec_xml_node_get_previousSibling, NULL);
  swfdec_xml_node_add_variable (proto, SWFDEC_AS_STR_nextSibling,
      swfdec_xml_node_get_nextSibling, NULL);
  swfdec_xml_node_add_variable (proto, SWFDEC_AS_STR_firstChild,
      swfdec_xml_node_get_firstChild, NULL);
  swfdec_xml_node_add_variable (proto, SWFDEC_AS_STR_lastChild,
      swfdec_xml_node_get_lastChild, NULL);
  swfdec_xml_node_add_variable (proto, SWFDEC_AS_STR_childNodes,
      swfdec_xml_node_get_childNodes, NULL);
}
