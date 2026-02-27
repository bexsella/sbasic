/**
 * sbasic -- A small embedable BASIC interpretter
 * 
 * TODO:
 *  - Parser
 *  - String library
 *  - Module system
 *  - Full syntax
 *  - Big Numbers
 *  - Interpretter
 */
#define _RUN_TEST_ 1

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

// absolute trash:
#ifdef _MSC_VER
#define strncasecmp _strnicmp
#endif

#ifdef __cplusplus
extern "C" {
#endif

enum token_types_t {
	TOK_UNKNOWN,
	TOK_INVALID,

	TOK_IDENTIFIER,

	// Key words:
	TOK_PROCEDURE,
	TOK_ENDPROCEDURE,
	TOK_IF,
	TOK_ELSEIF,
	TOK_ELSE,
	TOK_ENDIF,
	TOK_THEN,
	TOK_FOR,
	TOK_IN,
	TOK_NEXT,
	TOK_STEP,
	TOK_NOT,
	TOK_AND,
	TOK_OR,
	TOK_TRUE,
	TOK_FALSE,
	TOK_MOD,
	TOK_XOR,

	// Operators:
	TOK_MULTIPLY,
	TOK_DIVIDE,
	TOK_ADD,
	TOK_SUBTRACT,
	TOK_POWER,
	TOK_EQUALS,
	TOK_GREATER,
	TOK_LESS,
	TOK_NOT_EQUAL,
	TOK_GREATER_OR_EQUAL,
	TOK_LESS_OR_EQUAL,
	TOK_SHIFT_LEFT,
	TOK_SHIFT_RIGHT,
	TOK_BIT_OR,
	TOK_BIT_AND,
	TOK_BIT_XOR,
	TOK_BIT_NOT,

	TOK_OPEN_PAREN,
	TOK_CLOSE_PAREN,
	TOK_COMMA,

	// Types:
	TOK_STRING,
	TOK_NUMBER,

	TOK_EOL,
	TOK_EOF,
	TOK_COMMENT,

	TOK_COUNT
};

const char* token_type_strings[TOK_COUNT] = {
	"Unknown Token",
	"Invalid Token",
	"Identifier",

	// Key words:
	"Procedure",
	"EndProcedure",
	"If",
	"ElseIf",
	"Else",
	"EndIf",
	"Then",
	"For",
	"In",
	"Next",
	"Step",
	"Not",
	"And",
	"Or",
	"True",
	"False",
	"Mod",
	"Xor",

	// Operators:
	"*",
	"/",
	"+",
	"-",
	"**",
	"=",
	">",
	"<",
	"<>",
	">=",
	"<=",
	"<<",
	">>",
	"|",
	"&",
	"^",
	"!",

	"Open Paren",
	"Close Paren",
	"Comma",

	"String",
	"Number",
	"End of Line",
	"End of File",
	"Comment",
};

typedef struct {
	size_t offset;
	size_t row;
	size_t column;
} text_position_t;

enum tokeniser_error_t {
	TOK_ERR_NONE = 0,
	TOK_ERR_EOF,
};

typedef struct {
	text_position_t pos;
	int type;
	char* data;
	size_t data_len;
} token_t;

typedef struct {
	text_position_t pos;
	char* data;
	char current_char;
	char last_char;
	token_t last_token;
} tokeniser_t;

typedef struct {
	tokeniser_t tokeniser;
} parser_t;

tokeniser_t sb_tokeniser_new(char* data);
char sb_tokeniser_next_char(tokeniser_t* t);
char sb_tokeniser_peek_char(tokeniser_t* t);

#ifdef __cplusplus
} // extern "C"
#endif

inline bool sb_tokeniser_is_whitespace(const char ch)
{
	return ch == ' ' || ch == '\t' || ch == '\v' || ch == '\f' || ch == '\r';
}

inline void sb_tokeniser_skip_whitespace(tokeniser_t* t)
{
	char ch = t->current_char;
	while (sb_tokeniser_is_whitespace(ch))
	{
		ch = sb_tokeniser_next_char(t);
	}
}

