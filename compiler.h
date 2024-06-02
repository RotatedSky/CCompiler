#ifndef __CCOMPILER_H__
#define __CCOMPILER_H__

#include <stdio.h>
#include <stdbool.h>

typedef struct _pos_s
{
    int line;
    int col;
    const char *filename;
} pos_s;

typedef enum _token_type_e
{
    TOKEN_TYPE_IDENTIFIER,
    TOKEN_TYPE_KEYWORD,
    TOKEN_TYPE_OPERATOR,
    TOKEN_TYPE_SYMBOL,
    TOKEN_TYPE_NUMBER,
    TOKEN_TYPE_STRING,
    TOKEN_TYPE_COMMENT,
    TOKEN_TYPE_NEWLINE
} token_type_e;

typedef struct _token_s
{
    int type;
    int flags;

    union
    {
        char cval;
        const char *sval;
        unsigned int inum;
        unsigned long lnum;
        unsigned long long llnum;
        void *any;
    };

    // True if their is whitespace between the token and the next token
    // i.e. * a for operator token * would mean whitespace would be set for token "a"
    bool whitespace;
    const char *between_brackets;
} token_s;

typedef enum _compiler_result_e
{
    COMPILER_FILE_COMPILED_OK,
    COMPILER_FAILED_WITH_ERRORS
} compiler_result_e;

typedef enum _lex_result_e
{
    LEXICAL_ANALYSIS_ALL_OK,
    LEXICAL_ANALYSIS_INPUT_ERROR
} lex_result_e;

typedef struct _compile_process_input_file_s
{
    FILE *fp;
    const char *abs_path;
} compile_process_input_file_s;

typedef struct _compile_process_s
{
    int flags; ///< The flags in regrads to how this file should be compiled
    pos_s pos;
    compile_process_input_file_s cfile;
    FILE *ofp;
} compile_process_s;

typedef struct _lex_process_s lex_process_s;

typedef struct _lex_process_functions
{
    char (*next_char)(lex_process_s *process);
    char (*peek_char)(lex_process_s *process);
    void (*push_char)(lex_process_s *process, char c);
} lex_process_functions;

typedef struct vector vector_s;
typedef struct buffer buffer_s;

typedef struct _lex_process_s
{
    pos_s pos;
    vector_s *token_vec;
    compile_process_s *compiler;

    int current_expression_count;
    buffer_s *parentheses_buffer;
    lex_process_functions *function;

    // This willl be private data that the lexer does not understand,
    // but the person using the lexer does understand.
    void *private;
} lex_process_s;

int compile_file(const char *filename, const char *out_filename, int flags);
compile_process_s *compile_process_create(const char *filename, const char* filename_out, int flags);

char compile_process_next_char(lex_process_s *lex_process);
char compile_process_peek_char(lex_process_s *lex_process);
void compile_process_push_char(lex_process_s *lex_process, char c);

lex_process_s *lex_process_create(compile_process_s *compiler, lex_process_functions *functions, void *private);
void lex_process_free(lex_process_s *process);
void *lex_process_private(lex_process_s *process);
vector_s *lex_process_tokens(lex_process_s *process);
int lex(lex_process_s *process);

#endif // !__CCOMPILER_H__
