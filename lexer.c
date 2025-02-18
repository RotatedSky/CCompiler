#include "compiler.h"
#include "helpers/vector.h"
#include "helpers/buffer.h"
#include <string.h>
#include <assert.h>
#include <ctype.h>

#define LEX_GETC_IF(buffer, c, exp)     \
    for (c = peekc(); exp; c = peekc()) \
    {                                   \
        buffer_write(buffer, c);        \
        nextc();                        \
    }

static lex_process_s *lex_process;
static token_s tmp_token;

token_s *read_next_token();
token_s *token_make_identifier_or_keyword();
static bool _lex_is_in_expression();

static char peekc()
{
    return lex_process->function->peek_char(lex_process);
}

static char nextc()
{
    char c = lex_process->function->next_char(lex_process);
    if (_lex_is_in_expression())
    {
        buffer_write(lex_process->parentheses_buffer, c);
    }
    lex_process->pos.col += 1;
    if (c == '\n')
    {
        lex_process->pos.line += 1;
        lex_process->pos.col = 1;
    }
    return c;
}

static void pushc(char c)
{
    lex_process->function->push_char(lex_process, c);
}

static char assert_next_char(char c)
{
    char next_c = nextc();
    assert(c == next_c);
    return next_c;
}

static pos_s _lex_file_position()
{
    return lex_process->pos;
}

token_s *token_create(token_s *_token)
{
    memcpy(&tmp_token, _token, sizeof(token_s));
    tmp_token.pos = _lex_file_position();
    if (_lex_is_in_expression())
    {
        tmp_token.between_brackets = buffer_ptr(lex_process->parentheses_buffer);
    }
    return &tmp_token;
}

token_s *lexer_last_token()
{
    return vector_back_or_null(lex_process->token_vec);
}

token_s *handle_whitespace()
{
    token_s *last_token = lexer_last_token();
    if (last_token != NULL)
    {
        last_token->whitespace = true;
    }

    nextc();
    return read_next_token();
}

const char *read_number_str()
{
    const char *num = NULL;
    buffer_s *buffer = buffer_create();
    char c = peekc();
    LEX_GETC_IF(buffer, c, (c >= '0' && c <= '9'))
    buffer_write(buffer, 0x00);
    return buffer_ptr(buffer);
}

unsigned long long read_number()
{
    const char *s = read_number_str();
    return atoll(s);
}

token_number_type_e lexer_number_type(char c)
{
    token_number_type_e res = NUMBER_TYPE_NORMAL;
    c = tolower(c);
    if (c == 'l')
    {
        res = NUMBER_TYPE_LONG;
    }
    else if (c == 'f')
    {
        res = NUMBER_TYPE_FLOAT;
    }
    return res;
}

token_s *token_make_number_for_value(unsigned long long number)
{
    token_number_type_e number_type = lexer_number_type((peekc()));
    if (number_type != NUMBER_TYPE_NORMAL)
    {
        nextc();
    }
    return token_create(&(token_s){.type = TOKEN_TYPE_NUMBER, .llnum = number, .num.type = number_type});
}

token_s *token_make_number()
{
    return token_make_number_for_value(read_number());
}

void lexer_pop_token()
{
    vector_pop(lex_process->token_vec);
}

bool is_hex_char(char c)
{
    c = tolower(c);
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
}

const char *read_hex_number_str()
{
    buffer_s *buf = buffer_create();
    char c = peekc();
    LEX_GETC_IF(buf, c, is_hex_char(c));
    // Write our null terminator
    buffer_write(buf, 0x00);
    return buffer_ptr(buf);
}

token_s *token_make_special_number_hexadecimal()
{
    // Skip the 'x'
    nextc();

    unsigned long number = 0;
    const char *number_str = read_hex_number_str();
    number = strtoll(number_str, 0, 16);
    return token_make_number_for_value(number);
}

void lexer_validate_binary_string(const char *str)
{
    size_t len = strlen(str);
    for (size_t i = 0; i < len; i++)
    {
        if (str[i] != '0' && str[i] != '1')
        {
            compile_error(lex_process->compiler, "This is not a valid binary number\n");
        }
    }
}

