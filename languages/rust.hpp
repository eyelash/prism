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

constexpr Language rust_language = {
	"rust",
	[](const StringView& file_name) {
		return file_name.ends_with(".rs");
	},
	[]() {
		scopes["rust"] = scope(
			// whitespace
			one_or_more(c_whitespace_char),
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
	}
};
