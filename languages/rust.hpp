constexpr auto rust_block_comment = recursive([](auto rust_block_comment) {
	return sequence(
		"/*",
		repetition(choice(
			rust_block_comment,
			but("*/")
		)),
		optional("*/")
	);
});
constexpr auto rust_comment = choice(
	rust_block_comment,
	sequence("//", repetition(but('\n')))
);

static bool rust_file_name(ParseContext& context) {
	return ends_with(".rs").parse(context);
}

static bool rust_language(ParseContext& context) {
	static constexpr auto expression = scope(
		// whitespace
		c_whitespace_char,
		// comments
		highlight(Style::COMMENT, rust_comment),
		// literals
		highlight(Style::LITERAL, c_keywords(
			"false",
			"true"
		)),
		// keywords
		highlight(Style::KEYWORD, c_keywords(
			"let",
			"mut",
			"if",
			"else",
			"while",
			"for",
			"in",
			"loop",
			"match",
			"break",
			"continue",
			"return",
			"fn",
			"struct",
			"enum",
			"trait",
			"type",
			"impl",
			"where",
			"pub",
			"use",
			"mod"
		)),
		// types
		highlight(Style::TYPE, c_keywords(
			"bool",
			"char",
			sequence(
				choice('u', 'i'),
				choice("8", "16", "32", "64", "128", "size")
			),
			sequence('f', choice("32", "64")),
			"str"
		)),
		// identifiers
		c_identifier
	);
	return expression.parse(context);
}
