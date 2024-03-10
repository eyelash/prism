#include "prism.hpp"
#include <iostream>

#include "themes/one_dark.hpp"
#include "themes/monokai.hpp"

constexpr Theme themes[] = {
	one_dark_theme,
	monokai_theme,
};

const Theme& prism::get_theme(const char* name) {
	for (const Theme& theme: themes) {
		if (StringView(theme.name) == name) {
			return theme;
		}
	}
	return one_dark_theme;
}

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

std::size_t prism::Tree::get_last_checkpoint() const {
	if (checkpoints.empty()) {
		return 0;
	}
	return checkpoints.back().pos;
}
void prism::Tree::add_checkpoint(std::size_t pos, std::size_t max_pos) {
	if (pos >= get_last_checkpoint() + 16) {
		checkpoints.push_back({pos, max_pos});
	}
}
std::size_t prism::Tree::find_checkpoint(std::size_t pos) const {
	constexpr auto comp = [](const Checkpoint& checkpoint, std::size_t pos) {
		return checkpoint.pos > pos;
	};
	auto iter = std::lower_bound(checkpoints.rbegin(), checkpoints.rend(), pos, comp);
	return iter != checkpoints.rend() ? iter->pos : 0;
}
void prism::Tree::edit(std::size_t pos) {
	constexpr auto comp = [](const Checkpoint& checkpoint, std::size_t pos) {
		return checkpoint.max_pos < pos;
	};
	auto iter = std::lower_bound(checkpoints.begin(), checkpoints.end(), pos, comp);
	checkpoints.erase(iter, checkpoints.end());
}