inline void sb_tokeniser_skip_to_eol(tokeniser_t* t)
{
	const size_t current_line = t->pos.row;
	while (current_line == t->pos.row)
	{
		sb_tokeniser_next_char(t);
	}
}

inline void sb_tokeniser_skip_numeric(tokeniser_t* t)
{
	char ch = t->current_char;
	while ((ch >= '0' && ch <= '9') || ch == '.')
	{
		ch = sb_tokeniser_next_char(t);
	}
}

inline void sb_tokeniser_skip_alphanumeric(tokeniser_t* t)
{
	char ch = t->current_char;
	while ((ch >= '0' && ch <= '9') || (ch >= 'A' && ch <= 'z'))
	{
		ch = sb_tokeniser_next_char(t);
	}
}

inline void sb_tokeniser_skip_string(tokeniser_t* t)
{
	char ch = sb_tokeniser_next_char(t);
	while (ch != '"')
	{
		ch = sb_tokeniser_next_char(t);
	}
}

void sb_tokensier_check_keyword(tokeniser_t* t, token_t* tok)
{
	for (int i = TOK_PROCEDURE; i <= TOK_XOR; i++)
	{
		// TODO: Write a proper string handling library, and we can avoid this nonsense. (ODIN IS CALLING)
		if (!strncasecmp(tok->data, token_type_strings[i], strlen(token_type_strings[i])))
		{
			if (strlen(token_type_strings[i]) == tok->data_len)
			{
				tok->type = i;
				return;
			}
		}
	}
}

int sb_tokeniser_next_token(tokeniser_t* t, token_t* tok)
{
	tok->type = TOK_UNKNOWN;

	while (tok->type == TOK_UNKNOWN)
	{
		if (sb_tokeniser_is_whitespace(t->current_char))
		{
			sb_tokeniser_skip_whitespace(t);
		}

		char ch = t->current_char;
		tok->pos = t->pos;

		if (ch == '\0')
		{
			tok->data = NULL;
			tok->data_len = 0;
			tok->type = TOK_EOF;

			return TOK_ERR_EOF;
		}
		else if (ch >= '0' && ch <= '9')
		{
			sb_tokeniser_skip_numeric(t);
			tok->type = TOK_NUMBER;
		}
		else if (ch >= 'A' && ch <= 'z')
		{
			sb_tokeniser_skip_alphanumeric(t);
			tok->type = TOK_IDENTIFIER;
		}
		else if (ch == ';')
		{
			sb_tokeniser_skip_to_eol(t);
			tok->type = TOK_COMMENT;
		}
		else if (ch == '(')
		{
			tok->type = TOK_OPEN_PAREN;
			sb_tokeniser_next_char(t);
		}
		else if (ch == ')')
		{
			tok->type = TOK_CLOSE_PAREN;
			sb_tokeniser_next_char(t);
		}
		else if (ch == ',')
		{
			tok->type = TOK_COMMA;
			sb_tokeniser_next_char(t);
		}
		else if (ch == '"')
		{
			sb_tokeniser_skip_string(t);

			tok->type = TOK_STRING;

			sb_tokeniser_next_char(t);
		}
		else if (ch == '*')
		{
			if (sb_tokeniser_peek_char(t) == '*')
			{
				sb_tokeniser_next_char(t);
				tok->type = TOK_POWER;
			}
			else
			{
				tok->type = TOK_MULTIPLY;
			}

			sb_tokeniser_next_char(t);
		}
		else if (ch == '/')
		{
			tok->type = TOK_DIVIDE;
			sb_tokeniser_next_char(t);
		}
		else if (ch == '+')
		{
			tok->type = TOK_ADD;
			sb_tokeniser_next_char(t);
		}
		else if (ch == '-')
		{
			tok->type = TOK_SUBTRACT;
			sb_tokeniser_next_char(t);
		}
		else if (ch == '=')
		{
			tok->type = TOK_EQUALS;
			sb_tokeniser_next_char(t);
		}
		else if (ch == '>')
		{
			if (sb_tokeniser_peek_char(t) == '=')
			{
				tok->type = TOK_GREATER_OR_EQUAL;
				sb_tokeniser_next_char(t);
			}
			else if (sb_tokeniser_peek_char(t) == '>')
			{
				tok->type = TOK_SHIFT_RIGHT;
				sb_tokeniser_next_char(t);
			}
			else
			{
				tok->type = TOK_GREATER;
			}

			sb_tokeniser_next_char(t);
		}
		else if (ch == '<')
		{
			if (sb_tokeniser_peek_char(t) == '>')
			{
				tok->type = TOK_NOT_EQUAL;
				sb_tokeniser_next_char(t);
			}
			else if (sb_tokeniser_peek_char(t) == '=')
			{
				tok->type = TOK_LESS_OR_EQUAL;
				sb_tokeniser_next_char(t);
			}
			else if (sb_tokeniser_peek_char(t) == '<')
			{
				tok->type = TOK_SHIFT_LEFT;
				sb_tokeniser_next_char(t);
			}
			else
			{
				tok->type = TOK_LESS;
			}

			sb_tokeniser_next_char(t);
		}
		else if (ch == '|')
		{
			tok->type = TOK_BIT_OR;
			sb_tokeniser_next_char(t);
		}
		else if (ch == '&')
		{
			tok->type = TOK_BIT_AND;
			sb_tokeniser_next_char(t);
		}
		else if (ch == '^')
		{
			tok->type = TOK_BIT_XOR;
			sb_tokeniser_next_char(t);
		}
		else if (ch == '!')
		{
			tok->type = TOK_BIT_NOT;
			sb_tokeniser_next_char(t);
		}
		else if (ch == '\n')
		{
			sb_tokeniser_next_char(t);
		}
	}

	tok->data = t->data + tok->pos.offset;
	tok->data_len = t->pos.offset - tok->pos.offset;

	if (tok->type == TOK_IDENTIFIER)
	{
		// Determine if the identifer is a keyword, or just an identifier:
		sb_tokensier_check_keyword(t, tok);
	}

	return TOK_ERR_NONE;
}

