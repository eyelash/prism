#include "prism.hpp"
#include "c.hpp"
#include "one_dark.hpp"
#include <fstream>
#include <vector>
#include <iostream>

class Style {
	less::Color color;
	bool bold;
	bool italic;
public:
	Style(const less::Color& color, bool bold = false, bool italic = false): color(color), bold(bold), italic(italic) {}
	static void set_background_color(const less::Color& color) {
		std::cout << "\e[48;2;"
			<< std::round(less::red(color)) << ";"
			<< std::round(less::green(color)) << ";"
			<< std::round(less::blue(color)) << "m";
	}
	void apply() const {
		std::cout << "\e[38;2;"
			<< std::round(less::red(color)) << ";"
			<< std::round(less::green(color)) << ";"
			<< std::round(less::blue(color)) << ";"
			<< (bold ? 1 : 22) << ";"
			<< (italic ? 3 : 23) << "m";
	}
	static void clear() {
		std::cout << "\e[m";
	}
};

const Style styles[] = {
	Style(one_dark::syntax_fg),
	Style(one_dark::hue_3, false),
	Style(one_dark::hue_1, false),
	Style(one_dark::hue_6, false),
	Style(one_dark::mono_3, false, true),
};

static void print(const char* source, const std::unique_ptr<SourceNode>& node, int outer_style = 0) {
	int style = node->get_style() ? node->get_style() : outer_style;
	if (style != outer_style) {
		styles[style].apply();
	}
	if (node->get_children().empty()) {
		for (std::size_t i = 0; i < node->get_length(); ++i) {
			std::cout << source[i];
		}
	}
	else {
		for (auto& child: node->get_children()) {
			print(source, child, style);
			source += child->get_length();
		}
	}
	if (style != outer_style) {
		styles[outer_style].apply();
	}
}

static std::vector<char> read_file(const char* file_name) {
	std::ifstream file(file_name);
	std::vector<char> content;
	content.insert(content.end(), std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
	content.push_back('\0');
	return content;
}

int main(int argc, const char** argv) {
	if (argc > 1) {
		CLanguage language;
		auto source = read_file(argv[1]);
		const char* c = source.data();
		auto source_tree = language.language->match(c);
		Style::set_background_color(one_dark::syntax_bg);
		print(source.data(), source_tree);
		Style::clear();
	}
}
