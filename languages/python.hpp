// https://docs.python.org/3/reference/lexical_analysis.html

constexpr auto python_comment = sequence('#', repetition(any_char_but('\n')));
constexpr auto python_escape = sequence('\\', any_char());
constexpr auto python_string = choice(
	sequence(
		"\"\"\"",
		repetition(choice(highlight(Style::ESCAPE, python_escape), any_char_but("\"\"\""))),
		optional("\"\"\"")
	),
	sequence(
		"'''",
		repetition(choice(highlight(Style::ESCAPE, python_escape), any_char_but("'''"))),
		optional("'''")
	),
	sequence(
		'"',
		repetition(choice(highlight(Style::ESCAPE, python_escape), any_char_but(choice('"', '\n')))),
		optional('"')
	),
	sequence(
		'\'',
		repetition(choice(highlight(Style::ESCAPE, python_escape), any_char_but(choice('\'', '\n')))),
		optional('\'')
	)
);
constexpr auto python_digits = sequence(
	range('0', '9'),
	zero_or_more(sequence(optional('_'), range('0', '9')))
);
constexpr auto python_number = choice(
	// hexadecimal
	sequence(
		'0',
		choice('x', 'X'),
		one_or_more(sequence(optional('_'), hex_digit))
	),
	// binary
	sequence(
		'0',
		choice('b', 'B'),
		one_or_more(sequence(optional('_'), range('0', '1')))
	),
	// octal
	sequence(
		'0',
		choice('o', 'O'),
		one_or_more(sequence(optional('_'), range('0', '7')))
	),
	// decimal
	sequence(
		choice(
			sequence(python_digits, optional('.'), optional(python_digits)),
			sequence('.', python_digits)
		),
		// exponent
		optional(sequence(
			choice('e', 'E'),
			optional(choice('+', '-')),
			python_digits
		))
	)
);

struct python_file_name {
	static constexpr auto expression = ends_with(".py");
};

struct python_language {
	static constexpr auto expression = choice(
		// whitespace
		c_whitespace_char,
		// comments
		highlight(Style::COMMENT, python_comment),
		// strings
		highlight(Style::STRING, python_string),
		// numbers
		highlight(Style::LITERAL, python_number),
		// literals
		highlight(Style::LITERAL, c_keywords(
			"None",
			"False",
			"True"
		)),
		// keywords
		sequence(
			highlight(Style::KEYWORD, c_keyword("def")),
			zero_or_more(' '),
			optional(highlight(Style::FUNCTION, c_identifier))
		),
		sequence(
			highlight(Style::KEYWORD, c_keyword("class")),
			zero_or_more(' '),
			optional(highlight(Style::TYPE, c_identifier))
		),
		highlight(Style::KEYWORD, c_keywords(
			"lambda",
			"if",
			"elif",
			"else",
			"for",
			"in",
			"while",
			"break",
			"continue",
			"return",
			"import"
		)),
		// operators
		highlight(Style::OPERATOR, c_keywords(
			"and",
			"or",
			"not",
			"is",
			"in"
		))
	);
};
