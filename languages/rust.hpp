struct rust_block_comment {
	static constexpr auto expression = sequence(
		"/*",
		repetition(choice(
			reference<rust_block_comment>(),
			but("*/")
		)),
		optional("*/")
	);
};
constexpr auto rust_comment = choice(
	reference<rust_block_comment>(),
	sequence("//", repetition(but('\n')))
);

struct rust_file_name {
	static constexpr auto expression = ends_with(".rs");
};

struct rust_language {
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
};
