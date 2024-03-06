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

constexpr Language haskell_language = {
	"haskell",
	[](const StringView& file_name) {
		return file_name.ends_with(".hs");
	},
	[]() {
		scopes["haskell"] = scope(
			// whitespace
			one_or_more(c_whitespace_char),
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
	}
};
