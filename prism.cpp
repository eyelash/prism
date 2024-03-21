#include "prism.hpp"
#include <memory>

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

template <class F> class Char {
	F f;
public:
	static constexpr bool always_succeeds() {
		return false;
	}
	constexpr Char(F f): f(f) {}
	bool parse(ParseContext& context, bool can_checkpoint) const {
		if (!f(context.get())) {
			return false;
		}
		context.advance();
		return true;
	}
};

class String {
	const char* string;
public:
	static constexpr bool always_succeeds() {
		return false;
	}
	constexpr String(const char* string): string(string) {}
	bool parse(ParseContext& context, bool can_checkpoint) const {
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

class Function {
	bool (*function)(ParseContext&);
public:
	static constexpr bool always_succeeds() {
		return false;
	}
	constexpr Function(bool (*function)(ParseContext&)): function(function) {}
	bool parse(ParseContext& context) const {
		return function(context);
	}
};

template <class... T> class Sequence;
template <> class Sequence<> {
public:
	static constexpr bool always_succeeds() {
		return true;
	}
	constexpr Sequence() {}
	bool parse(ParseContext& context, bool can_checkpoint) const {
		return true;
	}
};
template <class T0, class... T> class Sequence<T0, T...> {
	T0 t0;
	Sequence<T...> t;
public:
	static constexpr bool always_succeeds() {
		return T0::always_succeeds() && Sequence<T...>::always_succeeds();
	}
	constexpr Sequence(T0 t0, T... t): t0(t0), t(t...) {}
	bool parse(ParseContext& context, bool can_checkpoint) const {
		const bool sequence_can_checkpoint = can_checkpoint && Sequence<T...>::always_succeeds();
		const auto save_point = context.save();
		if (!t0.parse(context, sequence_can_checkpoint)) {
			return false;
		}
		if (sequence_can_checkpoint && !context.is_before_window_end()) {
			return true;
		}
		if (!t.parse(context, sequence_can_checkpoint)) {
			context.restore(save_point);
			return false;
		}
		return true;
	}
};
template <class... T> Sequence(T...) -> Sequence<T...>;

template <class... T> class Choice;
template <> class Choice<> {
public:
	static constexpr bool always_succeeds() {
		return false;
	}
	constexpr Choice() {}
	bool parse(ParseContext& context, bool can_checkpoint) const {
		return false;
	}
};
template <class T0, class... T> class Choice<T0, T...> {
	T0 t0;
	Choice<T...> t;
public:
	static constexpr bool always_succeeds() {
		return T0::always_succeeds() || Choice<T...>::always_succeeds();
	}
	constexpr Choice(T0 t0, T... t): t0(t0), t(t...) {}
	bool parse(ParseContext& context, bool can_checkpoint) const {
		if (t0.parse(context, can_checkpoint)) {
			return true;
		}
		else {
			return t.parse(context, can_checkpoint);
		}
	}
};
template <class... T> Choice(T...) -> Choice<T...>;

template <std::size_t MIN_REPETITIONS, std::size_t MAX_REPETITIONS, class T> class Repetition {
	T t;
public:
	static constexpr bool always_succeeds() {
		return MIN_REPETITIONS == 0 || T::always_succeeds();
	}
	constexpr Repetition(T t): t(t) {}
	bool parse(ParseContext& context, bool can_checkpoint) const {
		if constexpr (MIN_REPETITIONS > 0) {
			const bool sequence_can_checkpoint = can_checkpoint && T::always_succeeds();
			const auto save_point = context.save();
			if (!t.parse(context, sequence_can_checkpoint)) {
				return false;
			}
			if (sequence_can_checkpoint && !context.is_before_window_end()) {
				return true;
			}
			for (std::size_t i = 1; i < MIN_REPETITIONS; ++i) {
				if (!t.parse(context, sequence_can_checkpoint)) {
					context.restore(save_point);
					return false;
				}
				if (sequence_can_checkpoint && !context.is_before_window_end()) {
					return true;
				}
			}
		}
		static_assert(MAX_REPETITIONS != 0 || !T::always_succeeds(), "infinite loop in grammar");
		if (can_checkpoint) {
			auto scope = context.enter_scope();
			context.skip_to_checkpoint();
			for (std::size_t i = MIN_REPETITIONS; (MAX_REPETITIONS == 0 || i < MAX_REPETITIONS) && context.is_before_window_end(); ++i) {
				context.add_checkpoint();
				if (!t.parse(context, can_checkpoint)) {
					break;
				}
			}
			context.leave_scope(scope);
		}
		else {
			for (std::size_t i = MIN_REPETITIONS; MAX_REPETITIONS == 0 || i < MAX_REPETITIONS; ++i) {
				if (!t.parse(context, can_checkpoint)) {
					break;
				}
			}
		}
		return true;
	}
};

template <class T> class Optional {
	T t;
public:
	static constexpr bool always_succeeds() {
		return true;
	}
	constexpr Optional(T t): t(t) {}
	bool parse(ParseContext& context, bool can_checkpoint) const {
		t.parse(context, can_checkpoint);
		return true;
	}
};

template <class T> class And {
	T t;
public:
	static constexpr bool always_succeeds() {
		return T::always_succeeds();
	}
	constexpr And(T t): t(t) {}
	bool parse(ParseContext& context, bool can_checkpoint) const {
		const auto save_point = context.save();
		if (t.parse(context, false)) {
			context.restore(save_point);
			return true;
		}
		else {
			return false;
		}
	}
};

template <class T> class Not {
	T t;
public:
	static constexpr bool always_succeeds() {
		return false;
	}
	constexpr Not(T t): t(t) {}
	bool parse(ParseContext& context, bool can_checkpoint) const {
		const auto save_point = context.save();
		if (t.parse(context, false)) {
			context.restore(save_point);
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
	static constexpr bool always_succeeds() {
		return T::always_succeeds();
	}
	constexpr Highlight(T t, int style): t(t), style(style) {}
	bool parse(ParseContext& context, bool can_checkpoint) const {
		const int old_style = context.change_style(style);
		const bool result = t.parse(context, can_checkpoint);
		context.change_style(old_style);
		return result;
	}
};

template <class F> class Recursive {
	F f;
public:
	constexpr Recursive(F f): f(f) {}
	bool parse(ParseContext& context) const {
		return f(*this).parse(context);
	}
};

template <class T> class Reference {
public:
	static constexpr bool always_succeeds() {
		return decltype(T::expression)::always_succeeds();
	}
	bool parse(ParseContext& context, bool can_checkpoint) const {
		return T::expression.parse(context, can_checkpoint);
	}
};

constexpr auto get_expression(char c) {
	return Char([c](char i) {
		return i == c;
	});
}
constexpr String get_expression(const char* s) {
	return String(s);
}
constexpr Function get_expression(bool (*f)(ParseContext&)) {
	return Function(f);
}
constexpr auto get_expression(bool (*f)(char)) {
	return Char(f);
}
template <class T> constexpr T get_expression(T expression) {
	return expression;
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
	return Sequence(get_expression(t)...);
}
template <class... T> constexpr auto choice(T... t) {
	return Choice(get_expression(t)...);
}
template <std::size_t MIN_REPETITIONS, std::size_t MAX_REPETITIONS, class T> constexpr Repetition<MIN_REPETITIONS, MAX_REPETITIONS, T> repetition_(T t) {
	return Repetition<MIN_REPETITIONS, MAX_REPETITIONS, T>(t);
}
template <std::size_t MIN_REPETITIONS = 0, std::size_t MAX_REPETITIONS = 0, class T> constexpr auto repetition(T t) {
	return repetition_<MIN_REPETITIONS, MAX_REPETITIONS>(get_expression(t));
}
template <class T> constexpr auto optional(T t) {
	return Optional(get_expression(t));
}
template <class T> constexpr auto and_(T t) {
	return And(get_expression(t));
}
template <class T> constexpr auto not_(T t) {
	return Not(get_expression(t));
}
template <class T> constexpr auto highlight(int style, T t) {
	return Highlight(get_expression(t), style);
}
template <class T> constexpr auto zero_or_more(T t) {
	return repetition(t);
}
template <class T> constexpr auto one_or_more(T t) {
	return repetition<1>(t);
}
template <class T> constexpr auto but(T t) {
	return sequence(not_(t), any_char());
}
constexpr auto end() {
	return not_(any_char());
}
template <class T> constexpr auto ends_with(T t) {
	const auto e = sequence(t, end());
	return sequence(repetition(but(e)), e);
}
template <class F> constexpr auto recursive(F f) {
	return Recursive(f);
}
template <class T> constexpr auto reference() {
	return Reference<T>();
}

template <class... T> constexpr auto scope(T... t) {
	return choice(t...);
}
template <class S, class E, class... T> constexpr auto nested_scope(int style, S start, E end, T... t) {
	return highlight(style, sequence(
		start,
		repetition(sequence(not_(end), choice(t..., any_char()))),
		optional(end)
	));
}
template <class T> constexpr auto root_scope(T t) {
	return repetition(choice(t, any_char()));
}

struct Language {
	const char* name;
	bool (*parse_file_name)(ParseContext&);
	bool (*parse)(ParseContext&);
};

template <class parse_file_name, class parse> constexpr Language language(const char* name) {
	return {
		name,
		[](ParseContext& context) {
			return reference<parse_file_name>().parse(context, false);
		},
		[](ParseContext& context) {
			return root_scope(reference<parse>()).parse(context, true);
		}
	};
}

constexpr auto hex_digit = choice(range('0', '9'), range('a', 'f'), range('A', 'F'));

#include "languages/c.hpp"
#include "languages/java.hpp"
#include "languages/xml.hpp"
#include "languages/javascript.hpp"
#include "languages/python.hpp"
#include "languages/rust.hpp"
#include "languages/toml.hpp"
#include "languages/haskell.hpp"

constexpr Language languages[] = {
	language<c_file_name, c_language>("C"),
	language<java_file_name, java_language>("Java"),
	language<xml_file_name, xml_language>("XML"),
	language<javascript_file_name, javascript_language>("JavaScript"),
	language<python_file_name, python_language>("Python"),
	language<rust_file_name, rust_language>("Rust"),
	language<toml_file_name, toml_language>("TOML"),
	language<haskell_file_name, haskell_language>("Haskell"),
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
	language->parse(context);
	context.change_style(Style::DEFAULT);
	return spans;
}
