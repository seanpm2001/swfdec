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

#include "vivi_code_compiler.h"

G_DEFINE_TYPE (ViviCodeCompiler, vivi_code_compiler, G_TYPE_OBJECT)

static void
vivi_code_compiler_dispose (GObject *object)
{
  ViviCodeCompiler *compiler = VIVI_CODE_COMPILER (object);

  g_object_unref (compiler->assembler);

  G_OBJECT_CLASS (vivi_code_compiler_parent_class)->dispose (object);
}

static void
vivi_code_compiler_class_init (ViviCodeCompilerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = vivi_code_compiler_dispose;
}

static void
vivi_code_compiler_init (ViviCodeCompiler *compiler)
{
  compiler->assembler = VIVI_CODE_ASSEMBLER (vivi_code_assembler_new ());
}

void
vivi_code_compiler_compile_token (ViviCodeCompiler *compiler,
    ViviCodeToken *token)
{
  g_return_if_fail (VIVI_IS_CODE_COMPILER (compiler));
  g_return_if_fail (VIVI_IS_CODE_TOKEN (token));

  vivi_code_token_compile (token, compiler);
}

void
vivi_code_compiler_add_code (ViviCodeCompiler *compiler, ViviCodeAsm *code)
{
  g_return_if_fail (VIVI_IS_CODE_COMPILER (compiler));
  g_return_if_fail (VIVI_IS_CODE_ASM (code));

  vivi_code_assembler_add_code (compiler->assembler, code);
}

ViviCodeLabel *
vivi_code_compiler_create_label (ViviCodeCompiler *compiler,
    const char *prefix)
{
  g_return_val_if_fail (VIVI_IS_CODE_COMPILER (compiler), NULL);
  g_return_val_if_fail (prefix != NULL, NULL);

  // FIXME
  return VIVI_CODE_LABEL (vivi_code_label_new (prefix));
}

ViviCodeCompiler *
vivi_code_compiler_new (guint version)
{
  ViviCodeCompiler *compiler;

  compiler = g_object_new (VIVI_TYPE_CODE_COMPILER, NULL);
  compiler->version = version;

  return compiler;
}

guint
vivi_code_compiler_get_version (ViviCodeCompiler *compiler)
{
  g_return_val_if_fail (VIVI_IS_CODE_COMPILER (compiler), 0);

  return compiler->version;
}


ViviCodeAssembler *
vivi_code_compiler_get_assembler (ViviCodeCompiler *compiler)
{
  g_return_val_if_fail (VIVI_IS_CODE_COMPILER (compiler), NULL);

  return compiler->assembler;
}