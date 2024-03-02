#include "prism.hpp"
#include <vector>
#include <cmath>
#include <fstream>
#include <iostream>

static void set_background_color(const Color& color) {
	std::cout << "\e[48;2;"
		<< std::round(color.r * 255) << ";"
		<< std::round(color.g * 255) << ";"
		<< std::round(color.b * 255) << "m";
}
static void apply_style(const Style& style) {
	std::cout << "\e[38;2;"
		<< std::round(style.color.r * 255) << ";"
		<< std::round(style.color.g * 255) << ";"
		<< std::round(style.color.b * 255) << ";"
		<< (style.bold ? 1 : 22) << ";"
		<< (style.italic ? 3 : 23) << "m";
}
static void clear_style() {
	std::cout << "\e[m";
}

static std::vector<char> read_file(const char* file_name) {
	std::ifstream file(file_name);
	return std::vector<char>(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
}

template <class T> static void print(const T& file, const std::vector<Span>& spans, const Theme& theme) {
	for (const Span& span: spans) {
		apply_style(theme.styles[span.style - Style::DEFAULT]);
		std::cout.write(file.data() + span.start, span.end - span.start);
	}
}

static void highlight(const char* file_name, const char* language, const Theme& theme) {
	const auto file = read_file(file_name);
	StringInput input(file.data(), file.size());
	Tree tree;
	std::vector<Span> spans = highlight(language, &input, tree, 0, file.size());
	set_background_color(theme.background);
	std::cout << '\n';
	print(file, spans, theme);
	clear_style();
	std::cout << '\n';
}

static void highlight_incremental(const char* file_name, const char* language, const Theme& theme) {
	const auto file = read_file(file_name);
	StringInput input(file.data(), file.size());
	Tree tree;
	set_background_color(theme.background);
	std::cout << '\n';
	for (std::size_t i = 0; i < file.size(); i += 1000) {
		std::vector<Span> spans = highlight(language, &input, tree, i, std::min(i + 1000, file.size()));
		print(file, spans, theme);
	}
	clear_style();
	std::cout << '\n';
}

int main(int argc, char** argv) {
	initialize();
	const Theme& theme = get_theme("one-dark");
	const char* file_name = argc > 1 ? argv[1] : "test.c";
	const char* language = argc > 2 ? argv[2] : get_language(file_name);
	if (language == nullptr) {
		std::cerr << "prism does currently not support this language\n";
	}
	else {
		highlight(file_name, language, theme);
	}
}
