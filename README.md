prism 🌈️
--------

An incremental [PEG](https://en.wikipedia.org/wiki/Parsing_expression_grammar)-based syntax highlighting engine written in C++.

```cpp
#include <prism.hpp>
#include <cassert>

int main() {
    const StringInput input = "int main() { return 42; }";
    const Language* language = prism::get_language("file.c");
    Cache cache;
    auto spans = prism::highlight(language, &input, cache, 0, input.size());
    
    const std::vector<Span> expected = {
        {0, 3, Style::TYPE},
        {13, 19, Style::KEYWORD},
        {20, 22, Style::LITERAL},
    };
    assert(spans == expected);
}
```
