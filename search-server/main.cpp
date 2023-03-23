#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <cassert>
#include <cstdlib>
/*
using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;

string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result;
    cin >> result;
    ReadLine();
    return result;
}

vector<string> SplitIntoWords(const string& text) {
    vector<string> words;
    string word;
    for (const char c : text) {
        if (c == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        } else {
            word += c;
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }

    return words;
}

struct Document {
    int id;
    double relevance;
    int rating;
};

enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};

class SearchServer {
public:
    void SetStopWords(const string& text) {
        for (const string& word : SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }

    void AddDocument(int document_id, const string& document, DocumentStatus status,
                     const vector<int>& ratings) {
        const vector<string> words = SplitIntoWordsNoStop(document);
        const double inv_word_count = 1.0 / words.size();
        for (const string& word : words) {
            word_to_document_freqs_[word][document_id] += inv_word_count;
        }
        documents_.emplace(document_id, DocumentData{ComputeAverageRating(ratings), status});
    }
    
    vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus status) const {
        return FindTopDocuments(raw_query, [status](int document_id, DocumentStatus stat, int rating) { return stat == status && document_id*1000 != rating; });
    }
    
    vector<Document> FindTopDocuments(const string& raw_query) const {
        return FindTopDocuments(raw_query, [](int document_id, DocumentStatus status, int rating) { return status == DocumentStatus::ACTUAL && document_id*1000 != rating; });
    }

    template <typename Keymapper>
    vector<Document> FindTopDocuments(const string& raw_query,
                                      Keymapper key_mapper) const {
        const Query query = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query, key_mapper);

        sort(matched_documents.begin(), matched_documents.end(),
             [](const Document& lhs, const Document& rhs) {
                 if (abs(lhs.relevance - rhs.relevance) < 1e-6) {
                     return lhs.rating > rhs.rating;
                 } else {
                     return lhs.relevance > rhs.relevance;
                 }
             });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }

    int GetDocumentCount() const {
        return documents_.size();
    }

    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query,
                                                        int document_id) const {
        const Query query = ParseQuery(raw_query);
        vector<string> matched_words;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.push_back(word);
            }
        }
        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.clear();
                break;
            }
        }
        return {matched_words, documents_.at(document_id).status};
    }

private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };

    set<string> stop_words_;
    map<string, map<int, double>> word_to_document_freqs_;
    map<int, DocumentData> documents_;

    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }

    vector<string> SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;
        for (const string& word : SplitIntoWords(text)) {
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }

    static int ComputeAverageRating(const vector<int>& ratings) {
        if (ratings.empty()) {
            return 0;
        }
        int rating_sum = 0;
        for (const int rating : ratings) {
            rating_sum += rating;
        }
        return rating_sum / static_cast<int>(ratings.size());
    }

    struct QueryWord {
        string data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(string text) const {
        bool is_minus = false;
        // Word shouldn't be empty
        if (text[0] == '-') {
            is_minus = true;
            text = text.substr(1);
        }
        return {text, is_minus, IsStopWord(text)};
    }

    struct Query {
        set<string> plus_words;
        set<string> minus_words;
    };

    Query ParseQuery(const string& text) const {
        Query query;
        for (const string& word : SplitIntoWords(text)) {
            const QueryWord query_word = ParseQueryWord(word);
            if (!query_word.is_stop) {
                if (query_word.is_minus) {
                    query.minus_words.insert(query_word.data);
                } else {
                    query.plus_words.insert(query_word.data);
                }
            }
        }
        return query;
    }

    // Existence required
    double ComputeWordInverseDocumentFreq(const string& word) const {
        return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
    }

    template <typename Keymapper>
    vector<Document> FindAllDocuments(const Query& query, Keymapper key_mapper) const {
        map<int, double> document_to_relevance;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);

            for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
                if (key_mapper(document_id, documents_.at(document_id).status, documents_.at(document_id).rating)) {
                    document_to_relevance[document_id] += term_freq * inverse_document_freq;
                }
            }
        }

        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
                document_to_relevance.erase(document_id);
            }
        }

        vector<Document> matched_documents;
        for (const auto [document_id, relevance] : document_to_relevance) {
            matched_documents.push_back(
                { document_id, relevance, documents_.at(document_id).rating });
        }
        return matched_documents;
    }
};

void CheckAssert(const string& file, int line, const string& func, bool v, const string& expr, const string& hint = ""s) {
    if (v == 0) {
        cout << file << "("s << line << "): "s << func << ": ASSERT("s << expr << ") failed."s;
        if (hint != ""s) {cout << " Hint: " << hint;}
        cout << endl;
        abort();
    }
}

void CheckAssertEqual(const string& file, int line, const string& func, bool v, const string& a, const string& b,  const string& hint = ""s) {
    if (v == 0) {
        cout << file << "("s << line << "): "s << func << ": ASSERT("s << a << " == "s << b << ") failed."s;
        if (hint != ""s) {cout << " Hint: " << hint;}
        cout << endl;
        abort();
    }
}

#define ASSERT(expr) CheckAssert(__FILE__, __LINE__, __FUNCTION__, expr, #expr);
#define ASSERT_HINT(expr, hint) CheckAssert(__FILE__, __LINE__, __FUNCTION__, expr, #expr, hint);
#define ASSERT_EQUAL(a, b) CheckAssertEqual(__FILE__, __LINE__, __FUNCTION__, (a == b), #a, #b);
#define ASSERT_EQUAL_HINT(a, b, hint) CheckAssertEqual(__FILE__, __LINE__, __FUNCTION__, (a == b), #a, #b, hint);
*/
// -------- Начало модульных тестов поисковой системы ----------

