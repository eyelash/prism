constexpr auto haskell_identifier_char = choice(range('a', 'z'), '_', range('A', 'Z'), range('0', '9'), '\'');

struct haskell_block_comment {
	static constexpr auto expression = nested_scope(Style::COMMENT, "{-", "-}", reference<haskell_block_comment>());
};
constexpr auto haskell_comment = scope(
	reference<haskell_block_comment>(),
	highlight(Style::COMMENT, sequence("--", repetition(but('\n'))))
);

struct haskell_file_name {
	static constexpr auto expression = ends_with(".hs");
};

struct haskell_language {
	static constexpr auto expression = scope(
		// whitespace
		c_whitespace_char,
		// comments
		haskell_comment,
		// keywords
		highlight(Style::KEYWORD, c_keywords(
			"if",
			"then",
			"else",
			"let",
			"in",
			"where",
			"case",
			"of",
			"do",
			"type",
			"newtype",
			"data",
			"class",
			"instance",
			"module",
			"import"
		)),
		// types
		highlight(Style::TYPE, sequence(range('A', 'Z'), zero_or_more(haskell_identifier_char))),
		// identifiers
		sequence(choice(range('a', 'z'), '_'), zero_or_more(haskell_identifier_char))
	);
};
