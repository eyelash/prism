// https://www.w3.org/TR/REC-xml/

constexpr auto xml_comment = sequence("<!--", repetition(but("-->")), optional("-->"));

constexpr auto xml_white_space = zero_or_more(choice(' ', '\t', '\n', '\r'));
constexpr auto xml_name_start_char = choice(range('a', 'z'), range('A', 'Z'), ':', '_');
constexpr auto xml_name_char = choice(xml_name_start_char, '-', '.', range('0', '9'));
constexpr auto xml_name = sequence(xml_name_start_char, zero_or_more(xml_name_char));
constexpr auto xml_escape = sequence(
	'&',
	choice(
		sequence('#', one_or_more(range('0', '9'))),
		sequence('#', 'x', one_or_more(hex_digit)),
		xml_name
	),
	';'
);
constexpr auto xml_string = choice(
	sequence(
		'"',
		repetition(choice(highlight(Style::ESCAPE, xml_escape), but(choice('"', '<')))),
		optional('"')
	),
	sequence(
		'\'',
		repetition(choice(highlight(Style::ESCAPE, xml_escape), but(choice('\'', '<')))),
		optional('\'')
	)
);

constexpr auto xml_syntax = scope(
	highlight(Style::COMMENT, xml_comment),
	highlight(Style::KEYWORD, sequence(
		'<',
		xml_name,
		xml_white_space,
		highlight(Style::TYPE, repetition(sequence(
			xml_name,
			xml_white_space,
			'=',
			xml_white_space,
			highlight(Style::STRING, xml_string),
			xml_white_space
		))),
		optional(choice('>', "/>"))
	)),
	highlight(Style::KEYWORD, sequence("</", xml_name, xml_white_space, optional('>'))),
	highlight(Style::ESCAPE, xml_escape)
);

struct xml_file_name {
	static constexpr auto expression = ends_with(choice(".xml", ".svg"));
};

struct xml_language {
	static constexpr auto expression = xml_syntax;
};