token_s *token_make_special_number_binary()
{
    // Skip the 'b'
    nextc();

    unsigned long number = 0;
    const char *number_str = read_number_str();
    lexer_validate_binary_string(number_str);
    number = strtoll(number_str, 0, 2);
    return token_make_number_for_value(number);
}

token_s *token_make_special_number()
{
    token_s *token = NULL;
    token_s *last_token = lexer_last_token();

    // Such as "x50", it's not a special number.
    if (NULL == last_token || !(last_token->type == TOKEN_TYPE_NUMBER && last_token->llnum == 0))
    {
        return token_make_identifier_or_keyword();
    }

    lexer_pop_token();

    char c = peekc();
    if (c == 'x')
    {
        token = token_make_special_number_hexadecimal();
    }
    else if (c == 'b')
    {
        token = token_make_special_number_binary();
    }

    return token;
}

token_s *token_make_string(char start_delim, char end_delim)
{
    buffer_s *buf = buffer_create();
    assert(nextc() == start_delim);
    char c = nextc();
    for (; c != end_delim && c != EOF; c = nextc())
    {
        if (c == '\\')
        {
            // We need to handle an escape character.
            continue;
        }

        buffer_write(buf, c);
    }

    buffer_write(buf, 0x00);
    return token_create(&(token_s){.type = TOKEN_TYPE_STRING, .sval = buffer_ptr(buf)});
}

static bool _op_treated_as_one(char op)
{
    return op == '(' ||
           op == '[' ||
           op == ',' ||
           op == '.' ||
           op == '*' ||
           op == '?';
}

static bool _is_single_operator(char op)
{
    return op == '+' ||
           op == '-' ||
           op == '/' ||
           op == '*' ||
           op == '=' ||
           op == '>' ||
           op == '<' ||
           op == '|' ||
           op == '&' ||
           op == '^' ||
           op == '%' ||
           op == '!' ||
           op == '(' ||
           op == '[' ||
           op == ',' ||
           op == '.' ||
           op == '~' ||
           op == '?';
}

bool op_valid(const char *op)
{
    return S_EQ(op, "+") ||
           S_EQ(op, "-") ||
           S_EQ(op, "*") ||
           S_EQ(op, "/") ||
           S_EQ(op, "!") ||
           S_EQ(op, "^") ||
           S_EQ(op, "+=") ||
           S_EQ(op, "-=") ||
           S_EQ(op, "*=") ||
           S_EQ(op, "/=") ||
           S_EQ(op, ">>") ||
           S_EQ(op, "<<") ||
           S_EQ(op, ">=") ||
           S_EQ(op, "<=") ||
           S_EQ(op, ">") ||
           S_EQ(op, "<") ||
           S_EQ(op, "||") ||
           S_EQ(op, "&&") ||
           S_EQ(op, "|") ||
           S_EQ(op, "&") ||
           S_EQ(op, "++") ||
           S_EQ(op, "--") ||
           S_EQ(op, "=") ||
           S_EQ(op, "!=") ||
           S_EQ(op, "==") ||
           S_EQ(op, "->") ||
           S_EQ(op, "(") ||
           S_EQ(op, "[") ||
           S_EQ(op, ",") ||
           S_EQ(op, ".") ||
           S_EQ(op, "...") ||
           S_EQ(op, "~") ||
           S_EQ(op, "?") ||
           S_EQ(op, "%");
}

void read_op_flush_back_but_keep_first(buffer_s *buf)
{
    const char *data = buffer_ptr(buf);
    int len = buf->len;
    for (int i = len - 1; i >= 1; i--)
    {
        if (data[i] == 0x00)
        {
            continue;
        }

        pushc(data[i]);
    }
}

const char *read_op()
{
    bool single_operator = true;
    char op = nextc();
    buffer_s *buf = buffer_create();
    buffer_write(buf, op);

    if (!_op_treated_as_one(op))
    {
        op = peekc();
        if (_is_single_operator(op))
        {
            buffer_write(buf, op);
            nextc();
            single_operator = false;
        }
    }

    // NULL terminator
    buffer_write(buf, 0x00);
    char *ptr = buffer_ptr(buf);
    if (!single_operator)
    {
        if (!op_valid(ptr))
        {
            read_op_flush_back_but_keep_first(buf);
            ptr[1] = 0x00;
        }
    }
    else if (!op_valid(ptr))
    {
        compile_error(lex_process->compiler, "The operator %s is not valid\n", ptr);
    }

    return ptr;
}

