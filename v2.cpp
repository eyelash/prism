#include <vector>
#include <cstdio>
#include <iostream>

template <class T> void clear_pointer(T*& pointer) {
	if (pointer) {
		delete pointer;
		pointer = nullptr;
	}
}

template <class... T> class Tuple;
template <> class Tuple<> {
public:
	constexpr Tuple() {}
	template <class F> void for_each(F f) {}
	template <class F, class... A> auto unpack(F f, A... a) {
		return f(a...);
	}
};
template <class T0, class... T> class Tuple<T0, T...> {
	T0 t0;
	Tuple<T...> t;
public:
	constexpr Tuple(T0 t0, T... t): t0(t0), t(t...) {}
	template <class F> void for_each(F f) {
		f(t0);
		t.for_each(f);
	}
	template <class F, class... A> auto unpack(F f, A... a) {
		return t.unpack(f, a..., t0);
	}
};
template <class... T> Tuple(T...) -> Tuple<T...>;

class Node {
public:
	const char* name;
	int style;
	size_t length = 0;
	size_t read_length = 0;
	std::vector<Node*> children;
	Node(const char* name, int style = 0): name(name), style(style) {}
	virtual ~Node() {
		for (Node* child: children) {
			delete child;
		}
	}
	void append_child(Node* child) {
		length += child->length;
		children.push_back(child);
	}
	void reset() {
		length = 0;
		children.clear();
	}
};

class Tree {
	Node* root;
	static void print(Node* node, int level = 0) {
		for (int i = 0; i < level; i++) printf("  ");
		printf("%s: %zu (%zu)\n", node->name, node->length, node->read_length);
		for (Node* child: node->children) {
			print(child, level + 1);
		}
	}
public:
	Tree(Node* root): root(root) {}
	Tree(const Tree&) = delete;
	~Tree() {
		delete root;
	}
	Tree& operator =(const Tree&) = delete;
	void print() const {
		print(root);
	}
};

class StringParseContext {
	const char* s;
	size_t i = 0;
	size_t maximum = 0;
public:
	StringParseContext(const char* s): s(s) {}
	using SavePoint = size_t;
	operator bool() const {
		return s[i] != '\0';
	}
	char operator *() const {
		return s[i];
	}
	StringParseContext& operator ++() {
		++i;
		if (i > maximum) maximum = i;
		return *this;
	}
	bool get_next(char& c) {
		c = s[i];
		if (c != '\0') {
			++i;
			if (i > maximum) maximum = i;
			return true;
		}
		else {
			return false;
		}
	}
	bool get(char& c) const {
		//if (i > maximum) maximum = i;
		return (c = s[i]) != '\0';
	}
	void advance() {
		++i;
		if (i > maximum) maximum = i;
	}
	SavePoint save() const {
		return i;
	}
	SavePoint save_with_style() {
		return i;
	}
	void set_style(SavePoint save_point, int style) {}
	void restore(SavePoint save_point) {
		i = save_point;
	}
};

class Epsilon {
public:
	static constexpr bool always_succeeds = true;
	bool match(StringParseContext& c, Node* node) const {
		return true;
	}
};

template <class F> class Char {
	F f;
public:
	static constexpr bool always_succeeds = false;
	constexpr Char(F f): f(f) {}
	Node* make_node() const {
		return new Node("Char");
	}
	bool match(StringParseContext& c, Node* node) const {
		if (c && f(*c)) {
			node->length = 1;
			++c;
			return true;
		}
		return false;
	}
};

class String {
	const char* string;
public:
	static constexpr bool always_succeeds = false;
	constexpr String(const char* string): string(string) {}
	Node* make_node() const {
		return new Node("String");
	}
	bool match(StringParseContext& c, Node* node) const {
		auto save_point = c.save();
		for (const char* s = string; *s != '\0'; ++s) {
			if (c && *c == *s) {
				node->length += 1;
				++c;
			}
			else {
				c.restore(save_point);
				return false;
			}
		}
		return true;
	}
};

template <class T0, class T1> class Sequence {
	T0 t0;
	T1 t1;
public:
	static constexpr bool always_succeeds = T0::always_succeeds && T1::always_succeeds;
	constexpr Sequence(T0 t0, T1 t1): t0(t0), t1(t1) {}
	Node* make_node() const {
		return new Node("Sequence");
	}
	bool match(StringParseContext& c, Node* node) const {
		auto save_point = c.save();
		Node* child = t0.make_node();
		if (t0.match(c, child)) {
			node->append_child(child);
		}
		else {
			delete child;
			return false;
		}
		child = t1.make_node();
		if (t1.match(c, child)) {
			node->append_child(child);
		}
		else {
			delete child;
			c.restore(save_point);
			return false;
		}
		return true;
	}
};

