constexpr auto c_whitespace_char = choice(' ', '\t', '\n', '\r', '\v', '\f');
constexpr auto c_identifier_begin_char = choice(range('a', 'z'), range('A', 'Z'), '_');
constexpr auto c_identifier_char = choice(range('a', 'z'), range('A', 'Z'), '_', range('0', '9'));
constexpr auto c_identifier = sequence(c_identifier_begin_char, zero_or_more(c_identifier_char));
template <class T> constexpr auto c_keyword(T t) {
	return sequence(t, not_(c_identifier_char));
}
template <class... T> constexpr auto c_keywords(T... arguments) {
	return choice(c_keyword(arguments)...);
}

constexpr auto c_comment = choice(
	sequence("/*", repetition(any_char_but("*/")), optional("*/")),
	sequence("//", repetition(any_char_but('\n')))
);
constexpr auto c_escape = sequence('\\', choice(
	'a', 'b', 't', 'n', 'v', 'f', 'r',
	'"', '\'', '?', '\\',
	repetition<1, 3>(range('0', '7')),
	sequence('x', one_or_more(hex_digit)),
	sequence('u', repetition<4, 4>(hex_digit)),
	sequence('U', repetition<8, 8>(hex_digit))
));
constexpr auto c_string = sequence(
	optional(choice('L', "u8", 'u', 'U')),
	'"',
	repetition(choice(highlight<Style::ESCAPE>(c_escape), any_char_but(choice('"', '\n')))),
	optional('"')
);
constexpr auto c_character = sequence(
	optional(choice('L', "u8", 'u', 'U')),
	'\'',
	repetition(choice(highlight<Style::ESCAPE>(c_escape), any_char_but(choice('\'', '\n')))),
	optional('\'')
);

constexpr auto c_digits = sequence(
	range('0', '9'),
	zero_or_more(sequence(optional('\''), range('0', '9')))
);
constexpr auto c_hex_digits = sequence(
	hex_digit,
	zero_or_more(sequence(optional('\''), hex_digit))
);
constexpr auto c_binary_digits = sequence(
	range('0', '1'),
	zero_or_more(sequence(optional('\''), range('0', '1')))
);
constexpr auto c_number = sequence(
	choice(
		// hexadecimal
		sequence(
			'0',
			choice('x', 'X'),
			choice(
				sequence(c_hex_digits, optional('.'), optional(c_hex_digits)),
				sequence('.', c_hex_digits)
			),
			// exponent
			optional(sequence(
				choice('p', 'P'),
				optional(choice('+', '-')),
				c_digits
			))
		),
		// binary
		sequence(
			'0',
			choice('b', 'B'),
			c_binary_digits
		),
		// decimal or octal
		sequence(
			choice(
				sequence(c_digits, optional('.'), optional(c_digits)),
				sequence('.', c_digits)
			),
			// exponent
			optional(sequence(
				choice('e', 'E'),
				optional(choice('+', '-')),
				c_digits
			))
		)
	),
	// suffix
	zero_or_more(choice('u', 'U', 'l', 'L', 'f', 'F'))
);
constexpr auto c_preprocessor = sequence(
	'#',
	zero_or_more(choice(' ', '\t')),
	choice(
		sequence(
			c_keyword("include"),
			zero_or_more(choice(' ', '\t')),
			optional(highlight<Style::STRING>(choice(
				sequence(
					'<',
					repetition(any_char_but(choice('>', '\n'))),
					optional('>')
				),
				sequence(
					'"',
					repetition(any_char_but(choice('"', '\n'))),
					optional('"')
				)
			)))
		),
		c_keyword("define"),
		c_keyword("undef"),
		c_keyword(sequence(optional("el"), "if", optional(sequence(optional('n'), "def")))),
		c_keyword("else"),
		c_keyword("endif"),
		c_keyword("error"),
		c_keyword("warning"),
		c_keyword("line"),
		c_keyword("pragma"),
		c_keyword("embed")
	)
);

struct c_file_name {
	static constexpr auto expression = ends_with(".c");
};

struct c_language {
	static constexpr auto expression = choice(
		// whitespace
		c_whitespace_char,
		// comments
		highlight<Style::COMMENT>(c_comment),
		// strings and characters
		highlight<Style::STRING>(c_string),
		highlight<Style::STRING>(c_character),
		// numbers
		highlight<Style::LITERAL>(c_number),
		// keywords
		highlight<Style::KEYWORD>(c_keywords(
			"if",
			"else",
			"for",
			"while",
			"do",
			"switch",
			"case",
			"default",
			"goto",
			"break",
			"continue",
			"return",
			"struct",
			"enum",
			"union",
			"typedef",
			"const",
			"static",
			"extern",
			"inline"
		)),
		// types
		highlight<Style::TYPE>(c_keywords(
			"void",
			"char",
			"short",
			"int",
			"long",
			"float",
			"double",
			"unsigned",
			"signed"
		)),
		// operators
		highlight<Style::OPERATOR>(c_keyword(
			"sizeof"
		)),
		choice(
			"+=", "-=", "*=", "/=", "%=", "&=", "|=", "^=", "<<=", ">>=",
			"++", "--",
			"&&", "||",
			"<<", ">>",
			"==", "!=", "<=", ">=",
			"->",
			'+', '-', '*', '/', '%',
			'&', '|', '^', '~',
			'<', '>',
			'=',
			'!',
			'?',
			':',
			'.'
		),
		// preprocessor
		highlight<Style::KEYWORD>(c_preprocessor),
		// identifiers
		c_identifier
	);
};
