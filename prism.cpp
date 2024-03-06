#include "prism.hpp"
#include <memory>
#include <map>

#include "themes/one_dark.hpp"
#include "themes/monokai.hpp"

constexpr std::initializer_list<Theme> themes = {
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

void prism::Tree::add_checkpoint(std::size_t pos, std::size_t max_pos) {
	if (checkpoints.empty() || pos > checkpoints.back().pos) {
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

template <class F> class Char {
	F f;
public:
	constexpr Char(F f): f(f) {}
	bool parse(ParseContext& context) const {
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
	constexpr String(const char* string): string(string) {}
	bool parse(ParseContext& context) const {
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
	constexpr Function(bool (*function)(ParseContext&)): function(function) {}
	bool parse(ParseContext& context) const {
		return function(context);
	}
};

template <class... T> class Tuple;
template <> class Tuple<> {
public:
	constexpr Tuple() {}
	bool parse_sequence(ParseContext&) const {
		return true;
	}
	bool parse_choice(ParseContext&) const {
		return false;
	}
};
template <class T0, class... T> class Tuple<T0, T...> {
	T0 t0;
	Tuple<T...> t;
public:
	constexpr Tuple(T0 t0, T... t): t0(t0), t(t...) {}
	bool parse_sequence(ParseContext& context) const {
		const auto save_point = context.save();
		if (t0.parse(context)) {
			if (t.parse_sequence(context)) {
				return true;
			}
			else {
				context.restore(save_point);
				return false;
			}
		}
		else {
			return false;
		}
	}
	bool parse_choice(ParseContext& context) const {
		if (t0.parse(context)) {
			return true;
		}
		else {
			return t.parse_choice(context);
		}
	}
};

template <class... T> class Sequence {
	Tuple<T...> t;
public:
	constexpr Sequence(T... t): t(t...) {}
	bool parse(ParseContext& context) const {
		return t.parse_sequence(context);
	}
};

template <class... T> class Choice {
	Tuple<T...> t;
public:
	constexpr Choice(T... t): t(t...) {}
	bool parse(ParseContext& context) const {
		return t.parse_choice(context);
	}
};

template <class T> class Repetition {
	T t;
public:
	constexpr Repetition(T t): t(t) {}
	bool parse(ParseContext& context) const {
		while (t.parse(context)) {}
		return true;
	}
};

template <class T> class Optional {
	T t;
public:
	constexpr Optional(T t): t(t) {}
	bool parse(ParseContext& context) const {
		t.parse(context);
		return true;
	}
};

template <class T> class Not {
	T t;
public:
	constexpr Not(T t): t(t) {}
	bool parse(ParseContext& context) const {
		const auto save_point = context.save();
		if (t.parse(context)) {
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
	constexpr Highlight(T t, int style): t(t), style(style) {}
	bool parse(ParseContext& context) const {
		const int old_style = context.change_style(style);
		const bool result = t.parse(context);
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

constexpr auto get_language_node(char c) {
	return Char([c](char i) {
		return i == c;
	});
}
constexpr String get_language_node(const char* s) {
	return String(s);
}
constexpr Function get_language_node(bool (*f)(ParseContext&)) {
	return Function(f);
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
template <class F> constexpr auto recursive(F f) {
	return Recursive(f);
}

class ScopeWrapper {
	class Interface {
	public:
		virtual ~Interface() = default;
		virtual bool parse(ParseContext&) const = 0;
	};
	template <class T> class Implementation final: public Interface {
		T t;
	public:
		constexpr Implementation(T t): t(t) {}
		bool parse(ParseContext& context) const override {
			return t.parse(context);
		}
	};
	std::unique_ptr<Interface> scope;
public:
	ScopeWrapper() {}
	template <class T> ScopeWrapper(T t): scope(std::make_unique<Implementation<T>>(t)) {}
	bool parse(ParseContext& context) const {
		return scope->parse(context);
	}
};

static std::map<StringView, ScopeWrapper> scopes;

template <class... T> class Scope {
	Tuple<T...> tuple;
public:
	constexpr Scope(T... t): tuple(t...) {}
	bool parse(ParseContext& context) const {
		return tuple.parse_choice(context);
	}
};

template <class S, class E, class... T> class NestedScope {
	S start;
	Tuple<T...> t;
	E end;
	bool parse_single(ParseContext& context) const {
		if (!not_(end).parse(context)) {
			return false;
		}
		if (t.parse_choice(context)) {
			return true;
		}
		else {
			return any_char().parse(context);
		}
	}
public:
	constexpr NestedScope(S start, E end, T... t): start(start), t(t...), end(end) {}
	bool parse(ParseContext& context) const {
		if (!start.parse(context)) {
			return false;
		}
		while (parse_single(context)) {}
		end.parse(context);
		return true;
	}
};

class ScopeReference {
	const char* name;
public:
	constexpr ScopeReference(const char* name): name(name) {}
	bool parse(ParseContext& context) const {
		return scopes[name].parse(context);
	}
};

class RootScope {
	const char* name;
	static bool parse_single(ParseContext& context, const ScopeWrapper& scope) {
		if (scope.parse(context)) {
			return true;
		}
		else {
			return any_char().parse(context);
		}
	}
public:
	constexpr RootScope(const char* name): name(name) {}
	bool parse(ParseContext& context) const {
		const ScopeWrapper& scope = scopes[name];
		context.skip_to_checkpoint();
		while (context.is_before_window_end() && parse_single(context, scope)) {
			context.add_checkpoint();
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

struct Language {
	const char* scope;
	bool (*match)(const StringView&);
	void (*init)();
};

constexpr auto hex_digit = choice(range('0', '9'), range('a', 'f'), range('A', 'F'));

#include "languages/c.hpp"
#include "languages/python.hpp"

constexpr std::initializer_list<Language> languages = {
	c_language,
	python_language,
};

static void initialize() {
	for (const Language& language: languages) {
		language.init();
	}
}

const char* prism::get_language(const char* file_name) {
	for (const Language& language: languages) {
		if (language.match(file_name)) {
			return language.scope;
		}
	}
	return nullptr;
}

std::vector<Span> prism::highlight(const char* language, const Input* input, Tree& tree, std::size_t window_start, std::size_t window_end) {
	if (scopes.empty()) {
		initialize();
	}
	std::vector<Span> spans;
	ParseContext context(input, tree, spans, window_start, window_end);
	root_scope(language).parse(context);
	context.change_style(Style::DEFAULT);
	return spans;
}
