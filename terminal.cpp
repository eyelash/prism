#include <prism.hpp>
#include <vector>
#include <cmath>
#include <fstream>
#include <iostream>

class FileInput final: public Input {
	std::vector<char> data_;
public:
	template <class I> FileInput(I first, I last): data_(first, last) {}
	char operator [](std::size_t i) const {
		return data_[i];
	}
	const char* data() const {
		return data_.data();
	}
	std::size_t size() const {
		return data_.size();
	}
	std::pair<Chunk, std::size_t> get_chunk(std::size_t pos) const override {
		return {{nullptr, data(), size()}, 0};
	}
	Chunk get_next_chunk(const void* chunk) const override {
		return {nullptr, "", 0};
	}
};

static FileInput read_file(const char* path) {
	std::ifstream file(path);
	return FileInput(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
}

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

static const char* get_file_name(const char* path) {
	const char* file_name = path;
	for (const char* i = path; *i != '\0'; ++i) {
		if (*i == '/') {
			file_name = i + 1;
		}
	}
	return file_name;
}

static void print(const FileInput& input, const std::vector<Span>& spans, const Theme& theme, std::size_t window_start, std::size_t window_end) {
	std::size_t i = window_start;
	for (const Span& span: spans) {
		if (span.start > i) {
			apply_style(theme.styles[0]);
			std::cout.write(input.data() + i, span.start - i);
		}
		apply_style(theme.styles[span.style - Style::DEFAULT]);
		std::cout.write(input.data() + span.start, span.end - span.start);
		i = span.end;
	}
	if (window_end > i) {
		apply_style(theme.styles[0]);
		std::cout.write(input.data() + i, window_end - i);
	}
}

static void highlight(const char* path, const Language* language, const Theme& theme) {
	FileInput input = read_file(path);
	Cache cache;
	set_background_color(theme.background);
	std::cout << '\n';
	std::vector<Span> spans = prism::highlight(language, &input, cache, 0, input.size());
	print(input, spans, theme, 0, input.size());
	clear_style();
	std::cout << '\n';
}

static void highlight_incremental(const char* path, const Language* language, const Theme& theme) {
	FileInput input = read_file(path);
	Cache cache;
	set_background_color(theme.background);
	std::cout << '\n';
	for (std::size_t i = 0; i < input.size(); i += 1000) {
		std::vector<Span> spans = prism::highlight(language, &input, cache, i, std::min(i + 1000, input.size()));
		print(input, spans, theme, i, std::min(i + 1000, input.size()));
	}
	clear_style();
	std::cout << '\n';
}

int main(int argc, const char** argv) {
	if (argc <= 1) {
		std::cerr << "Usage: " << argv[0] << " FILE [THEME]\n";
		return 1;
	}
	const char* path = argv[1];
	const Language* language = prism::get_language(get_file_name(path));
	if (language == nullptr) {
		std::cerr << "prism does currently not support this language\n";
		return 1;
	}
	const Theme& theme = prism::get_theme(argc > 2 ? argv[2] : "one-dark");
	highlight(path, language, theme);
}