template <class T0, class T1> class Choice {
	T0 t0;
	T1 t1;
public:
	static constexpr bool always_succeeds = T0::always_succeeds || T1::always_succeeds;
	constexpr Choice(T0 t0, T1 t1): t0(t0), t1(t1) {}
	Node* make_node() const {
		return new Node("Choice");
	}
	bool match(StringParseContext& c, Node* node) const {
		Node* child = t0.make_node();
		if (t0.match(c, child)) {
			node->append_child(child);
			return true;
		}
		delete child;
		child = t1.make_node();
		if (t1.match(c, child)) {
			node->append_child(child);
			return true;
		}
		delete child;
		return false;
	}
};

template <class T> class Repetition {
	T t;
public:
	static_assert(!T::always_succeeds);
	static constexpr bool always_succeeds = true;
	constexpr Repetition(T t): t(t) {}
	Node* make_node() const {
		return new Node("Repetition");
	}
	bool match(StringParseContext& c, Node* node) const {
		while (true) {
			Node* child = t.make_node();
			if (t.match(c, child)) {
				//node->length += child->length;
				node->append_child(child);
			}
			else {
				delete child;
				return true;
			}
		}
		//while (t.match(c, node)) {}
		//return true;
	}
};

template <class T> class Optional {
	T t;
public:
	static constexpr bool always_succeeds = true;
	constexpr Optional(T t): t(t) {}
	Node* make_node() const {
		return new Node("Optional");
	}
	bool match(StringParseContext& c, Node* node) const {
		if (t.match(c, node)) {
			return true;
		}
		else {
			return true;
		}
	}
};

template <class T> class Not {
	T t;
public:
	static constexpr bool always_succeeds = false;
	constexpr Not(T t): t(t) {}
	Node* make_node() const {
		return new Node("Not");
	}
	bool match(StringParseContext& c, Node* node) const {
		auto save_point = c.save();
		if (t.match(c, node)) {
			c.restore(save_point);
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
	static constexpr bool always_succeeds = T::always_succeeds;
	constexpr Highlight(T t, int style): t(t), style(style) {}
	Node* make_node() const {
		return new Node("Highlight", style);
	}
	bool match(StringParseContext& c, Node* node) const {
		auto save_point = c.save_with_style();
		Node* child = t.make_node();
		if (t.match(c, child)) {
			node->append_child(child);
			c.set_style(save_point, style);
			return true;
		}
		else {
			delete child;
			c.restore(save_point);
			return false;
		}
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
		return true;
	});
}
template <class T0, class T1> constexpr auto sequence(T0 t0, T1 t1) {
	return Sequence(get_language_node(t0), get_language_node(t1));
}
template <class T0, class T1, class T2, class... T> constexpr auto sequence(T0 t0, T1 t1, T2 t2, T... t) {
	return sequence(sequence(t0, t1), t2, t...);
}
template <class T0, class T1> constexpr auto choice(T0 t0, T1 t1) {
	return Choice(get_language_node(t0), get_language_node(t1));
}
template <class T0, class T1, class T2, class... T> constexpr auto choice(T0 t0, T1 t1, T2 t2, T... t) {
	return choice(choice(t0, t1), t2, t...);
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
template <class T> constexpr auto highlight(T child) {
	return Highlight(get_language_node(child), 1);
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

template <class T> Tree parse(const T& t, const char* s) {
	StringParseContext context(s);
	Node* node = t.make_node();
	t.match(context, node);
	return Tree(node);
}

constexpr auto c_syntax = repetition(choice(
	one_or_more(' '),
	highlight(choice(
		"if",
		"return"
	)),
	highlight(choice(
		"int",
		"void"
	)),
	one_or_more(range('a', 'z')),
	highlight(one_or_more(range('0', '9'))),
	highlight(sequence(
		"/*",
		repetition(sequence(not_("*/"), any_char())),
		optional("*/")
	)),
	any_char()
));

int main() {
	Tuple t(1, 2, 3.1, "hello");
	t.for_each([](auto n) {
		//std::cout << n << std::endl;
	});
	t.unpack([](auto... elements) {
		(..., (std::cout << elements << std::endl));
	});
}
