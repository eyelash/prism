// https://toml.io/en/v1.0.0

struct toml_file_name {
	static constexpr auto expression = ends_with(".toml");
};

struct toml_language {
	static constexpr auto expression = scope(
		// comments
		highlight(Style::COMMENT, sequence('#', repetition(but('\n'))))
	);
};
