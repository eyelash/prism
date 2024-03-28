// https://toml.io/en/v1.0.0

constexpr auto toml_comment = sequence('#', repetition(but('\n')));

constexpr auto toml_escape = sequence('\\', choice(
	'b', 't', 'n', 'f', 'r',
	'"', '\\',
	sequence('u', repetition<4, 4>(hex_digit)),
	sequence('U', repetition<8, 8>(hex_digit))
));
constexpr auto toml_string = choice(
	sequence(
		"\"\"\"",
		repetition(choice(highlight(Style::ESCAPE, toml_escape), but("\"\"\""))),
		optional("\"\"\"")
	),
	sequence(
		"'''",
		repetition(but("'''")),
		optional("'''")
	),
	sequence(
		'"',
		repetition(choice(highlight(Style::ESCAPE, toml_escape), but(choice('"', '\n')))),
		optional('"')
	),
	sequence(
		'\'',
		repetition(but(choice('\'', '\n'))),
		optional('\'')
	)
);

constexpr auto toml_number = choice(
	// hexadecimal
	sequence(
		"0x",
		java_hex_digits
	),
	// binary
	sequence(
		"0b",
		java_binary_digits
	),
	// octal
	sequence(
		"0o",
		java_octal_digits
	),
	// decimal
	sequence(
		optional(choice('+', '-')),
		java_digits,
		optional(sequence(
			'.',
			java_digits
		)),
		// exponent
		optional(sequence(
			choice('e', 'E'),
			optional(choice('+', '-')),
			java_digits
		))
	)
);

struct toml_file_name {
	static constexpr auto expression = ends_with(".toml");
};

struct toml_language {
	static constexpr auto expression = scope(
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
