#pragma once

#include <vector>
#include <memory>

class SourceNode {
	std::size_t length;
	std::vector<std::unique_ptr<SourceNode>> children;
	int style;
public:
	SourceNode(std::size_t length, int style = 0): length(length), style(style) {}
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

class LanguageNode {
public:
	virtual std::unique_ptr<SourceNode> match(const char*& c) = 0;
};

class Range: public LanguageNode {
	char first;
	char last;
public:
	Range(char first, char last): first(first), last(last) {}
	std::unique_ptr<SourceNode> match(const char*& c) override {
		if (*c >= first && *c <= last) {
			const char* begin = c;
			++c;
			return std::make_unique<SourceNode>(c - begin);
		}
		return nullptr;
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

class Repetition: public LanguageNode {
	std::unique_ptr<LanguageNode> child;
public:
	Repetition(std::unique_ptr<LanguageNode>&& child): child(std::move(child)) {}
	std::unique_ptr<SourceNode> match(const char*& c) override {
		const char* begin = c;
		std::vector<std::unique_ptr<SourceNode>> source_children;
		while (auto source_node = child->match(c)) {
			source_children.emplace_back(std::move(source_node));
		}
		return std::make_unique<SourceNode>(c - begin, std::move(source_children));
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
			std::vector<std::unique_ptr<SourceNode>> source_children;
			source_children.emplace_back(std::move(source_node));
			return std::make_unique<SourceNode>(c - begin, std::move(source_children), style);
		}
		else {
			return nullptr; 
		}
	}
};

inline std::unique_ptr<Range> range(char first, char last) {
	return std::make_unique<Range>(first, last);
}

inline std::unique_ptr<Range> character(char c) {
	return std::make_unique<Range>(c, c);
}

inline std::unique_ptr<Range> any_character() {
	return std::make_unique<Range>(1, 127);
}

inline std::unique_ptr<Sequence> string(const char* s) {
	auto result = std::make_unique<Sequence>();
	for (; *s; ++s) {
		result->add_child(character(*s));
	}
	return result;
}

template <class... A> std::unique_ptr<Sequence> sequence(A&&... children) {
	auto result = std::make_unique<Sequence>();
	(result->add_child(std::forward<A>(children)), ...);
	return result;
}

template <class... A> std::unique_ptr<Choice> choice(A&&... children) {
	auto result = std::make_unique<Choice>();
	(result->add_child(std::forward<A>(children)), ...);
	return result;
}

inline std::unique_ptr<Repetition> repetition(std::unique_ptr<LanguageNode>&& child) {
	return std::make_unique<Repetition>(std::move(child));
}

inline std::unique_ptr<Highlight> highlight(int style, std::unique_ptr<LanguageNode>&& child) {
	return std::make_unique<Highlight>(style, std::move(child));
}