class Spans {
	std::vector<Span>& spans;
	std::size_t start = 0;
	int style = Style::DEFAULT;
	void emit_span(std::size_t end, const Range& window) {
		if (start == end) {
			return;
		}
		if (end <= window.start || start >= window.end) {
			return;
		}
		if (style == Style::DEFAULT) {
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
		emit_span(pos, window);
		start = pos;
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

class ParseContext {
	InputAdapter input;
	prism::Tree& tree;
	Range window;
	std::size_t max_pos;
	Spans spans;
public:
	ParseContext(const Input* input, prism::Tree& tree, std::vector<Span>& spans, std::size_t window_start, std::size_t window_end): input(input), tree(tree), window(window_start, window_end), max_pos(0), spans(spans) {}
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

template <class T, class = void> struct Unwrap {
	using type = T;
};
template <class T> struct Unwrap<T, std::void_t<typename T::type>> {
	using type = typename Unwrap<typename T::type>::type;
};
template <class T> using unwrap = typename Unwrap<T>::type;

template <char c> struct Char {
	static constexpr char get() {
		return c;
	}
	static bool parse(ParseContext& context) {
		if (context.get() != c) {
			return false;
		}
		context.advance();
		return true;
	}
};

template <char first, char last> struct CharRange {
	static bool parse(ParseContext& context) {
		const char c = context.get();
		if (!(c >= first && c <= last)) {
			return false;
		}
		context.advance();
		return true;
	}
};

struct AnyChar {
	static bool parse(ParseContext& context) {
		if (context.get() == '\0') {
			return false;
		}
		context.advance();
		return true;
	}
};

template <char... chars> struct String {
	static constexpr std::size_t size = sizeof...(chars);
	static const char* c_str() {
		static constexpr char s[sizeof...(chars) + 1] = {chars..., '\0'};
		return s;
	}
	static bool parse(ParseContext& context) {
		const auto save_point = context.save();
		for (const char* s = c_str(); *s != '\0'; ++s) {
			if (context.get() != *s) {
				context.restore(save_point);
				return false;
			}
			context.advance();
		}
		return true;
	}
};
template <class T, T... chars> constexpr String<chars...> operator ""_s() {
	return {};
}

template <class... T> struct Seq;
template <> struct Seq<> {
	static bool parse(ParseContext& context) {
		return true;
	}
};
template <class T0, class... T> struct Seq<T0, T...> {
	static bool parse(ParseContext& context) {
		const auto save_point = context.save();
		if (!unwrap<T0>::parse(context)) {
			return false;
		}
		if (!Seq<T...>::parse(context)) {
			context.restore(save_point);
			return false;
		}
		return true;
	}
};

template <class... T> struct Choice;
template <> struct Choice<> {
	static bool parse(ParseContext& context) {
		return false;
	}
};
template <class T0, class... T> struct Choice<T0, T...> {
	static bool parse(ParseContext& context) {
		if (unwrap<T0>::parse(context)) {
			return true;
		}
		else {
			return Choice<T...>::parse(context);
		}
	}
};

template <class T> struct Repeat {
	static bool parse(ParseContext& context) {
		while (unwrap<T>::parse(context)) {}
		return true;
	}
};

template <class T> struct Optional {
	static bool parse(ParseContext& context) {
		unwrap<T>::parse(context);
		return true;
	}
};

template <class T> struct Not {
	static bool parse(ParseContext& context) {
		const auto save_point = context.save();
		if (unwrap<T>::parse(context)) {
			context.restore(save_point);
			return false;
		}
		else {
			return true;
		}
	}
};

template <int style, class T> struct Highlight {
	static bool parse(ParseContext& context) {
		const int old_style = context.change_style(style);
		const bool result = unwrap<T>::parse(context);
		context.change_style(old_style);
		return result;
	}
};

class RootScope {
	bool (*f)(ParseContext&);
	bool parse_single(ParseContext& context) const {
		if (f(context)) {
			return true;
		}
		else {
			return AnyChar::parse(context);
		}
	}
public:
	constexpr RootScope(bool (*f)(ParseContext&)): f(f) {}
	bool parse(ParseContext& context) const {
		context.skip_to_checkpoint();
		while (context.is_before_window_end() && parse_single(context)) {
			context.add_checkpoint();
		}
		return true;
	}
};

#define C(c) Char<c>
#define S(s) decltype(s##_s)
#define range(first, last) CharRange<first, last>
#define any_char AnyChar
#define seq(...) Seq<__VA_ARGS__>
#define choice(...) Choice<__VA_ARGS__>
#define repeat(...) Repeat<__VA_ARGS__>
#define optional(...) Optional<__VA_ARGS__>
#define not_(...) Not<__VA_ARGS__>
#define highlight(style, ...) Highlight<style, __VA_ARGS__>
#define zero_or_more(...) repeat(__VA_ARGS__)
#define one_or_more(...) seq(__VA_ARGS__, repeat(__VA_ARGS__))
#define but(...) seq(not_(__VA_ARGS__), any_char)
#define end not_(any_char)
#define ends_with(...) seq(repeat(but(seq(__VA_ARGS__, end))), seq(__VA_ARGS__, end))

#define DECLARE_PARSER(parser) struct parser;
#define DEFINE_PARSER(parser, ...) struct parser { using type = __VA_ARGS__; };

using hex_digit = choice(range('0', '9'), range('a', 'f'), range('A', 'F'));

using c_whitespace_char = choice(C(' '), C('\t'), C('\n'), C('\r'), C('\v'), C('\f'));
using c_identifier_begin_char = choice(range('a', 'z'), range('A', 'Z'), C('_'));
using c_identifier_char = choice(range('a', 'z'), range('A', 'Z'), C('_'), range('0', '9'));
using c_identifier = seq(c_identifier_begin_char, zero_or_more(c_identifier_char));
template <class T> using CKeyword = seq(T, not_(c_identifier_char));
#define c_keyword(...) CKeyword<__VA_ARGS__>
template <class... T> using CKeywords = choice(c_keyword(T)...);
#define c_keywords(...) CKeywords<__VA_ARGS__>

DEFINE_PARSER(c_comment, choice(
	seq(S("/*"), repeat(but(S("*/"))), optional(S("*/"))),
	seq(S("//"), repeat(but(C('\n'))))
))

DEFINE_PARSER(c_file_name, ends_with(S(".c")))

DEFINE_PARSER(c_language,
	choice(
		// whitespace
		c_whitespace_char,
		// comments
		highlight(Style::COMMENT, c_comment),
		// keywords
		highlight(Style::KEYWORD, c_keywords(
			S("if"),
			S("else"),
			S("for"),
			S("while"),
			S("do"),
			S("switch"),
			S("case"),
			S("default"),
			S("goto"),
			S("break"),
			S("continue"),
			S("return"),
			S("struct"),
			S("enum"),
			S("union"),
			S("typedef"),
			S("const"),
			S("static"),
			S("extern"),
			S("inline")
		)),
		// identifiers
		c_identifier
	)
)

DEFINE_PARSER(rust_block_comment, seq(
	S("/*"),
	repeat(choice(
		rust_block_comment,
		but(S("*/"))
	)),
	optional(S("*/"))
))
using rust_comment = choice(
	rust_block_comment,
	seq(S("//"), repeat(but(C('\n'))))
);

DEFINE_PARSER(rust_file_name, ends_with(S(".rs")))

DEFINE_PARSER(rust_language,
	choice(
		// comments
		highlight(Style::COMMENT, rust_comment)
	)
)

#undef highlight

struct Language {
	const char* name;
	bool (*parse_file_name)(ParseContext&);
	bool (*parse)(ParseContext&);
};

template <class file_name_parser, class language_parser> constexpr Language language(const char* name) {
	return Language{name, unwrap<file_name_parser>::parse, unwrap<language_parser>::parse};
}

constexpr Language languages[] = {
	language<c_file_name, c_language>("C"),
	language<rust_file_name, rust_language>("Rust"),
};

const Language* prism::get_language(const char* file_name) {
	StringInput input(file_name);
	Tree tree;
	std::vector<Span> spans;
	ParseContext context(&input, tree, spans, 0, input.size());
	for (const Language& language: languages) {
		if (language.parse_file_name(context)) {
			return &language;
		}
	}
	return nullptr;
}

std::vector<Span> prism::highlight(const Language* language, const Input* input, Tree& tree, std::size_t window_start, std::size_t window_end) {
	std::vector<Span> spans;
	ParseContext context(input, tree, spans, window_start, window_end);
	RootScope(language->parse).parse(context);
	context.change_style(Style::DEFAULT);
	return spans;
}
