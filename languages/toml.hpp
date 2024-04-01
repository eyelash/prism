// https://toml.io/en/v1.0.0

constexpr auto toml_comment = sequence('#', repetition(any_char_but('\n')));

constexpr auto toml_escape = sequence('\\', choice(
	'b', 't', 'n', 'f', 'r',
	'"', '\\',
	sequence('u', repetition<4, 4>(hex_digit)),
	sequence('U', repetition<8, 8>(hex_digit))
));
constexpr auto toml_string = choice(
	sequence(
		"\"\"\"",
		repetition(choice(highlight(Style::ESCAPE, toml_escape), any_char_but("\"\"\""))),
		optional("\"\"\"")
	),
	sequence(
		"'''",
		repetition(any_char_but("'''")),
		optional("'''")
	),
	sequence(
		'"',
		repetition(choice(highlight(Style::ESCAPE, toml_escape), any_char_but(choice('"', '\n')))),
		optional('"')
	),
	sequence(
		'\'',
		repetition(any_char_but(choice('\'', '\n'))),
		optional('\'')
	)
);

constexpr auto toml_digits = sequence(
	range('0', '9'),
	zero_or_more(sequence(optional('_'), range('0', '9')))
);
constexpr auto toml_number = choice(
	// hexadecimal
	sequence(
		"0x",
		hex_digit,
		zero_or_more(sequence(optional('_'), hex_digit))
	),
	// binary
	sequence(
		"0b",
		range('0', '1'),
		zero_or_more(sequence(optional('_'), range('0', '1')))
	),
	// octal
	sequence(
		"0o",
		range('0', '7'),
		zero_or_more(sequence(optional('_'), range('0', '7')))
	),
	// decimal
	sequence(
		optional(choice('+', '-')),
		toml_digits,
		optional(sequence(
			'.',
			toml_digits
		)),
		// exponent
		optional(sequence(
			choice('e', 'E'),
			optional(choice('+', '-')),
			toml_digits
		))
	)
);

struct toml_file_name {
	static constexpr auto expression = ends_with(".toml");
};

struct toml_language {
	static constexpr auto expression = choice(
		// comments
		highlight(Style::COMMENT, toml_comment),
		// strings
		highlight(Style::STRING, toml_string),
		// numbers
		highlight(Style::LITERAL, toml_number),
		// literals
		highlight(Style::LITERAL, c_keywords(
			"false",
			"true"
		))
	);
};