// Тест проверяет, что поисковая система исключает стоп-слова при добавлении документов
void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
    // Сначала убеждаемся, что поиск слова, не входящего в список стоп-слов,
    // находит нужный документ
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT(found_docs.size() == 1);
        const Document& doc0 = found_docs[0];
        ASSERT(doc0.id == doc_id);
    }

    // Затем убеждаемся, что поиск этого же слова, входящего в список стоп-слов,
    // возвращает пустой результат
    {
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT(server.FindTopDocuments("in"s).empty());
    }
}

void TestAddDocument() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
    SearchServer server;
    server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
    const auto found_docs = server.FindTopDocuments("in"s);
    ASSERT(found_docs.size() == 1);
}

void TestStopWords() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
    SearchServer server;
    server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
    server.SetStopWords("in the"s);
    server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
    ASSERT(server.FindTopDocuments("in"s).empty());
}

void TestMinusWords() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
    SearchServer server;
    server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
    const auto found_docs = server.FindTopDocuments("the -in"s);
    ASSERT(found_docs.size() == 0);
}

void TestMatching() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
    {SearchServer server;
    server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
    const auto found_docs = server.FindTopDocuments("in"s);
    ASSERT_HINT(found_docs.size() == 1, "NO"s);}
    {SearchServer server;
    server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
    const auto found_docs = server.FindTopDocuments("the -in"s);
    ASSERT_HINT(found_docs.size() == 0, "NO"s);}
}

void TestRelevance() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
    const int doc_id2 = 40;
    const string content2 = "happy dog"s;
    const vector<int> ratings2 = {10, 4, 2};
    SearchServer server;
    server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
    server.AddDocument(doc_id2, content2, DocumentStatus::ACTUAL, ratings2);
    const auto found_docs = server.FindTopDocuments("cat and dog in the city"s);
    //assert(found_docs[0].id == 42 && found_docs[1].id == 40);
    ASSERT_EQUAL(found_docs[0].id, 42);
    ASSERT_EQUAL(found_docs[1].id, 40);
}

void TestRating() {
    const int doc_id2 = 40;
    const string content2 = "happy dog"s;
    const vector<int> ratings2 = {10, 4, 10};
    SearchServer server;
    server.AddDocument(doc_id2, content2, DocumentStatus::ACTUAL, ratings2);
    const auto found_docs = server.FindTopDocuments("happy dog"s);
    ASSERT_EQUAL_HINT(found_docs[0].rating, 8, "NO"s);
}

void TestPredicate() {
    const int doc_id = 41;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
    const int doc_id2 = 40;
    const string content2 = "happy dog"s;
    const vector<int> ratings2 = {10, 4, 2};
    SearchServer server;
    server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
    server.AddDocument(doc_id2, content2, DocumentStatus::ACTUAL, ratings2);
    const auto found_docs = server.FindTopDocuments("happy dog"s, 
        [](int id, DocumentStatus status, double rating) {
            return id%2 == 0 && static_cast<int>(status)*1000!=rating;
        });
    ASSERT(found_docs.size() == 1);
}

void TestStatus() {
    const int doc_id = 41;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
    const int doc_id2 = 40;
    const string content2 = "happy dog"s;
    const vector<int> ratings2 = {10, 4, 2};
    SearchServer server;
    server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
    server.AddDocument(doc_id2, content2, DocumentStatus::BANNED, ratings2);
    const auto found_docs = server.FindTopDocuments("happy dog"s, 
        [](int id, DocumentStatus status, double rating) {
            return status == DocumentStatus::ACTUAL && id*1000 != rating;
        });
    ASSERT_EQUAL(found_docs.size(), 0);
}

/*void TF() {
    SearchServer server;
    server.SetStopWords("is are was a an in the with near at"s);
    const int doc_id = 41;
    const string content = "a colorful parrot with green wings and red tail is lost"s;
    const vector<int> ratings = {10, 4, 2};
    const int doc_id2 = 40;
    const string content2 = "a grey hound with black ears is found at the railway station"s;
    const vector<int> ratings2 = {10, 4, 2};
    const int doc_id3 = 43;
    const string content3 = "a white cat with long furry tail is found near the red square"s;
    const vector<int> ratings3 = {10, 4, 2};
    server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
    server.AddDocument(doc_id2, content2, DocumentStatus::BANNED, ratings2);
    server.AddDocument(doc_id3, content3, DocumentStatus::BANNED, ratings3);
    const auto found_docs = server.FindTopDocuments("white cat long tail"s);
    assert(found_docs[1].relevance == 0.462663 && found_docs[0].relevance == 0.0506831);*/

void TestSearchServer() {
    TestExcludeStopWordsFromAddedDocumentContent();
    TestAddDocument();
    TestMatching();
    TestRelevance();
    TestRating();
    TestPredicate();
    TestStatus();
    //TF();
    
}

// --------- Окончание модульных тестов поисковой системы -----------
/*
int main() {
    TestSearchServer();
    // Если вы видите эту строку, значит все тесты прошли успешно
    cout << "Search server testing finished"s << endl;
}
*/
