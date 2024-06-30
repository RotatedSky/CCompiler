#include "compiler.h"

bool token_is_keyword(token_s *token, const char *value)
{
    return token->type == TOKEN_TYPE_KEYWORD && S_EQ(token->sval, value);
}
