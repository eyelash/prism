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

Cache::Node::Node(std::size_t start_pos, std::size_t start_max_pos): start_pos(start_pos), start_max_pos(start_max_pos) {}
std::size_t Cache::Node::get_last_checkpoint() const {
	if (checkpoints.empty()) {
		return start_pos;
	}
	return checkpoints.back().pos;
}
void Cache::Node::add_checkpoint(std::size_t pos, std::size_t max_pos) {
	if (pos >= get_last_checkpoint() + 16) {
		checkpoints.push_back({pos, max_pos});
	}
}
Cache::Checkpoint Cache::Node::find_checkpoint(std::size_t pos) const {
	auto iter = std::lower_bound(checkpoints.rbegin(), checkpoints.rend(), pos, [](const Checkpoint& checkpoint, std::size_t pos) {
		return checkpoint.pos > pos;
	});
	if (iter != checkpoints.rend()) {
		return *iter;
	}
	else {
		return {start_pos, start_max_pos};
	}
}
Cache::Node* Cache::Node::get_child(std::size_t pos, std::size_t max_pos) {
	auto iter = std::lower_bound(children.begin(), children.end(), pos, [](const Node& child, std::size_t pos) {
		return child.start_pos < pos;
	});
	if (iter != children.end()) {
		return &*iter;
	}
	children.emplace_back(pos, max_pos);
	return &children.back();
}
void Cache::Node::invalidate(std::size_t pos) {
	{
		auto iter = std::lower_bound(checkpoints.begin(), checkpoints.end(), pos, [](const Checkpoint& checkpoint, std::size_t pos) {
			return checkpoint.max_pos < pos;
		});
		checkpoints.erase(iter, checkpoints.end());
	}
	{
		auto iter = std::lower_bound(children.begin(), children.end(), pos, [](const Node& child, std::size_t pos) {
			return child.start_max_pos < pos;
		});
		children.erase(iter, children.end());
	}
	if (!children.empty() && children.back().start_pos >= get_last_checkpoint()) {
		children.back().invalidate(pos);
	}
}
Cache::Cache(): root_node(0, 0) {}
Cache::Node* Cache::get_root_node() {
	return &root_node;
}
void Cache::invalidate(std::size_t pos) {
	root_node.invalidate(pos);
}

class Spans {
	std::vector<Span>& spans;
	std::size_t start;
	int style;
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
	Spans(std::vector<Span>& spans): spans(spans), start(0), style(Style::DEFAULT) {}
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
	Cache::Node* node;
	Range window;
	std::size_t max_pos;
	Spans spans;
public:
	ParseContext(const Input* input, Cache& cache, std::vector<Span>& spans, std::size_t window_start, std::size_t window_end): input(input), node(cache.get_root_node()), window(window_start, window_end), max_pos(0), spans(spans) {}
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
		node->add_checkpoint(input.get_position(), std::max(max_pos, input.get_position()));
	}
	void skip_to_checkpoint() {
		const auto checkpoint = node->find_checkpoint(window.start);
		input.set_position(checkpoint.pos);
		max_pos = checkpoint.max_pos;
	}
	Cache::Node* enter_scope() {
		return std::exchange(node, node->get_child(input.get_position(), std::max(max_pos, input.get_position())));
	}
	void leave_scope(Cache::Node* old_node) {
		node = old_node;
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
	static constexpr bool always_succeeds() {
		return false;
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
	static constexpr bool always_succeeds() {
		return false;
	}
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
	static constexpr bool always_succeeds() {
		return false;
	}
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
	static constexpr bool always_succeeds() {
		return false;
	}
	static bool parse(ParseContext& context) {
		static constexpr char string[] = {chars..., '\0'};
		if (*string == '\0') {
			return true;
		}
		if (context.get() != *string) {
			return false;
		}
		const auto save_point = context.save();
		context.advance();
		for (const char* s = string + 1; *s != '\0'; ++s) {
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
	static constexpr bool always_succeeds() {
		return true;
	}
	static bool parse(ParseContext& context) {
		return true;
	}
};
template <class T0, class... T> struct Seq<T0, T...> {
	static constexpr bool always_succeeds() {
		return unwrap<T0>::always_succeeds() && Seq<T...>::always_succeeds();
	}
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
	static constexpr bool always_succeeds() {
		return false;
	}
	static bool parse(ParseContext& context) {
		return false;
	}
};
template <class T0, class... T> struct Choice<T0, T...> {
	static constexpr bool always_succeeds() {
		return unwrap<T0>::always_succeeds() || Choice<T...>::always_succeeds();;
	}
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
	static constexpr bool always_succeeds() {
		return true;
	}
	static bool parse(ParseContext& context) {
		while (unwrap<T>::parse(context)) {}
		return true;
	}
};

template <class T> struct Optional {
	static constexpr bool always_succeeds() {
		return true;
	}
	static bool parse(ParseContext& context) {
		unwrap<T>::parse(context);
		return true;
	}
};

template <class T> struct Not {
	static constexpr bool always_succeeds() {
		return false;
	}
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
	static constexpr bool always_succeeds() {
		return unwrap<T>::always_succeeds();
	}
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

#include "type_based_languages/c.hpp"

#undef highlight

struct Language {
	const char* name;
	bool (*parse_file_name)(ParseContext&);
	bool (*parse)(ParseContext&);
};

template <class file_name_parser, class language_parser> constexpr Language language(const char* name) {
	return {name, unwrap<file_name_parser>::parse, unwrap<language_parser>::parse};
}

constexpr Language languages[] = {
	language<c_file_name, c_language>("C"),
};

const Language* prism::get_language(const char* file_name) {
	StringInput input(file_name);
	Cache cache;
	std::vector<Span> spans;
	ParseContext context(&input, cache, spans, 0, input.size());
	for (const Language& language: languages) {
		if (language.parse_file_name(context)) {
			return &language;
		}
	}
	return nullptr;
}

std::vector<Span> prism::highlight(const Language* language, const Input* input, Cache& cache, std::size_t window_start, std::size_t window_end) {
	std::vector<Span> spans;
	ParseContext context(input, cache, spans, window_start, window_end);
	RootScope(language->parse).parse(context);
	context.change_style(Style::DEFAULT);
	return spans;
}
