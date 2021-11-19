#include "prism.hpp"
#include <fstream>
#include <vector>

#include <iostream>

class Color {
	unsigned char red;
	unsigned char green;
	unsigned char blue;
public:
	constexpr Color(unsigned char red, unsigned char green, unsigned char blue): red(red), green(green), blue(blue) {}
	constexpr unsigned int get_red() const {
		return red;
	}
	constexpr unsigned int get_green() const {
		return green;
	}
	constexpr unsigned int get_blue() const {
		return blue;
	}
};

class Style {
	Color color;
	bool bold;
	bool italic;
public:
	constexpr Style(Color color, bool bold = false, bool italic = false): color(color), bold(bold), italic(italic) {}
	static void set_background_color(Color color) {
		std::cout << "\e[48;2;"
			<< color.get_red() << ";"
			<< color.get_green() << ";"
			<< color.get_blue() << "m";
	}
	void apply() const {
		std::cout << "\e[38;2;"
			<< color.get_red() << ";"
			<< color.get_green() << ";"
			<< color.get_blue() << ";"
			<< (bold ? 1 : 22) << ";"
			<< (italic ? 3 : 23) << "m";
	}
	static void clear() {
		std::cout << "\e[m";
	}
};

constexpr Style styles[] = {
	Style(Color(0x00, 0x00, 0x00)),
	Style(Color(0xCC, 0x33, 0x33), true),
	Style(Color(0x00, 0x99, 0x99), true),
	Style(Color(0x66, 0x00, 0xCC), false),
};

const auto language = repetition(choice(
	highlight(1, choice(
		string("return"),
		string("if"),
		string("else")
	)),
	highlight(2, choice(
		string("void"),
		string("int")
	)),
	highlight(3, sequence(range('0', '9'), repetition(range('0', '9')))),
	sequence(choice(range('a', 'z'), character('_')), repetition(choice(range('a', 'z'), range('0', '9')))),
	any_character()
));

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
		auto source = read_file(argv[1]);
		const char* c = source.data();
		auto source_tree = language->match(c);
		Style::set_background_color(Color(0xFF, 0xFF, 0xFF));
		print(source.data(), source_tree);
		Style::clear();
	}
}