int sb_tokeniser_expect(tokeniser_t* t, const char* tok)
{
	return TOK_ERR_NONE;
}

char sb_tokeniser_next_char(tokeniser_t* t)
{
	t->pos.offset++;
	t->pos.column++;
	t->last_char = t->current_char;
	t->current_char = *(t->data + t->pos.offset);

	if (t->current_char == '\n')
	{
		t->pos.row++;
		t->pos.column = 0;
	}

	return t->current_char;
}

char sb_tokeniser_peek_char(tokeniser_t* t)
{
	char ch = *(t->data + t->pos.offset + 1);
	return ch;
}

tokeniser_t sb_tokeniser_new(char* data)
{
	return (tokeniser_t) {
		.data = data,
		.pos = {
			.offset = 0,
			.row = 0,
			.column = 0
		},
		.current_char = data[0]
	};
}

#if _RUN_TEST_
#include <stdio.h>

int main(int argc, char* argv[])
{
	char* TEST_PROGRAM = {
	"Procedure Main()\n"
	"    D = 1234**56/7\n"
	"    If D>100 then\n"
	"        Print \"This is a big number!\\n\"\n"
	"    Endif\n\n"
	"    For XorVal=1 In 10 Step 2\n"
	"        Print \"Hello, world!\\n\" + D ;  can i have a comment here?\n"
	"    Next\n"
	"EndProcedure"
	};

	printf("%s\n\n", TEST_PROGRAM);

	tokeniser_t tokeniser = sb_tokeniser_new(TEST_PROGRAM);
	token_t tok;

	while (sb_tokeniser_next_token(&tokeniser, &tok) == TOK_ERR_NONE)
	{
		if (tok.type == TOK_UNKNOWN || tok.type == TOK_INVALID)
		{
			printf("Unknown/Invalid token at %zu:%zu\n", tok.pos.row, tok.pos.column);
			break;
		}
		printf("%s\n", token_type_strings[tok.type]);
	}

	return 0;
}
#endif
