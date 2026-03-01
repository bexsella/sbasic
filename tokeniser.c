/**
 * sbasic -- A small embedable BASIC interpreter
 * 
 * TODO:
 *  - Parser
 *  - String library
 *  - Module system
 *  - Full syntax
 *  - Big Numbers
 *  - Interpreter
 *  - IO Module
 *  - Maths Module
 */
#define _RUN_TEST_ 1
#define SB_BASIC_IMPLEMENTATION

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// absolute trash:
#ifdef _MSC_VER
#define strncasecmp _strnicmp
#endif

typedef struct {
	uint8_t sign : 1;
	size_t num_digits;
	uint32_t* digits;
} sb_int_t;

sb_int_t sb_int_new(const size_t len);

enum token_types_t {
	TOK_UNKNOWN,
	TOK_INVALID,

	TOK_IDENTIFIER,

	// Key words:
	TOK_PROCEDURE,
	TOK_ENDPROCEDURE,
	TOK_PROCEDURE_RETURN,
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
	TOK_ROTATE_LEFT,
	TOK_ROTATE_RIGHT,
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
	TOK_DOLLAR,

	TOK_OPEN_PAREN,
	TOK_CLOSE_PAREN,
	TOK_COMMA,

	// Types:
	TOK_STRING,
	TOK_NUMBER,

	TOK_COMMENT,
	TOK_EOF,

	TOK_COUNT
};

const char* token_type_strings[TOK_COUNT] = {
	"Unknown Token",
	"Invalid Token",

	"Identifier",

	// Key words:
	"Procedure",
	"EndProcedure",
	"ProcedureReturn"
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
	"Rotl",
	"Rotr",
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
	"$",

	"Open Paren",
	"Close Paren",
	"Comma",

	"String",
	"Number",
	
	"Comment",

	"End of File",
};

typedef struct {
	size_t offset;
	size_t row;
	size_t column;
} text_position_t;

enum tokeniser_error_t {
	TOK_ERR_NONE = 0,
	TOK_ERR_EOF, // not strictly an error
	TOK_ERR_INVALID_TOKEN,
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
} tokeniser_t;

tokeniser_t sb_tokeniser_new(char* data);
int sb_tokeniser_next_token(tokeniser_t* t, token_t* tok);

enum expression_types_t {
	EXPR_GROUP,
	EXPR_BINARY,
	EXPR_LITERAL,
	EXPR_UNARY,
};

typedef struct expression_t expression_t;

typedef struct {
	token_t operator;
	expression_t* right;
} unary_expression_t;

typedef struct {
	int type;
	union {
		const char* string_value;
		int integer_value;
		float float_value;
	};
} literal_expression_t;

typedef struct {
	expression_t* expr;
} group_expression_t;

typedef struct {
	expression_t* left;
	token_t operator;
	expression_t* right;
} binary_expression_t;

struct expression_t {
	int type;
	union {
		group_expression_t group;
		binary_expression_t binary;
		literal_expression_t literal;
		unary_expression_t unary;
	};
};

typedef struct ast_node_t {
	token_t token;

	int type;

	union {
		expression_t expr;
	};

	struct ast_node_t* left;
	struct ast_node_t* right;
} ast_node_t;

typedef struct {
	const char* file_path;
	tokeniser_t tokeniser;
	ast_node_t* ast_root;
	token_t last_token;
} parser_t;

/**
 * Modules
 * 
 * User-defined modules allow for extending the base sbasic language to
 * cater the interpreter to any application a user may need, e.g. interfacing with
 * hardware or specific maths applications.
 * 
 * For examples, see the maths and io modules that are defined witihn the implementation.
 */

enum mod_func_arg_type_t {
	MFAT_NONE,
	MFAT_NUMBER,
	MFAT_STRING,
};

typedef struct {
	const char* name;
	int arg_type;
	union {
		union {
			int integer;
			float f32;
		};
		const char* string;
	};
} mod_proc_arg_t;

typedef struct {
	const char* name;
	int return_type;
	size_t num_args;
	union {
		int (*i_proc)(mod_proc_arg_t*);
		float (*f_proc)(mod_proc_arg_t*);
		void (*v_proc)(mod_proc_arg_t*);
		void* (*vp_proc)(mod_proc_arg_t*);
	};
} mod_proc_t;

