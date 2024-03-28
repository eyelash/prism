constexpr auto haskell_identifier_char = choice(range('a', 'z'), '_', range('A', 'Z'), range('0', '9'), '\'');

struct haskell_block_comment {
	static constexpr auto expression = sequence(
		"{-",
		repetition(choice(
			reference<haskell_block_comment>(),
			but("-}")
		)),
		optional("-}")
	);
};
constexpr auto haskell_comment = choice(
	reference<haskell_block_comment>(),
	sequence("--", repetition(but('\n')))
);

constexpr auto haskell_module_name = sequence(
	highlight(Style::TYPE, sequence(range('A', 'Z'), zero_or_more(haskell_identifier_char))),
	zero_or_more(sequence(
		'.',
		highlight(Style::TYPE, sequence(range('A', 'Z'), zero_or_more(haskell_identifier_char)))
	))
);

struct haskell_file_name {
	static constexpr auto expression = ends_with(".hs");
};

struct haskell_language {
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
			"module"
		)),
		// imports
		sequence(
			highlight(Style::KEYWORD, c_keyword("import")),
			zero_or_more(' '),
			optional(highlight(Style::KEYWORD, c_keyword("qualified"))),
			zero_or_more(' '),
			optional(haskell_module_name),
			zero_or_more(' '),
			optional(choice(
				highlight(Style::KEYWORD, c_keyword("hiding")),
				highlight(Style::KEYWORD, c_keyword("as"))
			))
		),
		// types
		highlight(Style::TYPE, sequence(range('A', 'Z'), zero_or_more(haskell_identifier_char))),
		// identifiers
		sequence(choice(range('a', 'z'), '_'), zero_or_more(haskell_identifier_char))
	);
};
