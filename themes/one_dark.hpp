constexpr Theme one_dark_theme = {
	"one-dark",
	Color::hsl(220, 13, 18), // background
	Color::hsl(220, 13, 18 + 10), // selection
	Color::hsl(220, 100, 66), // cursor
	{
		Style(Color::hsl(220, 14, 71)), // text
		Style(Color::hsl(220, 10, 40), Style::ITALIC), // comments
		Style(Color::hsl(286, 60, 67)), // keywords
		Style(Color::hsl(286, 60, 67)), // operators
		Style(Color::hsl(187, 47, 55)), // types
		Style(Color::hsl(29, 54, 61)), // literals
		Style(Color::hsl(95, 38, 62)), // strings
		Style(Color::hsl(207, 82, 66)) // function names
	}
};