#ifdef __cplusplus
} // extern "C"
#endif

#ifdef SB_BASIC_IMPLEMENTATION

#if defined(SB_BASIC_ALLOC) && defined(SB_BASIC_FREE) && defined(SB_BASIC_REALLOC)
#elif !defined(SB_BASIC_ALLOC) && !defined(SB_BASIC_FREE) && !defined(SB_BASIC_REALLOC)
#else
#error MUST DEFINED ALL OR NONE OF THE MEMORY ALLOCATION FUNCTIONS
#endif

#if !defined(SB_BASIC_ALLOC) || !defined(SB_BASIC_FREE) || !defined(SB_BASIC_REALLOC)
#define SB_BASIC_ALLOC(s) malloc(s)
#define SB_BASIC_FREE(ptr) free(ptr)
#define SB_BASIC_REALLOC(ptr, s) realloc(ptr, s)
#endif

/**
 * Big Number Support
 * 
 * Currently a place holder api that does notihng. But reading up on Big Num implementations there
 * seems to be two ways of doing it:
 * 
 *		1) Have every digit be an individual number between 0 and 9 
 *			the way to from string procedure is currently setup; or
 * 
 *		2) Have every digit represent the bits of the overall number.
 * 
 */

sb_int_t sb_int_new(const size_t len)
{
	/// alloc digits:
	uint32_t* digits = SB_BASIC_ALLOC(len * sizeof(*digits));

	if (!digits)
	{
		return (sb_int_t) {
			.digits = NULL,
			.sign = 0,
			.num_digits = 0
		};	
	}

	return (sb_int_t) {
		.sign = 1,
		.num_digits = len,
		.digits = digits
	};
}

sb_int_t sb_int_from_string(const char* src, const size_t len)
{
	const char* digit_src = src[0] == '-' ? src + 1 : src;
	size_t num_digits = src[0] == '-' ? len - 1 : len;

	// trim leading zeros
	while (*digit_src == '0')
	{
		num_digits--;
		digit_src++;
	}

	uint32_t* digits = SB_BASIC_ALLOC(num_digits * sizeof(*digits));

	if (!digits)
	{
		return (sb_int_t) {
			.sign = 0,
			.num_digits = 0,
			.digits = NULL
		};
	}

	for (size_t i = 0; i < num_digits; i++)
	{
		digits[i] = digit_src[i] - '0';
	}

	return (sb_int_t) {
		.sign = src[0] == '-',
		.num_digits = num_digits,
		.digits = digits
	};
}

sb_int_t sb_int_add(const sb_int_t* a, const sb_int_t* b)
{


	return (sb_int_t) {
		.sign = 0,
		.num_digits = 0,
		.digits = NULL
	};
}

sb_int_t sb_int_sub(const sb_int_t* a, const sb_int_t* b)
{
	return (sb_int_t) {
		.sign = 0,
		.num_digits = 0,
		.digits = NULL
	};
}

sb_int_t sb_int_mul(const sb_int_t* a, const sb_int_t* b)
{
	return (sb_int_t) {
		.sign = 0,
		.num_digits = 0,
		.digits = NULL
	};
}

sb_int_t sb_int_div(const sb_int_t* a, const sb_int_t* b)
{
	return (sb_int_t) {
		.sign = 0,
		.num_digits = 0,
		.digits = NULL
	};
}

sb_int_t sb_int_pow(const sb_int_t* a, const sb_int_t* b)
{
	return (sb_int_t) {
		.sign = 0,
		.num_digits = 0,
		.digits = NULL
	};
}

sb_int_t sb_int_shl(const sb_int_t* a, const sb_int_t* b)
{
	return (sb_int_t) {
		.sign = 0,
		.num_digits = 0,
		.digits = NULL
	};
}

sb_int_t sb_int_shr(const sb_int_t* a, const sb_int_t* b)
{
	return (sb_int_t) {
		.sign = 0,
		.num_digits = 0,
		.digits = NULL
	};
}

sb_int_t sb_int_rotl(const sb_int_t* a, const sb_int_t* b)
{
	return (sb_int_t) {
		.sign = 0,
		.num_digits = 0,
		.digits = NULL
	};
}