static void _lex_new_expression()
{
    lex_process->current_expression_count++;
    if (lex_process->current_expression_count == 1)
    {
        lex_process->parentheses_buffer = buffer_create();
    }
}

static void _lex_finish_expression()
{
    lex_process->current_expression_count--;
    if (lex_process->current_expression_count < 0)
    {
        compile_error(lex_process->compiler, "You closed an expression that you never opened\n");
    }
}

static bool _lex_is_in_expression()
{
    return lex_process->current_expression_count > 0;
}

token_s *token_make_operator_or_string()
{
    char op = peekc();
    if (op == '<')
    {
        token_s *last_token = lexer_last_token();
        if (token_is_keyword(last_token, "include"))
        {
            return token_make_string('<', '>');
        }
    }

    token_s *token = token_create(&(token_s){.type=TOKEN_TYPE_OPERATOR, .sval=read_op()});
    if (op == '(')
    {
        _lex_new_expression();
    }

    return token;
}

token_s *token_make_symbol()
{
    char c = nextc();
    if (c == ')')
    {
        _lex_finish_expression();
    }

    token_s *token = token_create(&(token_s){.type=TOKEN_TYPE_SYMBOL, .cval=c});
}

bool is_keyword(const char *str)
{
    return S_EQ(str, "unsigned") ||
        S_EQ(str, "signed") ||
        S_EQ(str, "char") ||
        S_EQ(str, "short") ||
        S_EQ(str, "int") ||
        S_EQ(str, "long") ||
        S_EQ(str, "float") ||
        S_EQ(str, "double") ||
        S_EQ(str, "void") ||
        S_EQ(str, "struct") ||
        S_EQ(str, "union") ||
        S_EQ(str, "static") ||
        S_EQ(str, "__ignore_typecheck") ||
        S_EQ(str, "return") ||
        S_EQ(str, "include") ||
        S_EQ(str, "sizeof") ||
        S_EQ(str, "if") ||
        S_EQ(str, "else") ||
        S_EQ(str, "while") ||
        S_EQ(str, "for") ||
        S_EQ(str, "do") ||
        S_EQ(str, "break") ||
        S_EQ(str, "continue") ||
        S_EQ(str, "switch") ||
        S_EQ(str, "case") ||
        S_EQ(str, "default") ||
        S_EQ(str, "goto") ||
        S_EQ(str, "typedef") ||
        S_EQ(str, "const") ||
        S_EQ(str, "extern") ||
        S_EQ(str, "restrict");
}

