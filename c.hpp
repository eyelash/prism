#pragma once

#include "prism.hpp"

template <class A> std::unique_ptr<LanguageNode> any_character_but(A&& child) {
	return sequence(not_(std::forward<A>(child)), any_character());
}

class CLanguage {
	std::unique_ptr<LanguageNode> identifier_begin_character = choice(range('a', 'z'), range('A', 'Z'), '_');
	std::unique_ptr<LanguageNode> identifier_character = choice(range('a', 'z'), range('A', 'Z'), '_', range('0', '9'));
	std::unique_ptr<LanguageNode> hex_digit = choice(range('0', '9'), range('a', 'f'), range('A', 'F'));
	std::unique_ptr<LanguageNode> escape = sequence('\\', any_character());
	std::unique_ptr<LanguageNode> keyword(const char* s) {
		return sequence(s, not_(reference(identifier_character)));
	}
public:
	std::unique_ptr<LanguageNode> language = repetition(choice(
		// comments
		highlight(4, choice(
			sequence("/*", repetition(any_character_but("*/")), optional("*/")),
			sequence("//", repetition(any_character_but('\n')))
		)),
		// strings
		highlight(3, sequence('"', repetition(choice(reference(escape), any_character_but('"'))), optional('"'))),
		highlight(3, sequence('\'', choice(reference(escape), any_character_but('\'')), '\'')),
		// numbers
		highlight(3, sequence(
			choice(
				// hex
				sequence(
					'0',
					choice('x', 'X'),
					zero_or_more(reference(hex_digit)),
					optional('.'),
					zero_or_more(reference(hex_digit)),
					// exponent
					optional(sequence(
						choice('p', 'P'),
						optional(choice('+', '-')),
						one_or_more(range('0', '9'))
					))
				),
				// decimal
				sequence(
					choice(
						sequence(one_or_more(range('0', '9')), optional('.'), zero_or_more(range('0', '9'))),
						sequence('.', one_or_more(range('0', '9')))
					),
					// exponent
					optional(sequence(
						choice('e', 'E'),
						optional(choice('+', '-')),
						one_or_more(range('0', '9'))
					))
				)
			),
			// suffix
			zero_or_more(choice('u', 'U', 'l', 'L', 'f', 'F'))
		)),
		highlight(3, one_or_more(range('0', '9'))),
		// keywords
		highlight(1, choice(
			keyword("if"),
			keyword("else"),
			keyword("for"),
			keyword("while"),
			keyword("do"),
			keyword("switch"),
			keyword("case"),
			keyword("default"),
			keyword("goto"),
			keyword("break"),
			keyword("continue"),
			keyword("return"),
			keyword("struct"),
			keyword("enum"),
			keyword("union"),
			keyword("typedef"),
			keyword("sizeof"),
			keyword("static")
		)),
		// types
		highlight(2, choice(
			keyword("void"),
			keyword("char"),
			keyword("short"),
			keyword("int"),
			keyword("long"),
			keyword("float"),
			keyword("double"),
			keyword("unsigned"),
			keyword("signed")
		)),
		// preprocessor
		highlight(1, sequence('#', repetition(reference(identifier_character)))),
		// identifiers
		sequence(reference(identifier_begin_character), repetition(reference(identifier_character))),
		any_character()
	));
};
