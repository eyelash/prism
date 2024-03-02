constexpr auto c_whitespace_char = choice(' ', '\t', '\n', '\r', '\v', '\f');
constexpr auto c_identifier_begin_char = choice(range('a', 'z'), range('A', 'Z'), '_');
constexpr auto c_identifier_char = choice(range('a', 'z'), range('A', 'Z'), '_', range('0', '9'));
constexpr auto c_identifier = sequence(c_identifier_begin_char, zero_or_more(c_identifier_char));
template <class T> constexpr auto c_keyword(T t) {
	return sequence(t, not_(c_identifier_char));
}
template <class... T> constexpr auto c_keywords(T... arguments) {
	return choice(c_keyword(arguments)...);
}

constexpr Language c_language = {
	"c",
	[](const StringView& file_name) {
		return file_name.ends_with(".c");
	},
	[]() {
		scopes["c"] = scope(
			one_or_more(c_whitespace_char),
			highlight(Style::COMMENT, choice(
				nested_scope("/*", "*/"),
				sequence("//", repetition(but('\n')))
			)),
			highlight(Style::STRING, sequence(
				'"',
				repetition(but(choice('"', '\n'))),
				optional('"')
			)),
			highlight(Style::LITERAL, one_or_more(range('0', '9'))),
			highlight(Style::KEYWORD, c_keywords(
				"if",
				"else",
				"for",
				"while",
				"do",
				"switch",
				"case",
				"default",
				"goto",
				"break",
				"continue",
				"return",
				"struct",
				"enum",
				"union",
				"typedef",
				"const",
				"static",
				"extern",
				"inline"
			)),
			highlight(Style::TYPE, c_keywords(
				"void",
				"char",
				"short",
				"int",
				"long",
				"float",
				"double",
				"unsigned",
				"signed"
			)),
			highlight(Style::KEYWORD, sequence('#', optional(c_identifier))),
			c_identifier
		);
	}
};
