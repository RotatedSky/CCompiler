#include "compiler.h"
#include "helpers/vector.h"
#include <stdlib.h>

lex_process_s *lex_process_create(compile_process_s *compiler, lex_process_functions_s *functions, void *private)
{
    lex_process_s *process = calloc(1, sizeof(lex_process_s));
    process->function = functions;
    process->token_vec = vector_create(sizeof(token_s));
    process->compiler = compiler;
    process->private = private;
    process->pos.line = 1;
    process->pos.col = 1;
    return process;
}

void lex_process_free(lex_process_s *process)
{
    vector_free(process->token_vec);
    free(process);
}

void *lex_process_private(lex_process_s *process)
{
    return process->private;
}

vector_s *lex_process_tokens(lex_process_s *process)
{
    return process->token_vec;
}
