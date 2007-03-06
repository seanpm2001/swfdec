/* Swfdec
 * Copyright (C) 2006 Benjamin Otte <otte@gnome.org>
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

#include <stdlib.h>
#include <string.h>
#include <libswfdec/swfdec_debugger.h>
#include <libswfdec/swfdec_js.h>
#include <libswfdec/js/jsdbgapi.h>
#include <libswfdec/js/jsinterp.h>
#include "swfdec_player_manager.h"
#include "swfdec_source.h"

enum {
  PROP_0,
  PROP_PLAYING,
  PROP_SPEED,
  PROP_INTERRUPTED
};

enum {
  MESSAGE,
  LAST_SIGNAL
};
guint signals[LAST_SIGNAL];

/*** command handling ***/

typedef enum {
  SWFDEC_MESSAGE_INPUT,
  SWFDEC_MESSAGE_OUTPUT,
  SWFDEC_MESSAGE_ERROR
} SwfdecMessageType;

static void
swfdec_player_manager_send_message (SwfdecPlayerManager *manager,
    SwfdecMessageType type, char *format, ...) G_GNUC_PRINTF (3, 4);
static void
swfdec_player_manager_send_message (SwfdecPlayerManager *manager,
    SwfdecMessageType type, char *format, ...)
{
  va_list args;
  char *msg;

  va_start (args, format);
  msg = g_strdup_vprintf (format, args);
  va_end (args);
  g_signal_emit (manager, signals[MESSAGE], 0, (guint) type, msg);
  g_free (msg);
}
#define swfdec_player_manager_output(manager, ...) \
  swfdec_player_manager_send_message (manager, SWFDEC_MESSAGE_OUTPUT, __VA_ARGS__)
#define swfdec_player_manager_error(manager, ...) \
  swfdec_player_manager_send_message (manager, SWFDEC_MESSAGE_ERROR, __VA_ARGS__)

/*** SWFDEC_PLAYER_MANAGER ***/

G_DEFINE_TYPE (SwfdecPlayerManager, swfdec_player_manager, G_TYPE_OBJECT)

static void
swfdec_player_manager_get_property (GObject *object, guint param_id, GValue *value, 
    GParamSpec * pspec)
{
  SwfdecPlayerManager *manager = SWFDEC_PLAYER_MANAGER (object);
  
  switch (param_id) {
    case PROP_PLAYING:
      g_value_set_boolean (value, swfdec_player_manager_get_playing (manager));
      break;
    case PROP_SPEED:
      g_value_set_double (value, swfdec_player_manager_get_speed (manager));
      break;
    case PROP_INTERRUPTED:
      g_value_set_boolean (value, swfdec_player_manager_get_interrupted (manager));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
  }
}

