#include "prism.hpp"

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
	checkpoints.push_back({pos, max_pos});
}
const Cache::Checkpoint* Cache::Node::find_checkpoint(std::size_t pos) const {
	auto iter = std::lower_bound(checkpoints.rbegin(), checkpoints.rend(), pos, [](const Checkpoint& checkpoint, std::size_t pos) {
		return checkpoint.pos > pos;
	});
	if (iter != checkpoints.rend()) {
		return &*iter;
	}
	return nullptr;
}
Cache::Node* Cache::Node::find_child(std::size_t pos) {
	auto iter = std::lower_bound(children.begin(), children.end(), pos, [](const Node& child, std::size_t pos) {
		return child.start_pos < pos;
	});
	if (iter != children.end() && iter->start_pos == pos) {
		return &*iter;
	}
	return nullptr;
}
Cache::Node* Cache::Node::add_child(std::size_t pos, std::size_t max_pos) {
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

class Scope {
	Scope* parent_scope;
	std::size_t pos;
	std::size_t max_pos;
	Cache::Node* node;
	std::size_t get_last_checkpoint() const {
		return node ? node->get_last_checkpoint() : pos;
	}
	Cache::Node* find_child(std::size_t pos) const {
		return node ? node->find_child(pos) : nullptr;
	}
	Cache::Node* ensure_node() {
		if (node == nullptr) {
			node = parent_scope->ensure_node()->add_child(pos, max_pos);
		}
		return node;
	}
public:
	Scope(Cache::Node* node): parent_scope(nullptr), pos(0), max_pos(0), node(node) {}
	Scope(Scope* parent_scope, std::size_t pos, std::size_t max_pos): parent_scope(parent_scope), pos(pos), max_pos(max_pos), node(parent_scope->find_child(pos)) {}
	Scope* get_parent_scope() const {
		return parent_scope;
	}
	void add_checkpoint(std::size_t pos, std::size_t max_pos) {
		if (pos >= get_last_checkpoint() + 16) {
			ensure_node()->add_checkpoint(pos, max_pos);
		}
	}
	Cache::Checkpoint find_checkpoint(std::size_t pos) {
		if (node) {
			const Cache::Checkpoint* checkpoint = node->find_checkpoint(pos);
			if (checkpoint) {
				return *checkpoint;
			}
		}
		return {this->pos, this->max_pos};
	}
};

enum class Result: unsigned char {
	FAILURE,
	SUCCESS,
	PARTIAL_SUCCESS
};

class ParseContext {
	InputAdapter input;
	Range window;
	std::size_t max_pos;
	Spans spans;
	Scope* current_scope;
public:
	ParseContext(const Input* input, std::vector<Span>& spans, std::size_t window_start, std::size_t window_end): input(input), window(window_start, window_end), max_pos(0), spans(spans), current_scope(nullptr) {}
	char get() const {
		return input.get();
	}
	void advance() {
		input.advance();
	}
	int change_style(int new_style) {
		return spans.change_style(input.get_position(), new_style, window);
	}
	bool add_checkpoint() {
		current_scope->add_checkpoint(input.get_position(), std::max(max_pos, input.get_position()));
		return input.get_position() >= window.end;
	}
	void skip_to_checkpoint() {
		const auto checkpoint = current_scope->find_checkpoint(window.start);
		input.set_position(checkpoint.pos);
		max_pos = checkpoint.max_pos;
	}
	template <class F> void add_root_scope(Cache& cache, F f) {
		Scope root_scope(cache.get_root_node());
		current_scope = &root_scope;
		f();
		current_scope = nullptr;
	}
	template <class F> Result add_scope(F f) {
		Scope scope(current_scope, input.get_position(), std::max(max_pos, input.get_position()));
		current_scope = &scope;
		const Result result = f();
		current_scope = scope.get_parent_scope();
		return result;
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
	template <bool can_checkpoint> Result parse(ParseContext& context) const {
		if (!f(context.get())) {
			return Result::FAILURE;
		}
		context.advance();
		return Result::SUCCESS;
	}
};

class String {
	const char* string;
public:
	static constexpr bool always_succeeds() {
		return false;
	}
	constexpr String(const char* string): string(string) {}
	template <bool can_checkpoint> Result parse(ParseContext& context) const {
		if (*string == '\0') {
			return Result::SUCCESS;
		}
		if (context.get() != *string) {
			return Result::FAILURE;
		}
		const auto save_point = context.save();
		context.advance();
		for (const char* s = string + 1; *s != '\0'; ++s) {
			if (context.get() != *s) {
				context.restore(save_point);
				return Result::FAILURE;
			}
			context.advance();
		}
		return Result::SUCCESS;
	}
};

class CaseInsensitiveString {
	const char* string;
	static constexpr char to_lower(char c) {
		return c >= 'A' && c <= 'Z' ? c - 'A' + 'a' : c;
	}
public:
	static constexpr bool always_succeeds() {
		return false;
	}
	constexpr CaseInsensitiveString(const char* string): string(string) {}
	template <bool can_checkpoint> Result parse(ParseContext& context) const {
		const auto save_point = context.save();
		for (const char* s = string; *s != '\0'; ++s) {
			if (to_lower(context.get()) != to_lower(*s)) {
				context.restore(save_point);
				return Result::FAILURE;
			}
			context.advance();
		}
		return Result::SUCCESS;
	}
};

template <class... T> class Sequence;
template <> class Sequence<> {
public:
	static constexpr bool always_succeeds() {
		return true;
	}
	constexpr Sequence() {}
	template <bool can_checkpoint> Result parse(ParseContext& context) const {
		return Result::SUCCESS;
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
	template <bool can_checkpoint> Result parse(ParseContext& context) const {
		const auto save_point = context.save();
		const Result result = t0.template parse<can_checkpoint && Sequence<T...>::always_succeeds()>(context);
		if (result != Result::SUCCESS) {
			return result;
		}
		{
			const Result result = t.template parse<can_checkpoint>(context);
			if (result == Result::FAILURE) {
				context.restore(save_point);
			}
			return result;
		}
	}
};

template <class... T> class Choice;
template <> class Choice<> {
public:
	static constexpr bool always_succeeds() {
		return false;
	}
	constexpr Choice() {}
	template <bool can_checkpoint> Result parse(ParseContext& context) const {
		return Result::FAILURE;
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
	template <bool can_checkpoint> Result parse(ParseContext& context) const {
		const Result result = t0.template parse<can_checkpoint>(context);
		if (result != Result::FAILURE) {
			return result;
		}
		return t.template parse<can_checkpoint>(context);
	}
};

template <std::size_t MIN_REPETITIONS, std::size_t MAX_REPETITIONS, class T> class Repetition {
	T t;
public:
	static constexpr bool always_succeeds() {
		return MIN_REPETITIONS == 0 || T::always_succeeds();
	}
	constexpr Repetition(T t): t(t) {}
	template <bool can_checkpoint> Result parse(ParseContext& context) const {
		if constexpr (MIN_REPETITIONS == 1) {
			const Result result = t.template parse<can_checkpoint>(context);
			if (result != Result::SUCCESS) {
				return result;
			}
		}
		else if constexpr (MIN_REPETITIONS > 1) {
			const auto save_point = context.save();
			for (std::size_t i = 0; i < MIN_REPETITIONS; ++i) {
				const Result result = t.template parse<can_checkpoint && T::always_succeeds()>(context);
				if (result != Result::SUCCESS) {
					if (result == Result::FAILURE) {
						context.restore(save_point);
					}
					return result;
				}
			}
		}
		static_assert(MAX_REPETITIONS != 0 || !T::always_succeeds(), "infinite loop in grammar");
		if constexpr (can_checkpoint && MAX_REPETITIONS != 1) {
			return context.add_scope([&]() {
				context.skip_to_checkpoint();
				for (std::size_t i = MIN_REPETITIONS; (MAX_REPETITIONS == 0 || i < MAX_REPETITIONS); ++i) {
					const Result result = t.template parse<can_checkpoint>(context);
					if (result != Result::SUCCESS) {
						return result == Result::FAILURE ? Result::SUCCESS : result;
					}
					if (context.add_checkpoint()) {
						return Result::PARTIAL_SUCCESS;
					}
				}
				return Result::SUCCESS;
			});
		}
		else {
			for (std::size_t i = MIN_REPETITIONS; MAX_REPETITIONS == 0 || i < MAX_REPETITIONS; ++i) {
				const Result result = t.template parse<can_checkpoint>(context);
				if (result != Result::SUCCESS) {
					return result == Result::FAILURE ? Result::SUCCESS : result;
				}
			}
			return Result::SUCCESS;
		}
	}
};

template <class T> class And {
	T t;
public:
	static constexpr bool always_succeeds() {
		return T::always_succeeds();
	}
	constexpr And(T t): t(t) {}
	template <bool can_checkpoint> Result parse(ParseContext& context) const {
		const auto save_point = context.save();
		if (t.template parse<false>(context) == Result::SUCCESS) {
			context.restore(save_point);
			return Result::SUCCESS;
		}
		else {
			return Result::FAILURE;
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
	template <bool can_checkpoint> Result parse(ParseContext& context) const {
		const auto save_point = context.save();
		if (t.template parse<false>(context) == Result::SUCCESS) {
			context.restore(save_point);
			return Result::FAILURE;
		}
		else {
			return Result::SUCCESS;
		}
	}
};

template <int style, class T> class Highlight {
	T t;
public:
	static constexpr bool always_succeeds() {
		return T::always_succeeds();
	}
	constexpr Highlight(T t): t(t) {}
	template <bool can_checkpoint> Result parse(ParseContext& context) const {
		const int old_style = context.change_style(style);
		const Result result = t.template parse<can_checkpoint>(context);
		context.change_style(old_style);
		return result;
	}
};

template <class T> class Reference {
public:
	static constexpr bool always_succeeds() {
		return decltype(T::expression)::always_succeeds();
	}
	template <bool can_checkpoint> Result parse(ParseContext& context) const {
		return T::expression.template parse<can_checkpoint>(context);
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
constexpr CaseInsensitiveString case_insensitive(const char* s) {
	return CaseInsensitiveString(s);
}
template <class... T> constexpr Sequence<T...> sequence_(T... t) {
	return Sequence<T...>(t...);
}
template <class... T> constexpr auto sequence(T... t) {
	return sequence_(get_expression(t)...);
}
template <class... T> constexpr Choice<T...> choice_(T... t) {
	return Choice<T...>(t...);
}
template <class... T> constexpr auto choice(T... t) {
	return choice_(get_expression(t)...);
}
template <std::size_t MIN_REPETITIONS, std::size_t MAX_REPETITIONS, class T> constexpr Repetition<MIN_REPETITIONS, MAX_REPETITIONS, T> repetition_(T t) {
	return Repetition<MIN_REPETITIONS, MAX_REPETITIONS, T>(t);
}
template <std::size_t MIN_REPETITIONS = 0, std::size_t MAX_REPETITIONS = 0, class T> constexpr auto repetition(T t) {
	return repetition_<MIN_REPETITIONS, MAX_REPETITIONS>(get_expression(t));
}
template <class T> constexpr auto zero_or_more(T t) {
	return repetition<0>(t);
}
template <class T> constexpr auto one_or_more(T t) {
	return repetition<1>(t);
}
template <class T> constexpr auto optional(T t) {
	return repetition<0, 1>(t);
}
template <class T> constexpr auto and_(T t) {
	return And(get_expression(t));
}
template <class T> constexpr auto not_(T t) {
	return Not(get_expression(t));
}
template <int style, class T> constexpr Highlight<style, T> highlight_(T t) {
	return Highlight<style, T>(t);
}
template <int style, class T> constexpr auto highlight(T t) {
	return highlight_<style>(get_expression(t));
}
template <class T> constexpr auto any_char_but(T t) {
	return sequence(not_(t), any_char());
}
constexpr auto end() {
	return not_(any_char());
}
template <class T> constexpr auto ends_with(T t) {
	const auto e = sequence(t, end());
	return sequence(repetition(any_char_but(e)), e);
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
	void (*parse)(ParseContext&);
};

template <class parse_file_name, class parse> constexpr Language language(const char* name) {
	return {
		name,
		[](ParseContext& context) {
			return reference<parse_file_name>().template parse<false>(context) == Result::SUCCESS;
		},
		[](ParseContext& context) {
			root_scope(reference<parse>()).template parse<true>(context);
		}
	};
}

constexpr auto hex_digit = choice(range('0', '9'), range('a', 'f'), range('A', 'F'));

#include "languages/c.hpp"
#include "languages/java.hpp"
#include "languages/xml.hpp"
#include "languages/javascript.hpp"
#include "languages/json.hpp"
#include "languages/css.hpp"
#include "languages/python.hpp"
#include "languages/rust.hpp"
#include "languages/toml.hpp"
#include "languages/haskell.hpp"

constexpr Language languages[] = {
	language<c_file_name, c_language>("C"),
	language<java_file_name, java_language>("Java"),
	language<xml_file_name, xml_language>("XML"),
	language<javascript_file_name, javascript_language>("JavaScript"),
	language<json_file_name, json_language>("JSON"),
	language<css_file_name, css_language>("CSS"),
	language<python_file_name, python_language>("Python"),
	language<rust_file_name, rust_language>("Rust"),
	language<toml_file_name, toml_language>("TOML"),
	language<haskell_file_name, haskell_language>("Haskell"),
};

const Language* prism::get_language(const char* file_name) {
	StringInput input(file_name);
	std::vector<Span> spans;
	ParseContext context(&input, spans, 0, input.size());
	for (const Language& language: languages) {
		if (language.parse_file_name(context)) {
			return &language;
		}
	}
	return nullptr;
}

std::vector<Span> prism::highlight(const Language* language, const Input* input, Cache& cache, std::size_t window_start, std::size_t window_end) {
	std::vector<Span> spans;
	ParseContext context(input, spans, window_start, window_end);
	context.add_root_scope(cache, [&]() {
		language->parse(context);
	});
	context.change_style(Style::DEFAULT);
	return spans;
}
