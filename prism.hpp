#pragma once

#include <memory>
#include <vector>
#include <array>

class SourceNode {
	std::size_t length;
	std::vector<std::unique_ptr<SourceNode>> children;
	int style;
public:
	SourceNode(std::size_t length, int style = 0): length(length), style(style) {}
	SourceNode(std::size_t length, std::unique_ptr<SourceNode>&& child, int style = 0): length(length), style(style) {
		children.emplace_back(std::move(child));
	}
	SourceNode(std::size_t length, std::vector<std::unique_ptr<SourceNode>>&& children, int style = 0): length(length), children(std::move(children)), style(style) {}
	std::size_t get_length() const {
		return length;
	}
	const std::vector<std::unique_ptr<SourceNode>>& get_children() const {
		return children;
	}
	int get_style() const {
		return style;
	}
};

/*class SourceLeaf: public SourceNode {
public:
	void accept(SourceNodeVisitor& visitor) override {
		visitor.visit_leaf(*this);
	}
};*/

/*class SourceINode: public SourceNode {
	std::vector<std::unique_ptr<SourceNode>> children;
public:
	void accept(SourceNodeVisitor& visitor) override {
		visitor.visit_i_node(*this);
	}
	const std::vector<std::unique_ptr<SourceNode>>& get_children() const {
		return children;
	}
};*/

class LanguageNode {
public:
	virtual std::unique_ptr<SourceNode> match(const char*& c) = 0;
};

class CharacterClass: public LanguageNode {
	std::array<unsigned char, 256 / 8> characters;
public:
	CharacterClass() {
		for (unsigned int i = 0; i < 256 / 8; ++i) {
			characters[i] = 0;
		}
	}
	void add_character(unsigned char c) {
		const unsigned int byte_index = c / 8;
		const unsigned int bit_index = c % 8;
		characters[byte_index] |= 1 << bit_index;
	}
	void add_range(unsigned char first, unsigned char last) {
		for (unsigned int i = first; i <= last; ++i) {
			add_character(i);
		}
	}
	void invert() {
		for (unsigned int i = 0; i < 256 / 8; ++i) {
			characters[i] = ~characters[i];
		}
	}
	bool check(unsigned char c) const {
		const unsigned int byte_index = c / 8;
		const unsigned int bit_index = c % 8;
		return characters[byte_index] & 1 << bit_index;
	}
	std::unique_ptr<SourceNode> match(const char*& c) override {
		const char* begin = c;
		if (*c && check(*c)) {
			++c;
			return std::make_unique<SourceNode>(c - begin);
		}
		return nullptr;
	}
};

class Range: public LanguageNode {
	char first;
	char last;
public:
	Range(char first, char last): first(first), last(last) {}
	std::unique_ptr<SourceNode> match(const char*& c) override {
		const char* begin = c;
		if (*c && *c >= first && *c <= last) {
			++c;
			return std::make_unique<SourceNode>(c - begin);
		}
		return nullptr;
	}
};

class Character: public LanguageNode {
	char c;
public:
	Character(char c): c(c) {}
	std::unique_ptr<SourceNode> match(const char*& c) override {
		const char* begin = c;
		if (*c && *c == this->c) {
			++c;
			return std::make_unique<SourceNode>(c - begin);
		}
		return nullptr;
	}
};

class String: public LanguageNode {
	const char* s;
public:
	String(const char* s): s(s) {}
	std::unique_ptr<SourceNode> match(const char*& c) override {
		const char* begin = c;
		for (const char* i = s; *i; ++i) {
			if (*c && *c == *i) {
				++c;
			}
			else {
				c = begin;
				return nullptr;
			}
		}
		return std::make_unique<SourceNode>(c - begin);
		/*std::vector<std::unique_ptr<SourceNode>> source_children;
		for (auto& child: children) {
			if (auto source_node = child->match(c)) {
				source_children.emplace_back(std::move(source_node));
			}
			else {
				c = begin;
				return nullptr;
			}
		}
		return std::make_unique<SourceNode>(c - begin, std::move(source_children));
		const char* begin = c;
		if (*c && *c >= first && *c <= last) {
			++c;
			return std::make_unique<SourceNode>(c - begin);
		}
		return nullptr;*/
	}
};

class Sequence: public LanguageNode {
	std::vector<std::unique_ptr<LanguageNode>> children;
public:
	void add_child(std::unique_ptr<LanguageNode>&& child) {
		children.push_back(std::move(child));
	}
	std::unique_ptr<SourceNode> match(const char*& c) override {
		const char* begin = c;
		std::vector<std::unique_ptr<SourceNode>> source_children;
		for (auto& child: children) {
			if (auto source_node = child->match(c)) {
				source_children.emplace_back(std::move(source_node));
			}
			else {
				c = begin;
				return nullptr;
			}
		}
		return std::make_unique<SourceNode>(c - begin, std::move(source_children));
	}
};

class Choice: public LanguageNode {
	std::vector<std::unique_ptr<LanguageNode>> children;
public:
	void add_child(std::unique_ptr<LanguageNode>&& child) {
		children.push_back(std::move(child));
	}
	std::unique_ptr<SourceNode> match(const char*& c) override {
		for (auto& child: children) {
			if (auto source_node = child->match(c)) {
				return source_node;
			}
		}
		return nullptr;
	}
};

/*class Optional: public LanguageNode {
	std::unique_ptr<LanguageNode> child;
public:
	Optional(std::unique_ptr<LanguageNode>&& child): child(std::move(child)) {}
	std::unique_ptr<SourceNode> match(const char*& c) override {
		const char* begin = c;
		std::vector<std::unique_ptr<SourceNode>> source_children;
		if (auto source_node = child->match(c)) {
			source_children.emplace_back(std::move(source_node));
			return std::make_unique<SourceNode>(c - begin, std::move(source_node)); 
		}
		return std::make_unique<SourceNode>(0);
	}
};*/

