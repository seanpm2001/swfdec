/* Vivified
 * Copyright (C) 2008 Pekka Lampila <pekka.lampila@iki.fi>
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

#include "vivi_code_increment.h"
#include "vivi_code_printer.h"
#include "vivi_code_compiler.h"
#include "vivi_code_asm_code_default.h"

G_DEFINE_TYPE (ViviCodeIncrement, vivi_code_increment, VIVI_TYPE_CODE_VALUE)

static void
vivi_code_increment_dispose (GObject *object)
{
  ViviCodeIncrement *increment = VIVI_CODE_INCREMENT (object);

  g_object_unref (increment->value);

  G_OBJECT_CLASS (vivi_code_increment_parent_class)->dispose (object);
}

static void
vivi_code_increment_print (ViviCodeToken *token, ViviCodePrinter *printer)
{
  ViviCodeIncrement *increment = VIVI_CODE_INCREMENT (token);

  vivi_code_printer_print_value (printer, increment->value,
      VIVI_PRECEDENCE_INCREMENT);
  vivi_code_printer_print (printer, "++");
  vivi_code_printer_new_line (printer, FALSE);
}

static void
vivi_code_increment_compile (ViviCodeToken *token, ViviCodeCompiler *compiler)
{
  ViviCodeIncrement *increment = VIVI_CODE_INCREMENT (token);
  ViviCodeAsm *code;

  vivi_code_compiler_compile_value (compiler, increment->value);

  code = vivi_code_asm_increment_new ();
  vivi_code_compiler_add_code (compiler, code);
  g_object_unref (code);
}

static void
vivi_code_increment_class_init (ViviCodeIncrementClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ViviCodeTokenClass *token_class = VIVI_CODE_TOKEN_CLASS (klass);

  object_class->dispose = vivi_code_increment_dispose;

  token_class->print = vivi_code_increment_print;
  token_class->compile = vivi_code_increment_compile;
}

static void
vivi_code_increment_init (ViviCodeIncrement *increment)
{
}

ViviCodeValue *
vivi_code_increment_new (ViviCodeValue *value)
{
  ViviCodeIncrement *increment;

  g_return_val_if_fail (VIVI_IS_CODE_VALUE (value), NULL);

  increment = g_object_new (VIVI_TYPE_CODE_INCREMENT, NULL);
  increment->value = g_object_ref (value);

  return VIVI_CODE_VALUE (increment);
}