static void
swfdec_player_manager_set_property (GObject *object, guint param_id, const GValue *value,
    GParamSpec *pspec)
{
  SwfdecPlayerManager *manager = SWFDEC_PLAYER_MANAGER (object);

  switch (param_id) {
    case PROP_PLAYING:
      swfdec_player_manager_set_playing (manager, g_value_get_boolean (value));
      break;
    case PROP_SPEED:
      swfdec_player_manager_set_speed (manager, g_value_get_double (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
  }
}

static void breakpoint_hit_cb (SwfdecDebugger *debugger, guint id, SwfdecPlayerManager *manager);

static void
trace_cb (SwfdecPlayer *player, const char *str, SwfdecPlayerManager *manager)
{
  swfdec_player_manager_output (manager, "Trace: %s", str);
}

static void
swfdec_player_manager_set_player (SwfdecPlayerManager *manager, SwfdecPlayer *player)
{
  if (manager->player == player)
    return;

  if (manager->player) {
    g_signal_handlers_disconnect_by_func (manager->player, breakpoint_hit_cb, manager);
    g_object_unref (manager->player);
  }
  manager->player = player;
  if (player) {
    g_object_ref (player);
    g_signal_connect (player, "breakpoint", G_CALLBACK (breakpoint_hit_cb), manager);
    g_signal_connect (player, "trace", G_CALLBACK (trace_cb), manager);
  }
}

static void
swfdec_player_manager_dispose (GObject *object)
{
  SwfdecPlayerManager *manager = SWFDEC_PLAYER_MANAGER (object);

  swfdec_player_manager_set_playing (manager, FALSE);
  swfdec_player_manager_set_player (manager, FALSE);

  G_OBJECT_CLASS (swfdec_player_manager_parent_class)->dispose (object);
}

static void
swfdec_player_manager_class_init (SwfdecPlayerManagerClass * g_class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (g_class);

  object_class->dispose = swfdec_player_manager_dispose;
  object_class->set_property = swfdec_player_manager_set_property;
  object_class->get_property = swfdec_player_manager_get_property;

  g_object_class_install_property (object_class, PROP_SPEED,
      g_param_spec_double ("speed", "speed", "playback speed of movie",
	  G_MINDOUBLE, 16.0, 1.0, G_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_PLAYING,
      g_param_spec_boolean ("playing", "playing", "if the movie is played back",
	  FALSE, G_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_INTERRUPTED,
      g_param_spec_boolean ("interrupted", "interrupted", "TRUE if we're handling a breakpoint",
	  FALSE, G_PARAM_READABLE));

  signals[MESSAGE] = g_signal_new ("message", G_TYPE_FROM_CLASS (g_class),
      G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__UINT_POINTER, /* FIXME */
      G_TYPE_NONE, 2, G_TYPE_UINT, G_TYPE_POINTER);
}

static void
swfdec_player_manager_init (SwfdecPlayerManager *manager)
{
  manager->speed = 1.0;
}

SwfdecPlayerManager *
swfdec_player_manager_new (SwfdecPlayer *player)
{
  SwfdecPlayerManager *manager;
  
  g_return_val_if_fail (player == NULL || SWFDEC_IS_PLAYER (player), NULL);

  manager = g_object_new (SWFDEC_TYPE_PLAYER_MANAGER, 0);
  swfdec_player_manager_set_player (manager, player);

  return manager;
}

void
swfdec_player_manager_set_speed (SwfdecPlayerManager *manager, double speed)
{
  g_return_if_fail (SWFDEC_IS_PLAYER_MANAGER (manager));
  g_return_if_fail (speed > 0.0);

  if (manager->speed == speed)
    return;
  if (manager->source) {
    swfdec_player_manager_set_playing (manager, FALSE);
    manager->speed = speed;
    swfdec_player_manager_set_playing (manager, TRUE);
  } else {
    manager->speed = speed;
  }
  g_object_notify (G_OBJECT (manager), "speed");
}

double		
swfdec_player_manager_get_speed (SwfdecPlayerManager *manager)
{
  g_return_val_if_fail (SWFDEC_IS_PLAYER_MANAGER (manager), 1.0);

  return manager->speed;
}

gboolean
swfdec_player_manager_get_interrupted (SwfdecPlayerManager *manager)
{
  g_return_val_if_fail (SWFDEC_IS_PLAYER_MANAGER (manager), FALSE);

  return manager->interrupt_loop != NULL;
}

void
swfdec_player_manager_get_interrupt (SwfdecPlayerManager *manager,
    SwfdecDebuggerScript **script, guint *line)
{
  g_return_if_fail (SWFDEC_IS_PLAYER_MANAGER (manager));
  g_return_if_fail (swfdec_player_manager_get_interrupted (manager));

  if (script)
    *script = manager->interrupt_script;
  if (line)
    *line = manager->interrupt_line;
}

static void
swfdec_player_manager_update_playing (SwfdecPlayerManager *manager)
{
  gboolean should_have_source = manager->playing && 
    !swfdec_player_manager_get_interrupted (manager);

  if (should_have_source && manager->source == NULL) {
    manager->source = swfdec_iterate_source_new (manager->player, manager->speed);
    g_source_attach (manager->source, NULL);
  } else if (!should_have_source && manager->source != NULL) {
    g_source_destroy (manager->source);
    g_source_unref (manager->source);
    manager->source = NULL;
  }
}

void
swfdec_player_manager_set_playing (SwfdecPlayerManager *manager, gboolean playing)
{
  g_return_if_fail (SWFDEC_IS_PLAYER_MANAGER (manager));

  if (manager->playing == playing)
    return;
  manager->playing = playing;
  swfdec_player_manager_update_playing (manager);
  g_object_notify (G_OBJECT (manager), "playing");
}

gboolean
swfdec_player_manager_get_playing (SwfdecPlayerManager *manager)
{
  g_return_val_if_fail (SWFDEC_IS_PLAYER_MANAGER (manager), FALSE);

  return manager->playing;
}

guint
swfdec_player_manager_iterate (SwfdecPlayerManager *manager)
{
  guint msecs;

  g_return_val_if_fail (SWFDEC_IS_PLAYER_MANAGER (manager), 0);
  g_return_val_if_fail (!swfdec_player_manager_get_interrupted (manager), 0);

  swfdec_player_manager_set_playing (manager, FALSE);
  msecs = swfdec_player_get_next_event (manager->player);
  if (msecs)
    swfdec_player_advance (manager->player, msecs);

  return msecs;
}

static void
swfdec_player_manager_do_interrupt (SwfdecPlayerManager *manager)
{
  swfdec_debugger_set_stepping (SWFDEC_DEBUGGER (manager->player), FALSE);
  g_object_ref (manager);
  manager->interrupt_loop = g_main_loop_new (NULL, FALSE);
  swfdec_player_manager_update_playing (manager);
  g_object_notify (G_OBJECT (manager), "interrupted");
  g_main_loop_run (manager->interrupt_loop);
  g_main_loop_unref (manager->interrupt_loop);
  manager->interrupt_loop = NULL;
  g_object_notify (G_OBJECT (manager), "interrupted");
  swfdec_player_manager_update_playing (manager);
  g_object_unref (manager);
}

void
swfdec_player_manager_next (SwfdecPlayerManager *manager)
{
  g_return_if_fail (SWFDEC_IS_PLAYER_MANAGER (manager));
  g_return_if_fail (swfdec_player_manager_get_interrupted (manager));

  swfdec_debugger_set_stepping (SWFDEC_DEBUGGER (manager->player), TRUE);
  swfdec_player_manager_continue (manager);
}

void
swfdec_player_manager_continue (SwfdecPlayerManager *manager)
{
  g_return_if_fail (SWFDEC_IS_PLAYER_MANAGER (manager));
  g_return_if_fail (swfdec_player_manager_get_interrupted (manager));

  g_main_loop_quit (manager->interrupt_loop);
}

/*** commands ***/

const char *
parse_skip (const char *input)
{
  g_assert (input);
  if (g_ascii_isspace (*input))
    input++;
  return input;
}

const char *
parse_string (const char *input, char **output)
{
  const char *start = input;

  g_assert (output);
  if (input == NULL)
    return NULL;

  while (*input && !g_ascii_isspace (*input))
    input++;
  if (input == start)
    return NULL;
  *output = g_strndup (start, input - start);
  return parse_skip (input);
}

const char *
parse_uint (const char *input, guint *output)
{
  char *end;
  guint result;

  g_assert (output);
  if (input == NULL)
    return NULL;

  result = strtoul (input, &end, 10);
  if (input == end || (*end != '\0' && !g_ascii_isspace (*end)))
    return NULL;
  *output = result;
  return parse_skip (end);
}

static void
breakpoint_hit_cb (SwfdecDebugger *debugger, guint id, SwfdecPlayerManager *manager)
{
  if (id == 0) {
    if (!swfdec_debugger_get_current (debugger,
	  &manager->interrupt_script, &manager->interrupt_line)) {
      g_assert_not_reached ();
    }
  } else {
    if (!swfdec_debugger_get_breakpoint (debugger, id,
	  &manager->interrupt_script, &manager->interrupt_line)) {
      g_assert_not_reached ();
    }
    swfdec_player_manager_output (manager, "Breakpoint %u", id);
  }
  swfdec_player_manager_do_interrupt (manager);
}

static void
command_print (SwfdecPlayerManager *manager, const char *arg)
{
  jsval rval;

  if (swfdec_js_run (manager->player, arg, &rval)) {
    const char *s;
    s = swfdec_js_to_string (manager->player->jscx, rval);
    if (s)
      swfdec_player_manager_output (manager, "%s", s);
    else
      swfdec_player_manager_error (manager, "Invalid return value");
  } else {
    swfdec_player_manager_error (manager, "Invalid command");
  }
}

static void
command_play (SwfdecPlayerManager *manager, const char *arg)
{
  swfdec_player_manager_set_playing (manager, TRUE);
}

static void
command_stop (SwfdecPlayerManager *manager, const char *arg)
{
  swfdec_player_manager_set_playing (manager, FALSE);
}

static void
command_iterate (SwfdecPlayerManager *manager, const char *arg)
{
  guint msecs = swfdec_player_manager_iterate (manager);

  if (msecs == 0) {
    swfdec_player_manager_error (manager, "Cannot iterate this player");
  } else {
    swfdec_player_manager_error (manager, "advanced player %u.%03us",
	msecs / 1000, msecs % 1000);
  }
}

static void
command_continue (SwfdecPlayerManager *manager, const char *arg)
{
  if (swfdec_player_manager_get_interrupted (manager))
    swfdec_player_manager_continue (manager);
  else
    swfdec_player_manager_error (manager, "Not interrupted, cannot continue");
}

static void
set_breakpoint (gpointer script, gpointer debugger)
{
  swfdec_debugger_set_breakpoint (debugger, script, 0);
}

static void
command_next (SwfdecPlayerManager *manager, const char *arg)
{
  if (!swfdec_player_manager_get_interrupted (manager))
    swfdec_player_manager_error (manager, "Not interrupted, cannot continue");
  else 
    swfdec_player_manager_next (manager);
}

static void
command_break (SwfdecPlayerManager *manager, const char *arg)
{
  char *str;
  const char *next;
  guint line;

  next = parse_uint (arg, &line);
  if (next) {
    if (swfdec_player_manager_get_interrupted (manager)) {
      if (line < manager->interrupt_script->n_commands) {
	guint id = swfdec_debugger_set_breakpoint (SWFDEC_DEBUGGER (manager->player),
	    manager->interrupt_script, line);
	swfdec_player_manager_output (manager, "%u: %s line %u: %s",
	    id, manager->interrupt_script->script->name, line, 
	    manager->interrupt_script->commands[line].description);
      } else {
	swfdec_player_manager_error (manager, 
	    "Can't set breakpoint at line %u, script only has %u lines",
	    line, manager->interrupt_script->n_commands);
      }
    } else {
      swfdec_player_manager_error (manager, "Not interrupted");
    }
    return;
  }
  next = parse_string (arg, &str);
  if (next) {
    if (strcasecmp (str, "start") == 0) {
      swfdec_debugger_foreach_script (SWFDEC_DEBUGGER (manager->player), 
	  set_breakpoint, manager->player);
      swfdec_player_manager_output (manager, "set breakpoint at start of every script");
    } else {
      swfdec_player_manager_error (manager, "FIXME: implement");
    }
  }
}

static void
command_breakpoints (SwfdecPlayerManager *manager, const char *arg)
{
  guint i, n, line;
  SwfdecDebugger *debugger;
  SwfdecDebuggerScript *script;

  debugger = SWFDEC_DEBUGGER (manager->player);
  n = swfdec_debugger_get_n_breakpoints (debugger);
  for (i = 1; i <= n; i++) {
    if (swfdec_debugger_get_breakpoint (debugger, i, &script, &line)) {
      swfdec_player_manager_output (manager, "%u: %s line %u: %s",
	  i, script->script->name, line, script->commands[line].description);
    }
  }
}

static void
command_delete (SwfdecPlayerManager *manager, const char *arg)
{
  guint id;

  if (arg == NULL) {
    guint i, n;
    SwfdecDebugger *debugger = SWFDEC_DEBUGGER (manager->player);
    n = swfdec_debugger_get_n_breakpoints (debugger);
    for (i = 1; i <= n; i++) {
      swfdec_debugger_unset_breakpoint (debugger, i);
    }
  } else if (parse_uint (arg, &id)) {
    swfdec_debugger_unset_breakpoint (SWFDEC_DEBUGGER (manager->player), id);
  } else {
    swfdec_player_manager_error (manager, "no breakpoint '%s'", arg);
  }
}

static void
command_stack (SwfdecPlayerManager *manager, const char *arg)
{
  JSStackFrame *frame = NULL;
  guint i, min, max;

  if (!swfdec_player_manager_get_interrupted (manager)) {
    swfdec_player_manager_error (manager, "Not interrupted");
    return;
  }
  JS_FrameIterator (manager->player->jscx, &frame);
  min = 1;
  max = frame->sp - frame->spbase;
  for (i = min; i <= max; i++) {
    const char *s = swfdec_js_to_string (manager->player->jscx, frame->sp[-i]);
    swfdec_player_manager_output (manager, "%2u: %s", i, s);
  }
}

typedef struct {
  SwfdecPlayerManager *manager;
  const char *	       string;
} FindData;

static void
do_find (gpointer scriptp, gpointer datap)
{
  SwfdecDebuggerScript *script = scriptp;
  FindData *data = datap;
  guint i;

  for (i = 0; i < script->n_commands; i++) {
    if (strstr (script->commands[i].description, data->string)) {
      swfdec_player_manager_output (data->manager, "%s %u: %s",
	  script->script->name, i, script->commands[i].description);
    }
  }
}

static void
command_find (SwfdecPlayerManager *manager, const char *arg)
{
  FindData data = { manager, arg };
  if (arg == NULL) {
    swfdec_player_manager_error (manager, "What should be found?");
  }
  swfdec_debugger_foreach_script (SWFDEC_DEBUGGER (manager->player), do_find, &data);
}

static void
command_enumerate (SwfdecPlayerManager *manager, const char *arg)
{
  jsval rval;
  JSObject *obj;
  JSIdArray *array;
  const char *s, *t;
  int i;

  if (!swfdec_js_run (manager->player, arg, &rval)) {
    swfdec_player_manager_error (manager, "Invalid command");
    return;
  }
  if (!JSVAL_IS_OBJECT (rval)) {
    swfdec_player_manager_error (manager, "Given expression is not an object");
    return;
  }
  obj = JSVAL_TO_OBJECT (rval);
  array = JS_Enumerate (manager->player->jscx, obj);
  if (array == NULL) {
    swfdec_player_manager_error (manager, "Error enumerating");
    return;
  }
  s = swfdec_js_to_string (manager->player->jscx, rval);
  if (s == NULL) {
    swfdec_player_manager_error (manager, "Cannot convert object to string");
  }
  swfdec_player_manager_output (manager, "properties for %s:", s);
  for (i = 0; i < array->length; i++) {
    if (!JS_IdToValue (manager->player->jscx, array->vector[i], &rval))
      continue;
    s = swfdec_js_to_string (manager->player->jscx, rval);
    if (s == NULL)
      continue;
    if (!JS_GetProperty (manager->player->jscx, obj, s, &rval) ||
	!(t = swfdec_js_to_string (manager->player->jscx, rval)))
      t = "<error querying value>";
    swfdec_player_manager_output (manager, "  %s: %s", s, t);
  }
  JS_DestroyIdArray (manager->player->jscx, array);
}

static void command_help (SwfdecPlayerManager *manager, const char *arg);
/* NB: the first word in the command string is used, partial matches are ok */
struct {
  const char *	name;
  void		(* func)	(SwfdecPlayerManager *manager, const char *arg);
  const char *	description;
} commands[] = {
  { "help",	command_help,		"print all available commands and a quick description" },
  { "print",	command_print,	  	"evaluate the argument as a JavaScript script" },
  { "play",	command_play,		"play the movie" },
  { "stop",	command_stop,	 	"stop the movie" },
  { "iterate",	command_iterate,	"iterate the movie once" },
  { "break",	command_break,	  	"add breakpoint at [start]" },
  { "breakpoints", command_breakpoints,	"show all breakpoints" },
  { "delete",	command_delete,		"delete a breakpoint" },
  { "continue",	command_continue,	"continue when stopped inside a breakpoint" },
  { "next",	command_next,		"step forward one command when stopped inside a breakpoint" },
  { "stack",	command_stack,		"print the arguments on the stack" },
  { "find",	command_find,		"find the given argument verbatim in all scripts" },
  { "enumerate",command_enumerate,    	"enumerate all properties of the given object" },
};

static void
command_help (SwfdecPlayerManager *manager, const char *arg)
{
  guint i;
  swfdec_player_manager_output (manager, "The following commands are available:");
  for (i = 0; i < G_N_ELEMENTS (commands); i++) {
    swfdec_player_manager_output (manager, "%s: %s", commands[i].name, commands[i].description);
  }
}

void
swfdec_player_manager_execute (SwfdecPlayerManager *manager, const char *command)
{
  guint i;
  const char *args;
  char *run;

  g_return_if_fail (SWFDEC_IS_PLAYER_MANAGER (manager));
  g_return_if_fail (command != NULL);

  parse_skip (command);
  swfdec_player_manager_send_message (manager, SWFDEC_MESSAGE_INPUT, "%s", command);
  args = parse_string (command, &run);
  if (args == NULL)
    return;
  for (i = 0; i < G_N_ELEMENTS(commands); i++) {
    if (g_ascii_strncasecmp (commands[i].name, run, strlen (run)) == 0) {
      commands[i].func (manager, *args == '\0' ? NULL : args);
      g_free (run);
      return;
    }
  }
  swfdec_player_manager_error (manager, "No such command '%s'", run);
  g_free (run);
}

