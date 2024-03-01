#include "prism.hpp"
#include "c.hpp"
#include "one_dark.hpp"
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

static float hue(float h, float min, float max) {
	if (h > 360.f) h -= 360.f;
	if (h <= 60.f) return min + h / 60.f * (max - min);
	if (h <= 180.f) return max;
	if (h <= 210.f) return min + (210.f - h) / 60.f * (max - min);
	return min;
}

static Color hsl(float h, float s, float l) {
	h /= 60.f;
	s /= 100.f;
	l /= 100.f;
	s *= l < .5f ? l : 1.f - l;
	//const float max = l + s;
	//const float min = l - s;
	//float r = l - s + hue(h + 120.f) * (s * 2.f);
	float r, g, b;
	     if (h <= 1.f) r = 1.f, g = h - 0.f, b = 0.f;
	else if (h <= 2.f) r = 2.f - h, g = 1.f, b = 0.f;
	else if (h <= 3.f) r = 0.f, g = 1.f, b = h - 2.f;
	else if (h <= 4.f) r = 0.f, g = 4.f - h, b = 1.f;
	else if (h <= 5.f) r = h - 4.f, g = 0.f, b = 1.f;
	else               r = 1.f, g = 0.f, b = 6.f - h;
	r = l - s + r * (s * 2.f);
	g = l - s + g * (s * 2.f);
	b = l - s + b * (s * 2.f);
	return Color(r * 255.f, g * 255.f, b * 255.f);
}

class Color2 {
	static constexpr float minimum(float x, float y) {
		return x < y ? x : y;
	}
	static constexpr float minimum(float x, float y, float z) {
		return minimum(minimum(x, y), z);
	}
	static constexpr float maximum(float x, float y) {
		return x < y ? y : x;
	}
	static constexpr float maximum(float x, float y, float z) {
		return maximum(maximum(x, y), z);
	}
	constexpr float minimum() const {
		return minimum(red, green, blue);
	}
	constexpr float maximum() const {
		return maximum(red, green, blue);
	}
	constexpr Color2 multiply(float v) const {
		return Color2(red * v, green * v, blue * v, alpha);
	}
	constexpr Color2 add(float v) const {
		return Color2(red + v, green + v, blue + v, alpha);
	}
	static constexpr float unscale(float component, float min, float max) {
		return (component - min) / (max - min);
	}
	static constexpr Color2 hue_(float h) {
		return
		    h <= 1.f ? Color2(1.f, h-0.f, 0.f)
		  : h <= 2.f ? Color2(2.f-h, 1.f, 0.f)
		  : h <= 3.f ? Color2(0.f, 1.f, h-2.f)
		  : h <= 4.f ? Color2(0.f, 4.f-h, 1.f)
		  : h <= 5.f ? Color2(h-4.f, 0.f, 1.f)
		  : Color2(1.f, 0.f, 6.f-h);
	}
	constexpr float hue_() const {
		const float max = maximum();
		const float min = minimum();
		if (max == min) return 0.f;
		if (red == max && blue == min) return 0.f + unscale(green, min, max);
		if (green == max && blue == min) return 2.f - unscale(red, min, max);
		if (red == min && green == max) return 2.f + unscale(blue, min, max);
		if (red == min && blue == max) return 4.f - unscale(green, min, max);
		if (green == min && blue == max) return 4.f + unscale(red, min, max);
		if (red == max && green == min) return 6.f - unscale(blue, min, max);
	}
	static constexpr Color2 HSV_(float h, float s, float v) {
		s = s * v;
		return hue_(h).multiply(s).add(v - s);
	}
	static constexpr Color2 HSL_(float h, float s, float l) {
		s = s * (l < .5f ? l : 1.f - l);
		return hue_(h).multiply(s * 2.f).add(l - s);
	}
	constexpr float lightness_() const {
		return minimum() + (maximum() - minimum()) * .5f;
	}
	constexpr float saturation_() const {
		const float l = lightness_();
		if (l == 0.f || l == 1.f) return 0.f;
		const float s = maximum() - minimum();
		return s / (l < .5f ? l : 1.f - l);
	}
public:
	float red;
	float green;
	float blue;
	float alpha;
	constexpr Color2(float red, float green, float blue, float alpha = 1.f): red(red), green(green), blue(blue), alpha(alpha) {}
	static constexpr Color2 HSV(float h, float s, float v) {
		h /= 60.f;
		s /= 100.f;
		v /= 100.f;
		s *= v;
		return hue_(h).multiply(s).add(v - s);
	}
	static constexpr Color2 HSL(float h, float s, float l) {
		h /= 60.f;
		s /= 100.f;
		l /= 100.f;
		s *= l < .5f ? l : 1.f - l;
		return hue_(h).multiply(s).add(l - s);
	}
	constexpr float hue() const {
		return hue_() * 60.f;
	}
	constexpr float lightness() const {
		return lightness_() * 100.f;
	}
	constexpr float saturation() const {
		return saturation_() * 100.f;
	}
};

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
