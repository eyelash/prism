// https://docs.oracle.com/javase/specs/jls/se17/html/jls-3.html

constexpr auto java_identifier_begin_char = choice(range('a', 'z'), range('A', 'Z'), '$', '_');
constexpr auto java_identifier_char = choice(range('a', 'z'), range('A', 'Z'), '$', '_', range('0', '9'));
constexpr auto java_identifier = sequence(java_identifier_begin_char, zero_or_more(java_identifier_char));
template <class T> constexpr auto java_keyword(T t) {
	return sequence(t, not_(java_identifier_char));
}
template <class... T> constexpr auto java_keywords(T... arguments) {
	return choice(java_keyword(arguments)...);
}

constexpr auto java_escape = sequence('\\', choice(
	'b', 't', 'n', 'f', 'r', 's',
	'"', '\'', '\\',
	repetition<1, 3>(range('0', '7')),
	sequence(one_or_more('u'), repetition<4, 4>(hex_digit))
));
constexpr auto java_string = choice(
	sequence(
		"\"\"\"",
		zero_or_more(' '),
		'\n',
		repetition(choice(highlight<Style::ESCAPE>(java_escape), any_char_but("\"\"\""))),
		optional("\"\"\"")
	),
	sequence(
		'"',
		repetition(choice(highlight<Style::ESCAPE>(java_escape), any_char_but(choice('"', '\n')))),
		optional('"')
	)
);
constexpr auto java_character = sequence(
	'\'',
	repetition(choice(highlight<Style::ESCAPE>(java_escape), any_char_but(choice('\'', '\n')))),
	optional('\'')
);

constexpr auto java_digits = sequence(
	range('0', '9'),
	zero_or_more(sequence(zero_or_more('_'), range('0', '9')))
);
constexpr auto java_hex_digits = sequence(
	hex_digit,
	zero_or_more(sequence(zero_or_more('_'), hex_digit))
);
constexpr auto java_binary_digits = sequence(
	range('0', '1'),
	zero_or_more(sequence(zero_or_more('_'), range('0', '1')))
);
constexpr auto java_number = sequence(
	choice(
		// hexadecimal
		sequence(
			'0',
			choice('x', 'X'),
			choice(
				sequence(java_hex_digits, optional('.'), optional(java_hex_digits)),
				sequence('.', java_hex_digits)
			),
			// exponent
			optional(sequence(
				choice('p', 'P'),
				optional(choice('+', '-')),
				java_digits
			))
		),
		// binary
		sequence(
			'0',
			choice('b', 'B'),
			java_binary_digits
		),
		// decimal or octal
		sequence(
			choice(
				sequence(java_digits, optional('.'), optional(java_digits)),
				sequence('.', java_digits)
			),
			// exponent
			optional(sequence(
				choice('e', 'E'),
				optional(choice('+', '-')),
				java_digits
			))
		)
	),
	// suffix
	optional(choice('l', 'L', 'f', 'F', 'd', 'D'))
);

struct java_file_name {
	static constexpr auto expression = ends_with(".java");
};

struct java_language {
	static constexpr auto expression = choice(
		// comments
		highlight<Style::COMMENT>(c_comment),
		// strings and characters
		highlight<Style::STRING>(java_string),
		highlight<Style::STRING>(java_character),
		// numbers
		highlight<Style::LITERAL>(java_number),
		// literals
		highlight<Style::LITERAL>(java_keywords(
			"null",
			"false",
			"true"
		)),
		// keywords
		highlight<Style::KEYWORD>(java_keywords(
			"this",
			"new",
			"var",
			"if",
			"else",
			"for",
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
			"class",
			"record",
			"interface",
			"enum",
			"extends",
			"implements",
			"abstract",
			"final",
			"public",
			"protected",
			"private",
			"static",
			"throws",
			"import",
			"package"
		)),
		// types
		highlight<Style::TYPE>(java_keywords(
			"void",
			"boolean",
			"char",
			"byte",
			"short",
			"int",
			"long",
			"float",
			"double"
		)),
		// identifiers
		java_identifier
	);
};
