// https://www.json.org/json-en.html

constexpr auto json_escape = sequence('\\', choice(
	'b', 't', 'n', 'f', 'r',
	'"', '\\', '/',
	sequence('u', repetition<4, 4>(hex_digit))
));
constexpr auto json_string = sequence(
	'"',
	repetition(choice(highlight<Style::ESCAPE>(json_escape), any_char_but(choice('"', '\n')))),
	optional('"')
);

constexpr auto json_number = sequence(
	optional('-'),
	one_or_more(range('0', '9')),
	optional(sequence(
		'.',
		one_or_more(range('0', '9'))
	)),
	// exponent
	optional(sequence(
		choice('e', 'E'),
		optional(choice('+', '-')),
		one_or_more(range('0', '9'))
	))
);

struct json_file_name {
	static constexpr auto expression = ends_with(".json");
};

struct json_language {
	static constexpr auto expression = choice(
		// strings
		highlight<Style::STRING>(json_string),
		// numbers
		highlight<Style::LITERAL>(json_number),
		// literals
		highlight<Style::LITERAL>(java_keywords(
			"null",
			"false",
			"true"
		))
	);
};
