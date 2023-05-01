//Вставьте сюда своё решение из урока «Очередь запросов» темы «Стек, очередь, дек».‎

#include "document.h"

Document::Document() = default;

std::ostream& operator<<(std::ostream &os, const Document& doc)
{
    using namespace std::string_literals;
    return os << "{ document_id = "s << doc.id
               << ", relevance = "s   << doc.relevance
               << ", rating = "s      << doc.rating << " }"s;
}
