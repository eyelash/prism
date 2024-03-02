constexpr auto python_comment = sequence('#', repetition(but('\n')));

constexpr Language python_language = {
	"python",
	[](const StringView& file_name) {
		return file_name.ends_with(".py");
	},
	[]() {
		scopes["python"] = scope(
			// comments
			highlight(Style::COMMENT, python_comment),
			// literals
			highlight(Style::LITERAL, c_keywords(
				"None",
				"False",
				"True"
			)),
			// keywords
			highlight(Style::KEYWORD, c_keywords(
				"lambda",
				"and",
				"or",
				"not",
				"if",
				"elif",
				"else",
				"for",
				"in",
				"while",
				"break",
				"continue",
				"return",
				"def",
				"class",
				"import"
			))
		);
	}
};
