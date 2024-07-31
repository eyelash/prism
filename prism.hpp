#pragma once

#include <cstddef>
#include <utility>
#include <algorithm>
#include <vector>
#include <string>

class Color {
	static constexpr float hue_function(float h) {
		return h <= 60.f ? (h / 60.f) : h <= 180.f ? 1.f : h <= 240.f ? (4.f - h / 60.f) : 0.f;
	}
	static constexpr Color hue(float h) {
		return Color(
			hue_function(h < 240.f ? h + 120.f : h - 240.f),
			hue_function(h),
			hue_function(h < 120.f ? h + 240.f : h - 120.f)
		);
	}
public:
	float r;
	float g;
	float b;
	float a;
	constexpr Color(float r, float g, float b, float a = 1.f): r(r), g(g), b(b), a(a) {}
	constexpr Color operator +(const Color& c) const {
		return Color(
			(r * a * (1.f - c.a) + c.r * c.a) / (a * (1.f - c.a) + c.a),
			(g * a * (1.f - c.a) + c.g * c.a) / (a * (1.f - c.a) + c.a),
			(b * a * (1.f - c.a) + c.b * c.a) / (a * (1.f - c.a) + c.a),
			a * (1.f - c.a) + c.a
		);
	}
	static constexpr Color hsv(float h, float s, float v) {
		return hue(h) + Color(1.f, 1.f, 1.f, 1.f - s / 100.f) + Color(0.f, 0.f, 0.f, 1.f - v / 100.f);
	}
	static constexpr Color hsl(float h, float s, float l) {
		return hue(h) + Color(.5f, .5f, .5f, 1.f - s / 100.f) + (l < 50.f ? Color(0.f, 0.f, 0.f, 1.f - l / 50.f) : Color(1.f, 1.f, 1.f, l / 50.f - 1.f));
	}
	constexpr Color with_alpha(float a) const {
		return Color(r, g, b, this->a * a);
	}
};

class Style {
public:
	static constexpr int BOLD = 1 << 0;
	static constexpr int ITALIC = 1 << 1;
	Color color;
	bool bold;
	bool italic;
	constexpr Style(const Color& color, bool bold, bool italic): color(color), bold(bold), italic(italic) {}
	constexpr Style(const Color& color, int attributes = 0): color(color), bold(attributes & BOLD), italic(attributes & ITALIC) {}
	static constexpr int INHERIT = -1;
	static constexpr int DEFAULT = 0;
	static constexpr int LINE_NUMBER = 1;
	static constexpr int LINE_NUMBER_ACTIVE = 2;
	static constexpr int COMMENT = 3;
	static constexpr int KEYWORD = 4;
	static constexpr int OPERATOR = 5;
	static constexpr int TYPE = 6;
	static constexpr int LITERAL = 7;
	static constexpr int STRING = 8;
	static constexpr int ESCAPE = 9;
	static constexpr int FUNCTION = 10;
};

struct Theme {
	const char* name;
	Color background;
	Color background_active;
	Color selection;
	Color cursor;
	Color gutter_background;
	Color gutter_background_active;
	Style styles[11];
};

struct Language;

class Range {
public:
	std::size_t start;
	std::size_t end;
	constexpr Range(std::size_t start, std::size_t end): start(start), end(end) {}
	constexpr Range operator |(const Range& range) const {
		return Range(std::min(start, range.start), std::max(end, range.end));
	}
	constexpr Range operator &(const Range& range) const {
		return Range(std::max(start, range.start), std::min(end, range.end));
	}
	explicit constexpr operator bool() const {
		return start < end;
	}
};

class Span: public Range {
public:
	int style;
	constexpr Span(std::size_t start, std::size_t end, int style): Range(start, end), style(style) {}
	constexpr bool operator ==(const Span& span) const {
		return std::tie(start, end, style) == std::tie(span.start, span.end, span.style);
	}
	constexpr bool operator <(const Span& span) const {
		return std::tie(start, end, style) < std::tie(span.start, span.end, span.style);
	}
	/*friend std::ostream& operator <<(std::ostream& os, const Span& span) {
		return os << "(" << span.start << ", " << span.end << ") -> " << span.style;
	}*/
};

class Input {
public:
	struct Chunk {
		const void* chunk;
		const char* data;
		std::size_t size;
	};
	virtual ~Input() = default;
	virtual std::pair<Chunk, std::size_t> get_chunk(std::size_t pos) const = 0;
	virtual Chunk get_next_chunk(const void* chunk) const = 0;
};

class StringInput final: public Input {
	const char* data_;
	std::size_t size_;
public:
	constexpr StringInput(const char* data_, std::size_t size_): data_(data_), size_(size_) {}
	constexpr StringInput(const char* s): StringInput(s, std::char_traits<char>::length(s)) {}
	constexpr std::size_t size() const {
		return size_;
	}
	std::pair<Chunk, std::size_t> get_chunk(std::size_t pos) const override {
		return {{nullptr, data_, size_}, 0};
	}
	Chunk get_next_chunk(const void* chunk) const override {
		return {nullptr, "", 0};
	}
};

class Cache {
public:
	struct Checkpoint {
		std::size_t pos;
		std::size_t max_pos;
	};
	struct Node {
		const void* expression;
		std::size_t start_pos;
		std::size_t start_max_pos;
		std::vector<Checkpoint> checkpoints;
		std::vector<Node> children;
		Node(const void* expression, std::size_t start_pos, std::size_t start_max_pos);
		std::size_t get_last_checkpoint() const;
		void add_checkpoint(std::size_t pos, std::size_t max_pos);
		const Checkpoint* find_checkpoint(std::size_t pos) const;
		Node* find_child(const void* expression, std::size_t pos);
		Node* add_child(const void* expression, std::size_t pos, std::size_t max_pos);
		void invalidate(std::size_t pos);
	};
private:
	Node root_node;
public:
	Cache();
	~Cache();
	Node* get_root_node();
	void invalidate(std::size_t pos);
};

namespace prism {

const Theme& get_theme(const char* name);
const Language* get_language(const char* file_name);
std::vector<Span> highlight(const Language* language, const Input* input, Cache& cache, std::size_t window_start, std::size_t window_end);

}
