// https://html.spec.whatwg.org/multipage/syntax.html

constexpr auto html_comment = sequence("<!--", repetition(any_char_but("-->")), optional("-->"));

constexpr auto html_whitespace = choice(' ', '\t', '\n', '\r');
constexpr auto html_alphanumeric = choice(range('0', '9'), range('A', 'Z'), range('a', 'z'));

constexpr auto html_attribute = sequence(
	one_or_more(any_char_but(choice('\t', '\n', ' ', '"', '\'', '>', '/', '='))),
	zero_or_more(html_whitespace),
	optional(sequence(
		'=',
		zero_or_more(html_whitespace),
		highlight<Style::STRING>(optional(choice(
			one_or_more(any_char_but(choice(html_whitespace, '"', '\'', '=', '<', '>', '`'))),
			sequence(
				'"',
				repetition(any_char_but('"')),
				optional('"')
			),
			sequence(
				'\'',
				repetition(any_char_but('\'')),
				optional('\'')
			)
		)))
	))
);

constexpr auto html_start_tag(const char* name) {
	return sequence(
		'<',
		case_insensitive(name),
		zero_or_more(html_whitespace),
		zero_or_more(sequence(
			highlight<Style::TYPE>(html_attribute),
			zero_or_more(html_whitespace)
		)),
		'>'
	);
}
constexpr auto html_end_tag(const char* name) {
	return sequence(
		"</",
		case_insensitive(name),
		zero_or_more(html_whitespace),
		optional('>')
	);
}

struct html_file_name {
	static constexpr auto expression = ends_with(".html");
};

struct html_language {
	static constexpr auto expression = choice(
		// comments
		highlight<Style::COMMENT>(html_comment),
		// DOCTYPE
		highlight<Style::KEYWORD>(sequence(
			"<!",
			repetition(any_char_but('>')),
			optional('>')
		)),
		// end tags
		highlight<Style::KEYWORD>(sequence(
			"</",
			optional(sequence(
				one_or_more(html_alphanumeric),
				zero_or_more(html_whitespace),
				optional('>')
			))
		)),
		// CSS
		sequence(
			highlight<Style::KEYWORD>(html_start_tag("style")),
			repetition(sequence(not_(html_end_tag("style")), choice(reference<css_language>(), any_char()))),
			highlight<Style::KEYWORD>(optional(html_end_tag("style")))
		),
		// JavaScript
		sequence(
			highlight<Style::KEYWORD>(html_start_tag("script")),
			repetition(sequence(not_(html_end_tag("script")), choice(reference<javascript_language>(), any_char()))),
			highlight<Style::KEYWORD>(optional(html_end_tag("script")))
		),
		// start tags
		highlight<Style::KEYWORD>(sequence(
			'<',
			optional(sequence(
				one_or_more(html_alphanumeric),
				zero_or_more(html_whitespace),
				zero_or_more(sequence(
					highlight<Style::TYPE>(html_attribute),
					zero_or_more(html_whitespace)
				)),
				optional('>')
			))
		))
	);
};
