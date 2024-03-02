#pragma once

#include <cstddef>
#include <utility>
#include <algorithm>
#include <vector>

class StringView {
	const char* data_;
	std::size_t size_;
public:
	static constexpr int strncmp(const char* s0, const char* s1, std::size_t n) {
		return n == 0 ? 0 : *s0 != *s1 ? *s0 - *s1 : strncmp(s0 + 1, s1 + 1, n - 1);
	}
	static constexpr const char* strchr(const char* s, char c) {
		return *s == c ? s : *s == '\0' ? nullptr : strchr(s + 1, c);
	}
	static constexpr std::size_t strlen(const char* s) {
		return strchr(s, '\0') - s;
	}
	constexpr StringView(const char* data_, std::size_t size_): data_(data_), size_(size_) {}
	constexpr StringView(const char* s): StringView(s, strlen(s)) {}
	constexpr StringView(): StringView(nullptr, 0) {}
	constexpr const char* data() const {
		return data_;
	}
	constexpr std::size_t size() const {
		return size_;
	}
	constexpr char operator [](std::size_t i) const {
		return data_[i];
	}
	constexpr char operator *() const {
		return *data_;
	}
	constexpr operator bool() const {
		return data_ != nullptr;
	}
	constexpr bool operator ==(const StringView& s) const {
		return size_ != s.size_ ? false : strncmp(data_, s.data_, size_) == 0;
	}
	constexpr bool operator !=(const StringView& s) const {
		return !operator ==(s);
	}
	constexpr bool operator <(const StringView& s) const {
		return size_ != s.size_ ? size_ < s.size_ : strncmp(data_, s.data_, size_) < 0;
	}
	constexpr const char* begin() const {
		return data_;
	}
	constexpr const char* end() const {
		return data_ + size_;
	}
	constexpr StringView substr(std::size_t pos, std::size_t count) const {
		return StringView(data_ + pos, count);
	}
	constexpr StringView substr(std::size_t pos) const {
		return StringView(data_ + pos, size_ - pos);
	}
	constexpr bool starts_with(const StringView& s) const {
		return size_ < s.size_ ? false : strncmp(data_, s.data_, s.size_) == 0;
	}
	constexpr bool ends_with(const StringView& s) const {
		return size_ < s.size_ ? false : strncmp(data_ + size_ - s.size_, s.data_, s.size_) == 0;
	}
};

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
	static constexpr int INHERIT = 0;
	static constexpr int WORD = 1;
	static constexpr int DEFAULT = 2;
	static constexpr int COMMENT = 3;
	static constexpr int KEYWORD = 4;
	static constexpr int OPERATOR = 5;
	static constexpr int TYPE = 6;
	static constexpr int LITERAL = 7;
	static constexpr int STRING = 8;
	static constexpr int FUNCTION = 9;
};

struct Theme {
	const char* name;
	Color background;
	Color background_active;
	Color selection;
	Color cursor;
	Color number_background;
	Color number_background_active;
	Style number;
	Style number_active;
	Style styles[8];
};

class Range {
public:
	std::size_t start;
	std::size_t end;
	Range() {}
	constexpr Range(std::size_t start, std::size_t end): start(start), end(end) {}
	constexpr Range operator |(const Range& range) const {
		return Range(std::min(start, range.start), std::max(end, range.end));
	}
	constexpr Range operator &(const Range& range) const {
		return Range(std::max(start, range.start), std::min(end, range.end));
	}
	constexpr operator bool() const {
		return start < end;
	}
};

class Span: public Range {
public:
	int style;
	Span() {}
	constexpr Span(std::size_t start, std::size_t end, int style): Range(start, end), style(style) {}
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
	constexpr StringInput(const char* s): StringInput(s, StringView::strlen(s)) {}
	std::pair<Chunk, std::size_t> get_chunk(std::size_t pos) const override {
		return {{nullptr, data_, size_}, 0};
	}
	Chunk get_next_chunk(const void* chunk) const override {
		return {nullptr, nullptr, 0};
	}
};

class Tree {
	struct Checkpoint {
		std::size_t pos;
		std::size_t max_pos;
	};
	std::vector<Checkpoint> checkpoints;
public:
	void add_checkpoint(std::size_t pos, std::size_t max_pos);
	std::size_t find_checkpoint(std::size_t pos) const;
	void edit(std::size_t pos);
};

const Theme& get_theme(const char* name);
const char* get_language(const char* file_name);
std::vector<Span> highlight(const char* language, const Input* input, Tree& tree, std::size_t window_start, std::size_t window_end);
