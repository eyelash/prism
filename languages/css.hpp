constexpr auto css_comment = sequence("/*", repetition(any_char_but("*/")), optional("*/"));
constexpr auto css_identifier = sequence(
	optional('-'),
	choice(range('a', 'z'), range('A', 'Z'), '_'),
	zero_or_more(choice(range('a', 'z'), range('A', 'Z'), range('0', '9'), '_', '-'))
);
constexpr auto css_string = choice(
	sequence(
		'"',
		repetition(any_char_but(choice('"', '\n'))),
		optional('"')
	),
	sequence(
		'\'',
		repetition(any_char_but(choice('\'', '\n'))),
		optional('\'')
	)
);

struct css_file_name {
	static constexpr auto expression = ends_with(".css");
};

struct css_language {
	static constexpr auto expression = choice(
		highlight<Style::COMMENT>(css_comment),
		highlight<Style::KEYWORD>(sequence('#', css_identifier)),
		highlight<Style::TYPE>(sequence('.', css_identifier)),
		highlight<Style::STRING>(css_string)
	);
};
