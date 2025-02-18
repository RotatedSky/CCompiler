#include "compiler.h"
#include <stdarg.h>
#include <stdlib.h>

lex_process_functions_s compiler_lex_functions =
{
    .next_char = compile_process_next_char,
    .peek_char = compile_process_peek_char,
    .push_char = compile_process_push_char
};

void compile_error(compile_process_s *compiler, const char *msg, ...)
{
    va_list args;
    va_start(args, msg);
    vfprintf(stderr, msg, args);
    va_end(args);
    fprintf(stderr, " on line %i, col %i in file %s\n", compiler->pos.line, compiler->pos.col, compiler->pos.filename);
    exit(-1);
}

void compile_warning(compile_process_s *compiler, const char *msg, ...)
{
    va_list args;
    va_start(args, msg);
    vfprintf(stderr, msg, args);
    va_end(args);
    fprintf(stderr, " on line %i, col %i in file %s\n", compiler->pos.line, compiler->pos.col, compiler->pos.filename);
}

int compile_file(const char *filename, const char *filename_out, int flags)
{
    compile_process_s *process = compile_process_create(filename, filename_out, flags);
    if (NULL == process)
    {
        return COMPILER_FAILED_WITH_ERRORS;
    }

    // Perform lexical analysis
    lex_process_s *lex_process = lex_process_create(process, &compiler_lex_functions, NULL);
    if (NULL == lex_process)
    {
        return COMPILER_FAILED_WITH_ERRORS;
    }

    if (lex(lex_process) != LEXICAL_ANALYSIS_ALL_OK)
    {
        return COMPILER_FAILED_WITH_ERRORS;
    }

    process->token_vec = lex_process->token_vec;

    // Perform parsing

    // Perform code generation

    return COMPILER_FILE_COMPILED_OK;
}
