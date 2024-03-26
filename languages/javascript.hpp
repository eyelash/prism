// https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Lexical_grammar

struct javascript_language;

constexpr auto javascript_escape = sequence('\\', choice(
	'b', 't', 'n', 'v', 'f', 'r',
	'"', '$', '\'', '\\', '`',
	'0',
	'\n',
	sequence('x', repetition<2, 2>(hex_digit)),
	sequence('u', repetition<4, 4>(hex_digit)),
	sequence("u{", one_or_more(hex_digit), '}')
));
constexpr auto javascript_template_string = sequence(
	'`',
	repetition(choice(
		highlight(Style::ESCAPE, javascript_escape),
		highlight(Style::DEFAULT, sequence(
			"${",
			repetition(sequence(not_('}'), choice(reference<javascript_language>(), any_char()))),
			optional('}')
		)),
		but('`')
	)),
	optional('`')
);
constexpr auto javascript_string = choice(
	sequence(
		'"',
		repetition(choice(highlight(Style::ESCAPE, javascript_escape), but(choice('"', '\n')))),
		optional('"')
	),
	sequence(
		'\'',
		repetition(choice(highlight(Style::ESCAPE, javascript_escape), but(choice('\'', '\n')))),
		optional('\'')
	),
	javascript_template_string
);
constexpr auto javascript_number = sequence(
	choice(
		// hexadecimal
		sequence(
			'0',
			choice('x', 'X'),
			one_or_more(hex_digit)
		),
		// binary
		sequence(
			'0',
			choice('b', 'B'),
			one_or_more(range('0', '1'))
		),
		// octal
		sequence(
			'0',
			choice('o', 'O'),
			one_or_more(range('0', '7'))
		),
		// decimal
		sequence(
			choice(
				sequence(one_or_more(range('0', '9')), optional('.'), zero_or_more(range('0', '9'))),
				sequence('.', one_or_more(range('0', '9')))
			),
			// exponent
			optional(sequence(
				choice('e', 'E'),
				optional(choice('+', '-')),
				one_or_more(range('0', '9'))
			))
		)
	),
	// suffix
	optional('n')
);

constexpr auto javascript_syntax = choice(
	// comments
	highlight(Style::COMMENT, c_comment),
	// strings
	highlight(Style::STRING, javascript_string),
	// numbers
	highlight(Style::LITERAL, javascript_number),
	// literals
	highlight(Style::LITERAL, java_keywords(
		"null",
		"false",
		"true"
	)),
	// keywords
	highlight(Style::KEYWORD, java_keywords(
		"function",
		"this",
		"var",
		"let",
		"const",
		"if",
		"else",
		"for",
		"in",
		"of",
		"while",
		"do",
		"switch",
		"case",
		"default",
		"break",
		"continue",
		"try",
		"catch",
		"finally",
		"throw",
		"return",
		"new",
		"class",
		"extends",
		"static",
		"import",
		"export"
	)),
	sequence(
		'{',
		repetition(sequence(not_('}'), choice(reference<javascript_language>(), any_char()))),
		optional('}')
	),
	// identifiers
	java_identifier
);

struct javascript_file_name {
	static constexpr auto expression = ends_with(".js");
};

struct javascript_language {
	static constexpr auto expression = javascript_syntax;
};