sb_int_t sb_int_rotr(const sb_int_t* a, const sb_int_t* b)
{
	return (sb_int_t) {
		.sign = 0,
		.num_digits = 0,
		.digits = NULL
	};
}

char sb_tokeniser_next_char(tokeniser_t* t);
char sb_tokeniser_peek_char(tokeniser_t* t);

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
	while ((ch >= '0' && ch <= '9') || (ch >= 'A' && ch <= 'z') || ch == '_')
	{
		ch = sb_tokeniser_next_char(t);
	}
}

inline void sb_tokeniser_skip_string(tokeniser_t* t)
{
	sb_tokeniser_next_char(t);
	while (t->current_char != '"')
	{
		sb_tokeniser_next_char(t);

		if (t->current_char == '"' && t->last_char == '\\')
		{
			sb_tokeniser_next_char(t);
		}
	}
}

void sb_tokensier_check_keyword(tokeniser_t* t, token_t* tok)
{
	for (int i = TOK_PROCEDURE; i <= TOK_XOR; i++)
	{
		// check length first as it's the cheaper of the two operations:
		if (strlen(token_type_strings[i]) == tok->data_len)
		{
			if (!strncasecmp(tok->data, token_type_strings[i], tok->data_len))
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
		else if (ch == '$')
		{
			tok->type = TOK_DOLLAR;
			sb_tokeniser_next_char(t);
		}
		else if (ch == '\n')
		{
			sb_tokeniser_next_char(t);
		}
		else
		{
			tok->type = TOK_INVALID;
			sb_tokeniser_next_char(t);
		}
	}

	tok->data = t->data + tok->pos.offset;
	tok->data_len = t->pos.offset - tok->pos.offset;

	switch (tok->type)
	{
	case TOK_IDENTIFIER:
		// Determine if the identifer is a keyword, or just an identifier:
		sb_tokensier_check_keyword(t, tok);
		break;

	case TOK_INVALID:
		// Report an invalid token has been found:
		return TOK_ERR_INVALID_TOKEN;

	default:
		break;
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

void sb_parser_print_expr(const expression_t* expr)
{
	switch (expr->type)
	{
	case EXPR_BINARY:

	case EXPR_GROUP:
	case EXPR_UNARY:
	case EXPR_LITERAL:
	default:
		fprintf(stderr, "Invalid expression.");
		break;
	}
}

parser_t sb_parser_new(const char* file_path)
{

	return (parser_t) {
		.file_path = file_path,
		.tokeniser = sb_tokeniser_new(""),
	};
}

#if _RUN_TEST_

int main(int argc, char* argv[])
{
	char* TEST_PROGRAM = {
	"Procedure Main()\n"
	"    D = 1234**56/7\n"
	"    If D>100 then\n"
	"        Print \"This is a \\\"big\\\" number!\\n\"\n"
	"    Endif\n\n"
	"    For XorVal=1 In 10 Step 2\n"
	"        Print \"Hello, world!\\n\" + D ;  can i have a comment here?\n"
	"    Next##\n"
	"EndProcedure"
	};

	printf("%s\n\n", TEST_PROGRAM);

	tokeniser_t tokeniser = sb_tokeniser_new(TEST_PROGRAM);
	token_t tok;

	while (sb_tokeniser_next_token(&tokeniser, &tok) == TOK_ERR_NONE)
	{
		printf("%s\n", token_type_strings[tok.type]);
	}

	if (tok.type == TOK_INVALID)
	{
		printf("Unknown/Invalid token at %zu:%zu\n", tok.pos.row, tok.pos.column);
		return -1;
	}

	sb_parser_print_expr(&(expression_t) {
		.type = EXPR_BINARY,
		.binary =
		{
			.left = &(expression_t) { .type = EXPR_UNARY, .unary = { .operator = {.data = "-", .data_len = 1, .type = TOK_SUBTRACT }, .right = &(expression_t) { .type = EXPR_LITERAL, .literal = {.integer_value = 123 } } } },
			.operator = {.data = "*", .data_len = 1, .type = TOK_MULTIPLY },
			.right = &(expression_t) { .type = EXPR_GROUP, .group = {.expr = &(expression_t) { .type = EXPR_LITERAL, .literal = {.type = TOK_NUMBER, .float_value = 45 } } } }
		}
	});

	return 0;
}
#endif

#endif // SB_BASIC_IMPLEMENTATION