token_s *token_make_identifier_or_keyword()
{
    buffer_s *buf = buffer_create();
    char c = 0x00;
    LEX_GETC_IF(buf, c, (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || (c == '_'))

    // NULL terminator
    buffer_write(buf, 0x00);

    // Check if this is a keyword
    if (is_keyword(buffer_ptr(buf)))
    {
        return token_create(&(token_s){.type=TOKEN_TYPE_KEYWORD, .sval=buffer_ptr(buf)});
    }
    return token_create(&(token_s){.type=TOKEN_TYPE_IDENTIFIER, .sval=buffer_ptr(buf)});
}

token_s *read_special_token()
{
    char c = peekc();
    if (isalpha(c) || c == '_')
    {
        return token_make_identifier_or_keyword();
    }

    return NULL;
}

token_s *token_make_newline()
{
    nextc();
    return token_create(&(token_s){.type=TOKEN_TYPE_NEWLINE});
}

token_s *token_make_one_line_comment()
{
    buffer_s *buf = buffer_create();
    char c = 0x00;
    LEX_GETC_IF(buf, c, (c != '\n' && c != EOF))
    return token_create(&(token_s){.type=TOKEN_TYPE_COMMENT, .sval=buffer_ptr(buf)});
}

token_s *token_make_multi_line_comment()
{
    buffer_s *buf = buffer_create();
    char c = 0x00;
    while (1)
    {
        LEX_GETC_IF(buf, c, (c != '*' && c != EOF))
        if (c == EOF)
        {
            compile_error(lex_process->compiler, "You did not close this multi-line comment\n");
        }
        else if (c == '*')
        {
            nextc();
            if (peekc() == '/')
            {
                nextc();
                break;
            }
        }
    }
    return token_create(&(token_s){.type=TOKEN_TYPE_COMMENT, .sval=buffer_ptr(buf)});
}

token_s *handle_comment()
{
    char c = peekc();
    if (c == '/')
    {
        nextc();
        if (peekc() == '/')
        {
            nextc();
            return token_make_one_line_comment();
        }
        else if (peekc() == '*')
        {
            nextc();
            return token_make_multi_line_comment();
        }

        pushc('/');
        return token_make_operator_or_string();
    }

    return NULL;
}

char lex_get_escaped_char(char c)
{
    char co = 0x00;
    switch (c)
    {
    case 'n':
        co = '\n';
        break;
    case '\\':
        co = '\\';
        break;
    case 't':
        co = '\t';
        break;
    case '\'':
        co = '\'';
        break;
    default:
        compile_error(lex_process->compiler, "You input a wrong escaped char\n");
        break;
    }
    return co;
}

token_s *token_make_quote()
{
    assert_next_char('\'');
    char c = nextc();
    if (c == '\\')
    {
        c = nextc();
        c = lex_get_escaped_char(c);
    }

    if (nextc() != '\'')
    {
        compile_error(lex_process->compiler, "You opened a quote ', but did not close it with a ' character");
    }

    return token_create(&(token_s){.type=TOKEN_TYPE_NUMBER, .cval=c});
}

token_s *read_next_token()
{
    token_s *token = NULL;
    char c = peekc();

    token = handle_comment();
    if (token != NULL)
    {
        return token;
    }

    switch (c)
    {
    NUMERIC_CASE:
        token = token_make_number();
        break;

    OPERATOR_CASE_EXCLUDING_DIVISION:
        token = token_make_operator_or_string();
        break;

    SYMBOL_CASE:
        token = token_make_symbol();
        break;

    case 'b':
    case 'x':
        token = token_make_special_number();
        break;

    case '"':
        token = token_make_string('"', '"');
        break;

    case '\'':
        token = token_make_quote();
        break;

    case ' ':
    case '\t':
        // We don't care about whitespace, ignore them
        token = handle_whitespace();
        break;

    case '\n':
        token = token_make_newline();
        break;

    case EOF:
        // We have finished lexical analysis on the file
        break;

    default:
        token = read_special_token();
        if (NULL == token)
        {
            compile_error(lex_process->compiler, "Unexpecred token\n");
        }
    }
    return token;
}

int lex(lex_process_s *process)
{
    process->current_expression_count = 0;
    process->parentheses_buffer = NULL;
    lex_process = process;
    process->pos.filename = process->compiler->cfile.abs_path;

    token_s *token = read_next_token();
    while (token != NULL)
    {
        vector_push(process->token_vec, token);
        token = read_next_token();
    }

    return LEXICAL_ANALYSIS_ALL_OK;
}

char lexer_string_buffer_next_char(lex_process_s *process)
{
    buffer_s *buf = lex_process_private(process);
    return buffer_read(buf);
}

char lexer_string_buffer_peek_char(lex_process_s *process)
{
    buffer_s *buf = lex_process_private(process);
    return buffer_peek(buf);
}

void lexer_string_buffer_push_char(lex_process_s *process, char c)
{
    buffer_s *buf = lex_process_private(process);
    buffer_write(buf, c);
}

lex_process_functions_s lexer_string_buffer_functions =
{
    .next_char = lexer_string_buffer_next_char,
    .peek_char = lexer_string_buffer_peek_char,
    .push_char = lexer_string_buffer_push_char
};

lex_process_s *tokens_build_for_string(compile_process_s *compiler, const char *str)
{
    buffer_s *buf = buffer_create();
    buffer_printf(buf, str);
    lex_process_s *lex_process = lex_process_create(compiler, &lexer_string_buffer_functions, buf);
    if (NULL == lex_process)
    {
        return NULL;
    }

    if (lex(lex_process) != LEXICAL_ANALYSIS_ALL_OK)
    {
        return NULL;
    }

    return lex_process;
}
