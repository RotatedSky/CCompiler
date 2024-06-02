
#include "compiler.h"
#include <stdio.h>
#include <stdlib.h>

compile_process_s *compile_process_create(const char *filename, const char* filename_out, int flags)
{
    FILE* fp = fopen(filename, "r");
    if (NULL == fp)
    {
        return NULL;
    }

    FILE* fp_out = NULL;
    if (filename_out != NULL)
    {
        fp_out = fopen(filename_out, "w");
        if (NULL == fp_out)
        {
            return NULL;
        }
    }

    compile_process_s *process = calloc(1, sizeof(compile_process_s));
    process->flags = flags;
    process->cfile.fp = fp;
    process->ofp = fp_out;
    return process;
}

char compile_process_next_char(lex_process_s *lex_process)
{
    compile_process_s *compiler = lex_process->compiler;
    compiler->pos.col += 1;
    char c = getc(compiler->cfile.fp);
    if (c == '\n')
    {
        compiler->pos.line += 1;
        compiler->pos.col = 0;
    }

    return c;
}

char compile_process_peek_char(lex_process_s *lex_process)
{
    compile_process_s *compiler = lex_process->compiler;
    char c = getc(compiler->cfile.fp);
    ungetc(c, compiler->cfile.fp);
    return c;
}

void compile_process_push_char(lex_process_s *lex_process, char c)
{
    compile_process_s *compiler = lex_process->compiler;
    ungetc(c, compiler->cfile.fp);
}
