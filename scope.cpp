#include <memory>
#include <vector>
#include <map>
#include <utility>
#include <cmath>
#include <algorithm>
#include <fstream>
#include <iostream>
#include "os.hpp"

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
	static void set_background_color(const Color& color) {
		std::cout << "\e[48;2;"
			<< std::round(color.r * 255) << ";"
			<< std::round(color.g * 255) << ";"
			<< std::round(color.b * 255) << "m";
	}
	void apply() const {
		std::cout << "\e[38;2;"
			<< std::round(color.r * 255) << ";"
			<< std::round(color.g * 255) << ";"
			<< std::round(color.b * 255) << ";"
			<< (bold ? 1 : 22) << ";"
			<< (italic ? 3 : 23) << "m";
	}
	static void clear() {
		std::cout << "\e[m";
	}
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
	Color background;
	Color selection;
	Color cursor;
	Style styles[8];
};

constexpr Theme one_dark_theme = {
	Color::hsl(220, 13, 18), // background
	Color::hsl(220, 13, 18 + 10), // selection
	Color::hsl(220, 100, 66), // cursor
	{
		Style(Color::hsl(220, 14, 71)), // text
		Style(Color::hsl(220, 10, 40), Style::ITALIC), // comments
		Style(Color::hsl(286, 60, 67)), // keywords
		Style(Color::hsl(286, 60, 67)), // operators
		Style(Color::hsl(187, 47, 55)), // types
		Style(Color::hsl(29, 54, 61)), // literals
		Style(Color::hsl(95, 38, 62)), // strings
		Style(Color::hsl(207, 82, 66)) // function names
	}
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
	friend std::ostream& operator <<(std::ostream& os, const Span& span) {
		return os << "(" << span.start << ", " << span.end << ") -> " << span.style;
	}
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

class InputAdapter {
	const Input* input;
	Input::Chunk chunk;
	std::size_t offset;
	std::size_t i;
public:
	InputAdapter(const Input* input): input(input), chunk({nullptr, nullptr, 0}), offset(0), i(0) {
		set_position(0);
	}
	char get() const {
		return i < chunk.size ? chunk.data[i] : '\0';
	}
	void advance() {
		++i;
		if (i == chunk.size) {
			offset += chunk.size;
			chunk = input->get_next_chunk(chunk.chunk);
			i = 0;
		}
	}
	std::size_t get_position() const {
		return offset + i;
	}
	void set_position(std::size_t pos) {
		if (pos >= offset && pos - offset < chunk.size) {
			i = pos - offset;
		}
		else {
			auto chunk_pair = input->get_chunk(pos);
			chunk = chunk_pair.first;
			offset = chunk_pair.second;
			i = pos - offset;
		}
	}
};

class Tree {
	struct Checkpoint {
		std::size_t pos;
		std::size_t max_pos;
	};
	std::vector<Checkpoint> checkpoints;
public:
	void add_checkpoint(std::size_t pos, std::size_t max_pos) {
		if (checkpoints.empty() || pos > checkpoints.back().pos) {
			checkpoints.push_back({pos, max_pos});
		}
	}
	std::size_t find_checkpoint(std::size_t pos) const {
		constexpr auto comp = [](const Checkpoint& checkpoint, std::size_t pos) {
			return checkpoint.pos > pos;
		};
		auto iter = std::lower_bound(checkpoints.rbegin(), checkpoints.rend(), pos, comp);
		return iter != checkpoints.rend() ? iter->pos : 0;
	}
	void edit(std::size_t pos) {
		constexpr auto comp = [](const Checkpoint& checkpoint, std::size_t pos) {
			return checkpoint.max_pos < pos;
		};
		auto iter = std::lower_bound(checkpoints.begin(), checkpoints.end(), pos, comp);
		checkpoints.erase(iter, checkpoints.end());
	}
};

class Spans {
	std::vector<Span>& spans;
	std::size_t start = 0;
	int style = Style::DEFAULT;
	void emit_span(std::size_t end, const Range& window) {
		if (end <= window.start || start >= window.end) {
			return;
		}
		if (spans.size() > 0) {
			Span& last_span = spans.back();
			if (last_span.end == start && last_span.style == style) {
				last_span.end = std::min(end, window.end);
				return;
			}
		}
		spans.emplace_back(std::max(start, window.start), std::min(end, window.end), style);
	}
public:
	Spans(std::vector<Span>& spans): spans(spans) {}
	int change_style(std::size_t pos, int new_style, const Range& window) {
		if (pos != start) {
			emit_span(pos, window);
			start = pos;
		}
		const int old_style = style;
		style = new_style;
		return old_style;
	}
	struct SavePoint {
		std::size_t spans_size;
		std::size_t start;
		int style;
	};
	SavePoint save() const {
		return {spans.size(), start, style};
	}
	void restore(const SavePoint& save_point) {
		spans.resize(save_point.spans_size);
		start = save_point.start;
		style = save_point.style;
	}
};

class Cursor {
	InputAdapter input;
	Tree& tree;
	Range window;
	std::size_t max_pos;
	Spans spans;
public:
	Cursor(const Input* input, Tree& tree, std::vector<Span>& spans, std::size_t window_start, std::size_t window_end): input(input), tree(tree), window(window_start, window_end), max_pos(0), spans(spans) {}
	char get() const {
		return input.get();
	}
	void advance() {
		input.advance();
	}
	int change_style(int new_style) {
		return spans.change_style(input.get_position(), new_style, window);
	}
	void add_checkpoint() {
		tree.add_checkpoint(input.get_position(), std::max(max_pos, input.get_position()));
	}
	void skip_to_checkpoint() {
		input.set_position(tree.find_checkpoint(window.start));
	}
	bool is_before_window_end() const {
		return input.get_position() < window.end;
	}
	struct SavePoint {
		std::size_t pos;
		Spans::SavePoint spans;
	};
	SavePoint save() const {
		return {input.get_position(), spans.save()};
	}
	void restore(const SavePoint& save_point) {
		max_pos = std::max(max_pos, input.get_position());
		input.set_position(save_point.pos);
		spans.restore(save_point.spans);
	}
};

template <class F> class Char {
	F f;
public:
	constexpr Char(F f): f(f) {}
	bool match(Cursor& cursor) const {
		if (f(cursor.get())) {
			cursor.advance();
			return true;
		}
		return false;
	}
};

class String {
	const char* string;
public:
	constexpr String(const char* string): string(string) {}
	bool match(Cursor& cursor) const {
		const auto save_point = cursor.save();
		for (const char* s = string; *s != '\0'; ++s) {
			if (cursor.get() == *s) {
				cursor.advance();
			}
			else {
				cursor.restore(save_point);
				return false;
			}
		}
		return true;
	}
};

template <class... T> class Tuple;
template <> class Tuple<> {
public:
	constexpr Tuple() {}
	bool match_sequence(Cursor& cursor) const {
		return true;
	}
	bool match_choice(Cursor& cursor) const {
		return false;
	}
};
template <class T0, class... T> class Tuple<T0, T...> {
	T0 t0;
	Tuple<T...> t;
public:
	constexpr Tuple(T0 t0, T... t): t0(t0), t(t...) {}
	bool match_sequence(Cursor& cursor) const {
		const auto save_point = cursor.save();
		if (t0.match(cursor)) {
			if (t.match_sequence(cursor)) {
				return true;
			}
			else {
				cursor.restore(save_point);
				return false;
			}
		}
		else {
			return false;
		}
	}
	bool match_choice(Cursor& cursor) const {
		if (t0.match(cursor)) {
			return true;
		}
		else {
			return t.match_choice(cursor);
		}
	}
};

template <class... T> class Sequence {
	Tuple<T...> t;
public:
	constexpr Sequence(T... t): t(t...) {}
	bool match(Cursor& cursor) const {
		return t.match_sequence(cursor);
	}
};

template <class... T> class Choice {
	Tuple<T...> t;
public:
	constexpr Choice(T... t): t(t...) {}
	bool match(Cursor& cursor) const {
		return t.match_choice(cursor);
	}
};

template <class T> class Repetition {
	T t;
public:
	constexpr Repetition(T t): t(t) {}
	bool match(Cursor& cursor) const {
		while (t.match(cursor)) {}
		return true;
	}
};

template <class T> class Optional {
	T t;
public:
	constexpr Optional(T t): t(t) {}
	bool match(Cursor& cursor) const {
		t.match(cursor);
		return true;
	}
};

template <class T> class Not {
	T t;
public:
	constexpr Not(T t): t(t) {}
	bool match(Cursor& cursor) const {
		const auto save_point = cursor.save();
		if (t.match(cursor)) {
			cursor.restore(save_point);
			return false;
		}
		else {
			return true;
		}
	}
};

template <class T> class Highlight {
	T t;
	int style;
public:
	constexpr Highlight(T t, int style): t(t), style(style) {}
	bool match(Cursor& cursor) const {
		const int old_style = cursor.change_style(style);
		const bool result = t.match(cursor);
		cursor.change_style(old_style);
		return result;
	}
};

constexpr auto get_language_node(char c) {
	return Char([c](char i) {
		return i == c;
	});
}
constexpr String get_language_node(const char* s) {
	return String(s);
}
template <class T> constexpr T get_language_node(T language_node) {
	return language_node;
}

constexpr auto range(char first, char last) {
	return Char([first, last](char c) {
		return c >= first && c <= last;
	});
}
constexpr auto any_char() {
	return Char([](char c) {
		return c != '\0';
	});
}
template <class... T> constexpr auto sequence(T... t) {
	return Sequence(get_language_node(t)...);
}
template <class... T> constexpr auto choice(T... t) {
	return Choice(get_language_node(t)...);
}
template <class T> constexpr auto repetition(T child) {
	return Repetition(get_language_node(child));
}
template <class T> constexpr auto optional(T child) {
	return Optional(get_language_node(child));
}
template <class T> constexpr auto not_(T child) {
	return Not(get_language_node(child));
}
template <class T> constexpr auto highlight(int style, T child) {
	return Highlight(get_language_node(child), style);
}
template <class T> constexpr auto zero_or_more(T child) {
	return repetition(child);
}
template <class T> constexpr auto one_or_more(T child) {
	return sequence(child, repetition(child));
}
template <class T> constexpr auto but(T child) {
	return sequence(not_(child), any_char());
}
constexpr auto end() {
	return not_(any_char());
}

class ScopeWrapper {
	class Interface {
	public:
		virtual ~Interface() = default;
		virtual bool match(Cursor&) const = 0;
	};
	template <class T> class Implementation final: public Interface {
		T t;
	public:
		constexpr Implementation(T t): t(t) {}
		bool match(Cursor& cursor) const override {
			return t.match(cursor);
		}
	};
	std::unique_ptr<Interface> scope;
public:
	ScopeWrapper() {}
	template <class T> ScopeWrapper(T t): scope(std::make_unique<Implementation<T>>(t)) {}
	bool match(Cursor& cursor) const {
		return scope->match(cursor);
	}
};

static std::map<StringView, ScopeWrapper> scopes;

template <class... T> class Scope {
	Tuple<T...> tuple;
public:
	constexpr Scope(T... t): tuple(t...) {}
	bool match(Cursor& cursor) const {
		return tuple.match_choice(cursor);
	}
};

template <class S, class E, class... T> class NestedScope {
	S start;
	Tuple<T...> t;
	E end;
	bool match_single(Cursor& cursor) const {
		if (!not_(end).match(cursor)) {
			return false;
		}
		if (t.match_choice(cursor)) {
			return true;
		}
		else {
			return any_char().match(cursor);
		}
	}
public:
	constexpr NestedScope(S start, E end, T... t): start(start), t(t...), end(end) {}
	bool match(Cursor& cursor) const {
		if (!start.match(cursor)) {
			return false;
		}
		while (match_single(cursor)) {}
		end.match(cursor);
		return true;
	}
};

class ScopeReference {
	const char* name;
public:
	constexpr ScopeReference(const char* name): name(name) {}
	bool match(Cursor& cursor) const {
		return scopes[name].match(cursor);
	}
};

class RootScope {
	const char* name;
	static bool match_single(Cursor& cursor, const ScopeWrapper& scope) {
		if (scope.match(cursor)) {
			return true;
		}
		else {
			return any_char().match(cursor);
		}
	}
public:
	constexpr RootScope(const char* name): name(name) {}
	bool match(Cursor& cursor) const {
		const ScopeWrapper& scope = scopes[name];
		cursor.skip_to_checkpoint();
		while (cursor.is_before_window_end() && match_single(cursor, scope)) {
			cursor.add_checkpoint();
		}
		return true;
	}
};

template <class... T> constexpr auto scope(T... t) {
	return Scope(get_language_node(t)...);
}
constexpr auto scope(const char* name) {
	return ScopeReference(name);
}
template <class S, class E, class... T> constexpr auto nested_scope(S start, E end, T... t) {
	return NestedScope(get_language_node(start), get_language_node(end), get_language_node(t)...);
}
constexpr auto root_scope(const char* name) {
	return RootScope(name);
}

constexpr auto c_whitespace_char = choice(' ', '\t', '\n', '\r', '\v', '\f');
constexpr auto c_identifier_begin_char = choice(range('a', 'z'), range('A', 'Z'), '_');
constexpr auto c_identifier_char = choice(range('a', 'z'), range('A', 'Z'), '_', range('0', '9'));
constexpr auto c_identifier = sequence(c_identifier_begin_char, zero_or_more(c_identifier_char));
template <class T> constexpr auto c_keyword(T t) {
	return sequence(t, not_(c_identifier_char));
}
template <class... T> constexpr auto c_keywords(T... arguments) {
	return choice(c_keyword(arguments)...);
}

static void initialize() {
	scopes["c"] = scope(
		one_or_more(c_whitespace_char),
		highlight(Style::COMMENT, choice(
			nested_scope("/*", "*/"),
			sequence("//", repetition(but('\n')))
		)),
		highlight(Style::STRING, sequence(
			'"',
			repetition(but(choice('"', '\n'))),
			optional('"')
		)),
		highlight(Style::LITERAL, one_or_more(range('0', '9'))),
		highlight(Style::KEYWORD, c_keywords(
			"if",
			"else",
			"for",
			"while",
			"do",
			"switch",
			"case",
			"default",
			"goto",
			"break",
			"continue",
			"return",
			"struct",
			"enum",
			"union",
			"typedef",
			"const",
			"static",
			"extern",
			"inline"
		)),
		highlight(Style::TYPE, c_keywords(
			"void",
			"char",
			"short",
			"int",
			"long",
			"float",
			"double",
			"unsigned",
			"signed"
		)),
		highlight(Style::KEYWORD, sequence('#', optional(c_identifier))),
		c_identifier
	);
}

static std::vector<Span> highlight(const char* language, const Input* input, Tree& tree, std::size_t window_start, std::size_t window_end) {
	std::vector<Span> spans;
	Cursor cursor(input, tree, spans, window_start, window_end);
	root_scope(language).match(cursor);
	cursor.change_style(Style::DEFAULT);
	return spans;
}

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