class Repetition: public LanguageNode {
	std::unique_ptr<LanguageNode> child;
	unsigned int min_repetitions;
	unsigned int max_repetitions;
public:
	Repetition(std::unique_ptr<LanguageNode>&& child, unsigned int min_repetitions = 0, unsigned int max_repetitions = -1): child(std::move(child)), min_repetitions(min_repetitions), max_repetitions(max_repetitions) {}
	std::unique_ptr<SourceNode> match(const char*& c) override {
		const char* begin = c;
		std::vector<std::unique_ptr<SourceNode>> source_children;
		for (unsigned int i = 0; i < min_repetitions; ++i) {
			if (auto source_node = child->match(c)) {
				source_children.emplace_back(std::move(source_node));
			}
			else {
				c = begin;
				return nullptr;
			}
		}
		for (unsigned int i = min_repetitions; i < max_repetitions; ++i) {
			if (auto source_node = child->match(c)) {
				source_children.emplace_back(std::move(source_node));
			}
			else {
				break;
			}
		}
		return std::make_unique<SourceNode>(c - begin, std::move(source_children));
	}
};

class Not: public LanguageNode {
	std::unique_ptr<LanguageNode> child;
public:
	Not(std::unique_ptr<LanguageNode>&& child): child(std::move(child)) {}
	std::unique_ptr<SourceNode> match(const char*& c) override {
		const char* begin = c;
		if (child->match(c)) {
			c = begin;
			return nullptr;
		}
		else {
			return std::make_unique<SourceNode>(0);
		}
	}
};

class Reference: public LanguageNode {
	std::unique_ptr<LanguageNode>& node;
public:
	Reference(std::unique_ptr<LanguageNode>& node): node(node) {}
	std::unique_ptr<SourceNode> match(const char*& c) override {
		return node->match(c);
	}
};

class Highlight: public LanguageNode {
	int style;
	std::unique_ptr<LanguageNode> child;
public:
	Highlight(int style, std::unique_ptr<LanguageNode>&& child): style(style), child(std::move(child)) {}
	std::unique_ptr<SourceNode> match(const char*& c) override {
		const char* begin = c;
		if (auto source_node = child->match(c)) {
			return std::make_unique<SourceNode>(c - begin, std::move(source_node), style);
		}
		else {
			return nullptr; 
		}
	}
};

inline auto invert = [](CharacterClass& cc) {
	cc.invert();
};

inline auto add_character(char c) {
	return [=](CharacterClass& cc) {
		cc.add_character(c);
	};
}

inline auto add_range(char first, char last) {
	return [=](CharacterClass& cc) {
		cc.add_range(first, last);
	};
}

template <class... A> std::unique_ptr<LanguageNode> character_class(A... arguments) {
	auto result = std::make_unique<CharacterClass>();
	(arguments(*result), ...);
	return result;
}

inline std::unique_ptr<LanguageNode> character(char c) {
	return std::make_unique<Range>(c, c);
}

inline std::unique_ptr<LanguageNode> range(char first, char last) {
	auto result = std::make_unique<CharacterClass>();
	result->add_range(first, last);
	return result;
}

inline std::unique_ptr<LanguageNode> any_character() {
	auto result = std::make_unique<CharacterClass>();
	result->invert();
	return result;
}

inline std::unique_ptr<LanguageNode> string(const char* s) {
	//return std::make_unique<String>(s);
	auto result = std::make_unique<Sequence>();
	for (; *s; ++s) {
		result->add_child(character(*s));
	}
	return result;
}

inline std::unique_ptr<LanguageNode> get_language_node(char c) {
	return character(c);
}

inline std::unique_ptr<LanguageNode> get_language_node(const char* s) {
	return string(s);
}

inline std::unique_ptr<LanguageNode> get_language_node(std::unique_ptr<LanguageNode>&& language_node) {
	return std::move(language_node);
}

template <class... A> std::unique_ptr<LanguageNode> sequence(A&&... children) {
	auto result = std::make_unique<Sequence>();
	(result->add_child(get_language_node(std::forward<A>(children))), ...);
	return result;
}

template <class... A> std::unique_ptr<LanguageNode> choice(A&&... children) {
	auto result = std::make_unique<Choice>();
	(result->add_child(get_language_node(std::forward<A>(children))), ...);
	return result;
}

template <class A> std::unique_ptr<LanguageNode> optional(A&& child) {
	return std::make_unique<Repetition>(get_language_node(std::forward<A>(child)), 0, 1);
}

template <class A> std::unique_ptr<LanguageNode> zero_or_more(A&& child) {
	return std::make_unique<Repetition>(get_language_node(std::forward<A>(child)), 0);
}

template <class A> std::unique_ptr<LanguageNode> one_or_more(A&& child) {
	return std::make_unique<Repetition>(get_language_node(std::forward<A>(child)), 1);
}

template <class A> std::unique_ptr<LanguageNode> repetition(A&& child) {
	return std::make_unique<Repetition>(get_language_node(std::forward<A>(child)));
}

template <class A> std::unique_ptr<LanguageNode> not_(A&& child) {
	return std::make_unique<Not>(get_language_node(std::forward<A>(child)));
}

inline std::unique_ptr<LanguageNode> reference(std::unique_ptr<LanguageNode>& node) {
	return std::make_unique<Reference>(node);
}

template <class A> std::unique_ptr<LanguageNode> highlight(int style, A&& child) {
	return std::make_unique<Highlight>(style, get_language_node(std::forward<A>(child)));
}
