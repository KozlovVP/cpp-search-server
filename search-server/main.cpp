//Вставьте сюда своё решение из урока «Очередь запросов» темы «Стек, очередь, дек».‎

#include "document.h"
#include "paginator.h"
#include "read_input_functions.h"
#include "request_queue.h"
#include "search_server.h"
#include "string_processing.h"

template <typename Iterator>
ostream& operator<<(ostream &os, IteratorRange<Iterator> page) 
{
    for (auto i = page.begin(); i < page.end(); i++)
    {
        cout << *i;
    }
    return os;
}

ostream& operator<<(ostream &os, const Document& doc)
{
    return os << "{ document_id = " << doc.id
               << ", relevance = "   << doc.relevance
               << ", rating = "      << doc.rating << " }";
}
