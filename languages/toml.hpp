// https://toml.io/en/v1.0.0

static bool toml_file_name(ParseContext& context) {
	return ends_with(".toml").parse(context);
}

static bool toml_language(ParseContext& context) {
	static constexpr auto expression = scope(
		// comments
		highlight(Style::COMMENT, sequence('#', repetition(but('\n'))))
	);
	return expression.parse(context);
}
