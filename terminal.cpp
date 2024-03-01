#include "prism.hpp"
#include <vector>
#include <fstream>
#include <iostream>

static std::vector<char> read_file(const char* file_name) {
	std::ifstream file(file_name);
	return std::vector<char>(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
}

template <class T> static void print(const T& file, const std::vector<Span>& spans) {
	for (const Span& span: spans) {
		one_dark_theme.styles[span.style - Style::DEFAULT].apply();
		std::cout.write(file.data() + span.start, span.end - span.start);
	}
}

static void highlight(const char* file_name, const char* language) {
	const auto file = read_file(file_name);
	StringInput input(file.data(), file.size());
	Tree tree;
	std::vector<Span> spans = highlight(language, &input, tree, 0, file.size());
	Style::set_background_color(one_dark_theme.background);
	std::cout << '\n';
	print(file, spans);
	Style::clear();
	std::cout << '\n';
}

static void highlight_incremental(const char* file_name, const char* language) {
	const auto file = read_file(file_name);
	StringInput input(file.data(), file.size());
	Tree tree;
	Style::set_background_color(one_dark_theme.background);
	std::cout << '\n';
	for (std::size_t i = 0; i < file.size(); i += 1000) {
		std::vector<Span> spans = highlight(language, &input, tree, i, std::min(i + 1000, file.size()));
		print(file, spans);
	}
	Style::clear();
	std::cout << '\n';
}

int main(int argc, char** argv) {
	initialize();
	const char* file_name = argc > 1 ? argv[1] : "test.c";
	const char* language = argc > 2 ? argv[2] : "c";
	highlight(file_name, language);
}
