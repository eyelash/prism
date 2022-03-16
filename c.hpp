#pragma once

#include "prism.hpp"

inline std::unique_ptr<LanguageNode> any_character_but(std::unique_ptr<LanguageNode>&& child) {
	return sequence(not_(std::move(child)), any_character());
}

class CLanguage {
	std::unique_ptr<LanguageNode> identifier_begin_character = choice(range('a', 'z'), range('A', 'Z'), character('_'));
	std::unique_ptr<LanguageNode> identifier_character = choice(range('a', 'z'), range('A', 'Z'), character('_'), range('0', '9'));
	std::unique_ptr<LanguageNode> hex_digit = choice(range('0', '9'), range('a', 'f'), range('A', 'F'));
	std::unique_ptr<LanguageNode> escape = sequence(character('\\'), any_character());
	std::unique_ptr<LanguageNode> keyword(const char* s) {
		return sequence(string(s), not_(reference(identifier_character)));
	}
public:
	std::unique_ptr<LanguageNode> language = repetition(choice(
		// comments
		highlight(4, choice(
			sequence(string("/*"), repetition(any_character_but(string("*/"))), optional(string("*/"))),
			sequence(string("//"), repetition(any_character_but(character('\n'))))
		)),
		// strings
		highlight(3, sequence(character('"'), repetition(choice(reference(escape), any_character_but(character('"')))), optional(character('"')))),
		highlight(3, sequence(character('\''), choice(reference(escape), any_character_but(character('\''))), character('\''))),
		// numbers
		highlight(3, sequence(
			choice(
				// hex
				sequence(
					character('0'),
					choice(character('x'), character('X')),
					zero_or_more(reference(hex_digit)),
					optional(character('.')),
					zero_or_more(reference(hex_digit)),
					// exponent
					optional(sequence(
						choice(character('p'), character('P')),
						optional(choice(character('+'), character('-'))),
						one_or_more(range('0', '9'))
					))
				),
				// decimal
				sequence(
					choice(
						sequence(one_or_more(range('0', '9')), optional(character('.')), zero_or_more(range('0', '9'))),
						sequence(character('.'), one_or_more(range('0', '9')))
					),
					// exponent
					optional(sequence(
						choice(character('e'), character('E')),
						optional(choice(character('+'), character('-'))),
						one_or_more(range('0', '9'))
					))
				)
			),
			// suffix
			zero_or_more(choice(character('u'), character('U'), character('l'), character('L'), character('f'), character('F')))
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
		highlight(1, sequence(character('#'), repetition(reference(identifier_character)))),
		// identifiers
		sequence(reference(identifier_begin_character), repetition(reference(identifier_character))),
		any_character()
	));
};
