#include <cstddef>
#include <vector>
#include <iostream>

class Cursor {
	const char* s;
	size_t offset;
public:
	using Snapshot = size_t;
	Cursor(const char* s): s(s), offset(0) {}
	operator bool() const {
		return s[offset] != '\0';
	}
	char operator *() const {
		return s[offset];
	}
	void advance() {
		++offset;
	}
	Snapshot save() const {
		return offset;
	}
	void restore(Snapshot snapshot) {
		offset = snapshot;
	}
	void reset() {
		offset = 0;
	}
};

enum class Command {
	DESCEND,
	DESCEND_NEXT,
	RETURN_TRUE,
	RETURN_FALSE
};

struct LanguageNode {
	virtual ~LanguageNode() = default;
	virtual const LanguageNode* get_child(size_t index) const {
		return nullptr;
	}
	virtual Command process(Command command, Cursor& cursor, size_t index) const = 0;
	virtual int get_style() const {
		return 0;
	}
};

struct Char: LanguageNode {
	char c;
	Char(char c): c(c) {}
	Command process(Command command, Cursor& cursor, size_t index) const override {
		if (cursor && *cursor == this->c) {
			cursor.advance();
			return Command::RETURN_TRUE;
		}
		return Command::RETURN_FALSE;
	}
};

struct AnyChar: LanguageNode {
	Command process(Command command, Cursor& cursor, size_t index) const override {
		if (cursor) {
			cursor.advance();
			return Command::RETURN_TRUE;
		}
		return Command::RETURN_FALSE;
	}
};

struct Range: LanguageNode {
	char first;
	char last;
	Range(char first, char last): first(first), last(last) {}
	Command process(Command command, Cursor& cursor, size_t index) const override {
		if (cursor && *cursor >= first && *cursor <= last) {
			cursor.advance();
			return Command::RETURN_TRUE;
		}
		return Command::RETURN_FALSE;
	}
};

struct Seq: LanguageNode {
	std::vector<const LanguageNode*> children;
	void add_children() {}
	template <class... T> void add_children(const LanguageNode* child, T... t) {
		children.push_back(child);
		add_children(t...);
	}
	void add_child(const LanguageNode* child) {
		children.push_back(child);
	}
	const LanguageNode* get_child(size_t index) const override {
		if (index < children.size()) {
			return children[index];
		}
		return nullptr;
	}
	Command process(Command command, Cursor& cursor, size_t index) const override {
		if (command == Command::DESCEND || command == Command::DESCEND_NEXT) {
			return Command::DESCEND;
		}
		if (command == Command::RETURN_FALSE) {
			return Command::RETURN_FALSE;
		}
		if (index + 1 == children.size()) {
			return Command::RETURN_TRUE;
		}
		return Command::DESCEND_NEXT;
	}
};

struct Choice: LanguageNode {
	std::vector<const LanguageNode*> children;
	void add_children() {}
	template <class... T> void add_children(const LanguageNode* child, T... t) {
		children.push_back(child);
		add_children(t...);
	}
	void add_child(const LanguageNode* child) {
		children.push_back(child);
	}
	const LanguageNode* get_child(size_t index) const override {
		if (index < children.size()) {
			return children[index];
		}
		return nullptr;
	}
	Command process(Command command, Cursor& cursor, size_t index) const override {
		if (command == Command::DESCEND || command == Command::DESCEND_NEXT) {
			return Command::DESCEND;
		}
		if (command == Command::RETURN_TRUE) {
			return Command::RETURN_TRUE;
		}
		if (index + 1 == children.size()) {
			return Command::RETURN_FALSE;
		}
		return Command::DESCEND_NEXT;
	}
};

struct Repeat: LanguageNode {
	const LanguageNode* child;
	Repeat(const LanguageNode* child): child(child) {}
	const LanguageNode* get_child(size_t index) const override {
		if (index == 0) {
			return child;
		}
		return nullptr;
	}
	Command process(Command command, Cursor& cursor, size_t index) const override {
		if (command == Command::DESCEND || command == Command::DESCEND_NEXT) {
			return Command::DESCEND;
		}
		if (command == Command::RETURN_FALSE) {
			return Command::RETURN_TRUE;
		}
		return Command::DESCEND;
	}
};

struct Highlight: LanguageNode {
	const LanguageNode* child;
	int style;
	Highlight(const LanguageNode* child, int style): child(child), style(style) {}
	const LanguageNode* get_child(size_t index) const override {
		if (index == 0) {
			return child;
		}
		return nullptr;
	}
	Command process(Command command, Cursor& cursor, size_t index) const override {
		if (command == Command::DESCEND || command == Command::DESCEND_NEXT) {
			return Command::DESCEND;
		}
		return command;
	}
	int get_style() const override {
		return style;
	}
};

