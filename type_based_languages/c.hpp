using c_whitespace_char = choice(C(' '), C('\t'), C('\n'), C('\r'), C('\v'), C('\f'));
using c_identifier_begin_char = choice(range('a', 'z'), range('A', 'Z'), C('_'));
using c_identifier_char = choice(range('a', 'z'), range('A', 'Z'), C('_'), range('0', '9'));
using c_identifier = seq(c_identifier_begin_char, zero_or_more(c_identifier_char));
template <class T> using CKeyword = seq(T, not_(c_identifier_char));
#define c_keyword(...) CKeyword<__VA_ARGS__>
template <class... T> using CKeywords = choice(c_keyword(T)...);
#define c_keywords(...) CKeywords<__VA_ARGS__>

DEFINE_PARSER(c_comment, choice(
	seq(S("/*"), repeat(but(S("*/"))), optional(S("*/"))),
	seq(S("//"), repeat(but(C('\n'))))
))
DEFINE_PARSER(c_escape, seq(C('\\'), choice(
	C('a'), C('b'), C('t'), C('n'), C('v'), C('f'), C('r'),
	C('"'), C('\''), C('?'), C('\\'),
	one_or_more(range('0', '7')),
	seq(C('x'), one_or_more(hex_digit)),
	seq(C('u'), one_or_more(hex_digit)),
	seq(C('U'), one_or_more(hex_digit))
)))
DEFINE_PARSER(c_string, seq(
	optional(choice(C('L'), S("u8"), C('u'), C('U'))),
	C('"'),
	repeat(choice(highlight(Style::ESCAPE, c_escape), but(choice(C('"'), C('\n'))))),
	optional(C('"'))
))
DEFINE_PARSER(c_character, seq(
	optional(choice(C('L'), S("u8"), C('u'), C('U'))),
	C('\''),
	repeat(choice(highlight(Style::ESCAPE, c_escape), but(choice(C('\''), C('\n'))))),
	optional(C('\''))
))
DEFINE_PARSER(c_digits, seq(
	range('0', '9'),
	repeat(seq(optional(C('\'')), range('0', '9')))
))
DEFINE_PARSER(c_hex_digits, seq(
	hex_digit,
	repeat(seq(optional(C('\'')), hex_digit))
))
DEFINE_PARSER(c_binary_digits, seq(
	range('0', '1'),
	repeat(seq(optional(C('\'')), range('0', '1')))
))
DEFINE_PARSER(c_number, seq(
	choice(
		// hexadecimal
		seq(
			C('0'),
			choice(C('x'), C('X')),
			choice(
				seq(c_hex_digits, optional(C('.')), optional(c_hex_digits)),
				seq(C('.'), c_hex_digits)
			),
			// exponent
			optional(seq(
				choice(C('p'), C('P')),
				optional(choice(C('+'), C('-'))),
				c_digits
			))
		),
		// binary
		seq(
			C('0'),
			choice(C('b'), C('B')),
			c_binary_digits
		),
		// decimal or octal
		seq(
			choice(
				seq(c_digits, optional(C('.')), optional(c_digits)),
				seq(C('.'), c_digits)
			),
			// exponent
			optional(seq(
				choice(C('e'), C('E')),
				optional(choice(C('+'), C('-'))),
				c_digits
			))
		)
	),
	// suffix
	zero_or_more(choice(C('u'), C('U'), C('l'), C('L'), C('f'), C('F')))
))
DEFINE_PARSER(c_preprocessor, seq(
	C('#'),
	zero_or_more(choice(C(' '), C('\t'))),
	choice(
		seq(
			c_keyword(S("include")),
			zero_or_more(choice(C(' '), C('\t'))),
			optional(highlight(Style::STRING, choice(
				seq(
					C('<'),
					repeat(but(choice(C('>'), C('\n')))),
					optional(C('>'))
				),
				seq(
					C('"'),
					repeat(but(choice(C('"'), C('\n')))),
					optional(C('"'))
				)
			)))
		),
		c_keyword(S("define")),
		c_keyword(S("undef")),
		c_keyword(seq(optional(S("el")), S("if"), optional(seq(optional(C('n')), S("def"))))),
		c_keyword(S("else")),
		c_keyword(S("endif")),
		c_keyword(S("error")),
		c_keyword(S("warning")),
		c_keyword(S("line")),
		c_keyword(S("pragma")),
		c_keyword(S("embed"))
	)
))

DEFINE_PARSER(c_file_name, ends_with(S(".c")))

DEFINE_PARSER(c_language,
	choice(
		// whitespace
		c_whitespace_char,
		// comments
		highlight(Style::COMMENT, c_comment),
		// strings and characters
		highlight(Style::STRING, c_string),
		highlight(Style::STRING, c_character),
		// numbers
		highlight(Style::LITERAL, c_number),
		// keywords
		highlight(Style::KEYWORD, c_keywords(
			S("if"),
			S("else"),
			S("for"),
			S("while"),
			S("do"),
			S("switch"),
			S("case"),
			S("default"),
			S("goto"),
			S("break"),
			S("continue"),
			S("return"),
			S("struct"),
			S("enum"),
			S("union"),
			S("typedef"),
			S("const"),
			S("static"),
			S("extern"),
			S("inline")
		)),
		// types
		highlight(Style::TYPE, c_keywords(
			S("void"),
			S("char"),
			S("short"),
			S("int"),
			S("long"),
			S("float"),
			S("double"),
			S("unsigned"),
			S("signed")
		)),
		// operators
		highlight(Style::OPERATOR, c_keyword(
			S("sizeof")
		)),
		choice(
			S("+="), S("-="), S("*="), S("/="), S("%="), S("&="), S("|="), S("^="), S("<<="), S(">>="),
			S("++"), S("--"),
			S("&&"), S("||"),
			S("<<"), S(">>"),
			S("=="), S("!="), S("<="), S(">="),
			S("->"),
			C('+'), C('-'), C('*'), C('/'), C('%'),
			C('&'), C('|'), C('^'), C('~'),
			C('<'), C('>'),
			C('='),
			C('!'),
			C('?'),
			C(':'),
			C('.')
		),
		// preprocessor
		highlight(Style::KEYWORD, c_preprocessor),
		// identifiers
		c_identifier
	)
)
