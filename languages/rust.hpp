// https://doc.rust-lang.org/reference/index.html

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
constexpr auto rust_escape = sequence('\\', choice(
	't', 'n', 'r',
	'"', '\'', '\\',
	'0',
	sequence('x', range('0', '7'), hex_digit),
	sequence("u{", repetition<1, 6>(sequence(hex_digit, zero_or_more('_'))), '}')
));
constexpr auto rust_string = sequence(
	'"',
	repetition(choice(highlight(Style::ESCAPE, rust_escape), but('"'))),
	optional('"')
);
constexpr auto rust_character = sequence(
	'\'',
	choice(highlight(Style::ESCAPE, rust_escape), but(choice('\'', '\n'))),
	'\''
);
constexpr auto rust_lifetime = sequence('\'', c_identifier);

struct rust_file_name {
	static constexpr auto expression = ends_with(".rs");
};

struct rust_language {
	static constexpr auto expression = scope(
		// whitespace
		c_whitespace_char,
		// comments
		highlight(Style::COMMENT, rust_comment),
		// strings and characters
		highlight(Style::STRING, rust_string),
		highlight(Style::STRING, rust_character),
		// lifetimes
		highlight(Style::LITERAL, rust_lifetime),
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