inline const LanguageNode* get_language_node(char c) {
	return new Char(c);
}
inline const LanguageNode* get_language_node(const char* s) {
	Seq* seq = new Seq();
	for (; *s != '\0'; ++s) {
		seq->add_child(new Char(*s));
	}
	return seq;
}
inline const LanguageNode* get_language_node(const LanguageNode* language_node) {
	return language_node;
}

inline const LanguageNode* any_char() {
	return new AnyChar();
}
inline const LanguageNode* range(char first, char last) {
	return new Range(first, last);
}
template <class... T> const LanguageNode* seq(T... t) {
	Seq* seq = new Seq();
	seq->add_children(get_language_node(t)...);
	return seq;
}
template <class... T> const LanguageNode* choice(T... t) {
	Choice* choice = new Choice();
	choice->add_children(get_language_node(t)...);
	return choice;
}
template <class T> const LanguageNode* repeat(T t) {
	return new Repeat(get_language_node(t));
}
template <class T> const LanguageNode* highlight(int style, T t) {
	return new Highlight(get_language_node(t), style);
}
template <class T> const LanguageNode* highlight(T t) {
	return new Highlight(get_language_node(t), 1);
}

struct SourceNode {
	SourceNode* parent;
	const LanguageNode* language_node;
	std::vector<SourceNode*> children;
	size_t length;
	int style;
	size_t index = 0;
	Cursor::Snapshot snapshot;
	SourceNode(SourceNode* parent, const LanguageNode* language_node): parent(parent), language_node(language_node) {}
	SourceNode(const LanguageNode* language_node): parent(nullptr), language_node(language_node) {}
};

static void parse(SourceNode* node, Cursor& cursor) {
	Command command = Command::DESCEND;
	node->snapshot = cursor.save();
	SourceNode* original_parent = node->parent;
	while (true) {
		command = node->language_node->process(command, cursor, node->index);
		if (command == Command::DESCEND) {
			std::cout << "DESCEND\n";
			const LanguageNode* language_node = node->language_node->get_child(node->index);
			node = new SourceNode(node, language_node);
			node->snapshot = cursor.save();
		}
		else if (command == Command::DESCEND_NEXT) {
			std::cout << "DESCEND_NEXT\n";
			node->index += 1;
			const LanguageNode* language_node = node->language_node->get_child(node->index);
			node = new SourceNode(node, language_node);
			node->snapshot = cursor.save();
		}
		else if (command == Command::RETURN_TRUE) {
			std::cout << "RETURN_TRUE\n";
			node->length = cursor.save() - node->snapshot;
			std::cout << "  length = " << node->length << '\n';
			node->style = node->language_node->get_style();
			if (node->parent == original_parent) {
				return;
			}
			node->parent->children.push_back(node);
			node = node->parent;
		}
		else if (command == Command::RETURN_FALSE) {
			std::cout << "RETURN_FALSE\n";
			cursor.restore(node->snapshot);
			if (node->parent == original_parent) {
				return;
			}
			node = node->parent;
		}
	}
}

static void print_source_tree(SourceNode* node, Cursor& cursor, int indentation = 0) {
	for (size_t i = 0; i < indentation; ++i) std::cout << ' ';
	std::cout << "<node";
	if (true) std::cout << " length=\"" << node->length << '\"';
	if (node->style) std::cout << " style=\"" << node->style << '\"';
	std::cout << '>';
	if (node->children.empty()) {
		for (size_t i = 0; i < node->length; ++i) {
			std::cout << *cursor;
			cursor.advance();
		}
	}
	else {
		std::cout << '\n';
		for (SourceNode* child: node->children) {
			print_source_tree(child, cursor, indentation + 4);
			std::cout << '\n';
		}
		for (size_t i = 0; i < indentation; ++i) std::cout << ' ';
	}
	std::cout << "</node>";
}

static void print_source(SourceNode* node, Cursor& cursor) {
	if (node->style) std::cout << "\x1B[3" << static_cast<char>('0' + node->style) << "m";
	if (node->children.empty()) {
		for (size_t i = 0; i < node->length; ++i) {
			std::cout << *cursor;
			cursor.advance();
		}
	}
	else {
		for (SourceNode* child: node->children) {
			print_source(child, cursor);
		}
	}
	if (node->style) std::cout << "\x1B[39m";
}

int main() {
	auto language = repeat(choice(
		highlight(1, choice("let")),
		highlight(2, range('0', '9')),
		any_char()
	));
	Cursor cursor("let x = 3;");
	SourceNode* source_node = new SourceNode(language);
	parse(source_node, cursor);
	cursor.reset();
	print_source_tree(source_node, cursor);
	std::cout << '\n';
	cursor.reset();
	print_source(source_node, cursor);
	std::cout << '\n';
}
