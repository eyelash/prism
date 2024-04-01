// https://www.haskell.org/onlinereport/haskell2010/haskellch2.html

constexpr auto haskell_identifier_char = choice(range('a', 'z'), '_', range('A', 'Z'), range('0', '9'), '\'');
constexpr auto haskell_operator_char = choice('!', '#', '$', '%', '&', '*', '+', '.', '/', '<', '=', '>', '?', '@', '\\', '^', '|', '-', '~', ':');

struct haskell_block_comment {
	static constexpr auto expression = sequence(
		"{-",
		repetition(choice(
			reference<haskell_block_comment>(),
			any_char_but("-}")
		)),
		optional("-}")
	);
};
constexpr auto haskell_comment = choice(
	reference<haskell_block_comment>(),
	sequence(repetition<2>('-'), not_(haskell_operator_char), repetition(any_char_but('\n')))
);

constexpr auto haskell_escape = sequence('\\', choice(
	'a', 'b', 'f', 'n', 'r', 't', 'v', '\\', '"', '\'', '&',
	sequence('^', choice(range('A', 'Z'), '@', '[', '\\', ']', '^', '_')),
	"NUL", "SOH", "STX", "ETX", "EOT", "ENQ", "ACK", "BEL", "BS", "HT", "LF", "VT", "FF", "CR", "SO", "SI", "DLE", "DC1", "DC2", "DC3", "DC4", "NAK", "SYN", "ETB", "CAN", "EM", "SUB", "ESC", "FS", "GS", "RS", "US", "SP", "DEL",
	one_or_more(range('0', '9')),
	sequence('o', one_or_more(range('0', '7'))),
	sequence('x', one_or_more(hex_digit))
));
constexpr auto haskell_string = sequence(
	'"',
	repetition(choice(
		highlight(Style::ESCAPE, haskell_escape),
		highlight(Style::ESCAPE, sequence('\\', one_or_more(c_whitespace_char), '\\')),
		any_char_but(choice('"', '\n'))
	)),
	optional('"')
);
constexpr auto haskell_character = sequence(
	'\'',
	repetition(choice(highlight(Style::ESCAPE, haskell_escape), any_char_but(choice('\'', '\n')))),
	optional('\'')
);

constexpr auto haskell_number = choice(
	// hexadecimal
	sequence(
		'0',
		choice('x', 'X'),
		one_or_more(hex_digit)
	),
	// octal
	sequence(
		'0',
		choice('o', 'O'),
		one_or_more(range('0', '7'))
	),
	// decimal
	sequence(
		one_or_more(range('0', '9')),
		optional(sequence(
			'.',
			one_or_more(range('0', '9'))
		)),
		// exponent
		optional(sequence(
			choice('e', 'E'),
			optional(choice('+', '-')),
			one_or_more(range('0', '9'))
		))
	)
);

constexpr auto haskell_module = sequence(
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
	static constexpr auto expression = choice(
		// whitespace
		c_whitespace_char,
		// comments
		highlight(Style::COMMENT, haskell_comment),
		// strings and characters
		highlight(Style::STRING, haskell_string),
		highlight(Style::STRING, haskell_character),
		// numbers
		highlight(Style::LITERAL, haskell_number),
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
			optional(haskell_module),
			zero_or_more(' '),
			optional(choice(
				highlight(Style::KEYWORD, c_keyword("hiding")),
				highlight(Style::KEYWORD, c_keyword("as"))
			))
		),
		// qualified operators and identifiers
		sequence(
			haskell_module,
			optional(sequence(
				'.',
				choice(
					highlight(Style::OPERATOR, one_or_more(haskell_operator_char)),
					sequence(choice(range('a', 'z'), '_'), zero_or_more(haskell_identifier_char))
				)
			))
		),
		// unqualified operators and identifiers
		highlight(Style::OPERATOR, one_or_more(haskell_operator_char)),
		sequence(choice(range('a', 'z'), '_'), zero_or_more(haskell_identifier_char))
	);
};
