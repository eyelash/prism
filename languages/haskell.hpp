constexpr auto haskell_identifier_char = choice(range('a', 'z'), '_', range('A', 'Z'), range('0', '9'), '\'');

constexpr auto haskell_block_comment = recursive([](auto haskell_block_comment) {
	return sequence(
		"{-",
		repetition(choice(
			haskell_block_comment,
			but("-}")
		)),
		optional("-}")
	);
});
constexpr auto haskell_comment = choice(
	haskell_block_comment,
	sequence("--", repetition(but('\n')))
);

static bool haskell_file_name(ParseContext& context) {
	return ends_with(".hs").parse(context);
}

static bool haskell_language(ParseContext& context) {
	static constexpr auto expression = scope(
		// whitespace
		c_whitespace_char,
		// comments
		highlight(Style::COMMENT, haskell_comment),
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
		highlight(Style::TYPE, sequence(range('A', 'Z'), repetition(haskell_identifier_char))),
		// identifiers
		sequence(choice(range('a', 'z'), '_'), repetition(haskell_identifier_char))
	);
	return expression.parse(context);
}
