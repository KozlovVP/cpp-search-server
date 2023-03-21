#include <algorithm>
#include <iostream>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <map>
#include <cmath>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;

string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result = 0;
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
};

class SearchServer {
public:
    void SetStopWords(const string& text) {
        for (const string& word : SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }

    void AddDocument(int document_id, const string& document) {
        document_count_+=1;
        const vector<string> words = SplitIntoWordsNoStop(document);
        for (const string& word : words) {
            double tf = count(words.begin(), words.end(), word)*1./words.size(); 
            documents_[word].insert({document_id, tf});
        }
    }

    vector<Document> FindTopDocuments(const string& raw_query) const {
        const Query query_words = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query_words);

        sort(matched_documents.begin(), matched_documents.end(),
             [](const Document& lhs, const Document& rhs) {
                 return lhs.relevance > rhs.relevance;
             });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }

private:
    struct Query {
        set<string> minus;
        set<string> plus;
    };
    int document_count_ = 0;
    map<string, map<int, double>> documents_;

    set<string> stop_words_;

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

    Query ParseQuery(const string& text) const {
        Query query_words;
        for (const string& word : SplitIntoWordsNoStop(text)) {
            if (word[0] == '-') {query_words.minus.insert(word.substr(1));}
            else {query_words.plus.insert(word);}
        }
        return query_words;
    }

    vector<Document> FindAllDocuments(const Query& query_words) const {
        vector<Document> matched_documents;
        map<int, double> document_to_relevance;
        if (documents_.size() == 0) {return matched_documents;}
        for (const string& plus_word : query_words.plus) {
            if(documents_.count(plus_word) == 0) {continue;}
            double idf = log(document_count_*1.0/(documents_.at(plus_word).size()*1.));
            for (const auto& [id, tf] : documents_.at(plus_word)) {
                document_to_relevance[id]+=tf*idf;
            }
        }
        for (const string& minus_word : query_words.minus) {
            if (documents_.count(minus_word) > 0) {
                for (const auto& [id, tf] : documents_.at(minus_word)) {
                    document_to_relevance.erase(id);
                }}}
        for (const auto& [id, coun] : document_to_relevance) {
            matched_documents.push_back({id, coun});
        }
        return matched_documents;
    }
            
    /*vector<Document> FindAllDocuments(const Query& query_words) const {
        vector<Document> matched_documents;
        map<int, int> document_to_relevance;
        for (const string& plus_word : query_words.plus) {
            if (documents_.count(plus_word) > 0) {
                for (int i : documents_.at(plus_word)) {
                    document_to_relevance[i]+=1;}}}
        for (const string& minus_word : query_words.minus) {
            if (documents_.count(minus_word) > 0) {
                for (int i : documents_.at(minus_word)) {
                    document_to_relevance.erase(i);
                }}}                     
             
        for (const auto& document : documents_) {
            const int relevance = MatchDocument(document, query_words);
            if (relevance > 0) {
                matched_documents.push_back({document.id, relevance});
            }
        }
        for (const auto& [id, coun] : document_to_relevance) {
            matched_documents.push_back({id, coun});
        }
        return matched_documents;
    }*/

    /*static int MatchDocument(const DocumentContent& content, const Query& query_words) {
        if (query_words.plus.empty()) {return 0;}
        for (const string& word : content.words) {
            if (query_words.minus.count(word) > 0) {return 0;}}
        set<string> matched_words;
        for (const string& word : content.words) {
            if (matched_words.count(word) != 0) {
                continue;
            }
            if (query_words.plus.count(word) != 0) {
                matched_words.insert(word);
            }
        }
        return static_cast<int>(matched_words.size());
    }*/
};

SearchServer CreateSearchServer() {
    SearchServer search_server;
    search_server.SetStopWords(ReadLine());

    const int document_count = ReadLineWithNumber();
    for (int document_id = 0; document_id < document_count; ++document_id) {
        search_server.AddDocument(document_id, ReadLine());
    }

    return search_server;
}

int main() {
    const SearchServer search_server = CreateSearchServer();

    const string query = ReadLine();
    if (search_server.FindTopDocuments(query).size() != 0) {
        for (const auto& [document_id, relevance] : search_server.FindTopDocuments(query)) {   
            cout << "{ document_id = "s << document_id << ", "
             << "relevance = "s << relevance << " }"s << endl;
    }}
}
