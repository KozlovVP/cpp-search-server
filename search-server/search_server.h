#pragma once

#include <algorithm>
#include <string>
#include <vector>
#include <list>
#include <utility>
#include <set>
#include <map>
#include <stdexcept>
#include <tuple>
#include <cmath>
#include <iterator>
#include <execution>
#include "document.h"
#include "string_processing.h"
#include "log_duration.h"
#include "concurrent_map.h"

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;

class SearchServer {
public:
    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words);

    explicit SearchServer(const std::string stop_words_text)
        : SearchServer(
            SplitIntoWords(stop_words_text))  // Invoke delegating constructor from string container
    {
    }

    explicit SearchServer(const std::string_view& stop_words_text) : SearchServer(SplitIntoWords(stop_words_text))
    {
    }

    void AddDocument(int document_id, std::string_view document, DocumentStatus status, const std::vector<int>& ratings);

    void RemoveDocument(int document_id);
    void RemoveDocument(const execution::parallel_policy&, int document_id);
    void RemoveDocument(const execution::sequenced_policy&, int document_id);

    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(const std::string_view& raw_query, DocumentPredicate document_predicate) const;
    std::vector<Document> FindTopDocuments(const std::string_view& raw_query, DocumentStatus status) const;
    std::vector<Document> FindTopDocuments(const std::string_view& raw_query) const;
    
    
    //parallel 
    template <class ExecutionPolicy, typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(const ExecutionPolicy& policy, const std::string_view& raw_query, DocumentPredicate document_predicate) const;
    
    template <class ExecutionPolicy>
    std::vector<Document> FindTopDocuments(const ExecutionPolicy& policy, const std::string_view& raw_query, DocumentStatus status) const;
    
    template <class ExecutionPolicy>
    std::vector<Document> FindTopDocuments(const ExecutionPolicy& policy, const std::string_view& raw_query) const;
    //

    
    int GetDocumentCount() const;
    std::set<int>::iterator begin();
    std::set<int>::iterator end();
    const map<std::string_view, double>& GetWordFrequencies(int document_id)const;

    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const std::string_view& raw_query, int document_id) const;
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const execution::parallel_policy&, const std::string_view& raw_query, int document_id) const;
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const execution::sequenced_policy&, const std::string_view& raw_query, int document_id) const;

private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };
    const std::set<std::string, std::less<>> stop_words_;
    std::list<string> raw_documents_;
    std::map<std::string_view, std::map<int, double>> word_to_document_freqs_;
    std::map<int, std::map<std::string_view, double>> document_to_word_freqs_;
    std::map<int, DocumentData> documents_;
    std::set<int> document_ids_;

    bool IsStopWord(const std::string_view word) const;

    static bool IsValidWord(const std::string_view& word);

    std::vector<std::string_view> SplitIntoWordsNoStop(const std::string_view text) const;

    static int ComputeAverageRating(const std::vector<int>& ratings);

    struct QueryWord {
        std::string_view data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(const std::string_view& text) const;

    struct Query {
        //std::set<std::string_view> plus_words;
        //std::set<std::string_view> minus_words;
    	std::vector<std::string_view> plus_words;
    	std::vector<std::string_view> minus_words;
    };

    Query ParseQuery(const std::string_view& text) const;

    // Existence required
    double ComputeWordInverseDocumentFreq(const std::string_view& word) const;

    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const Query& query, DocumentPredicate document_predicate) const;
    
    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(std::execution::parallel_policy, const Query& query, DocumentPredicate document_predicate) const;
};

template <typename StringContainer>
SearchServer::SearchServer(const StringContainer& stop_words) : stop_words_(MakeUniqueNonEmptyStrings(stop_words))  // Extract non-empty stop words
    {
    	using namespace std;
        if (!all_of(SearchServer::stop_words_.begin(), SearchServer::stop_words_.end(), IsValidWord)) {
            throw invalid_argument("Some of stop words are invalid"s);
        }
    }

template <typename DocumentPredicate>
    std::vector<Document> SearchServer::FindTopDocuments(const std::string_view& raw_query,
                                      DocumentPredicate document_predicate) const {
    	using namespace std;
        const auto query = ParseQuery(raw_query);

        auto matched_documents = FindAllDocuments(query, document_predicate);

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

template <class ExecutionPolicy, typename DocumentPredicate>
    std::vector<Document> SearchServer::FindTopDocuments(const ExecutionPolicy& policy, const std::string_view& raw_query, DocumentPredicate document_predicate) const {
        using namespace std;
        
        if constexpr (is_same_v<ExecutionPolicy, std::execution::sequenced_policy>) {
            return FindTopDocuments(raw_query, document_predicate);
        }
        
        const auto query = ParseQuery(raw_query);

        auto matched_documents = FindAllDocuments(std::execution::par, query, document_predicate);
        
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
    
template <class ExecutionPolicy>
    std::vector<Document> SearchServer::FindTopDocuments(const ExecutionPolicy& policy, const std::string_view& raw_query, DocumentStatus status) const {
        if constexpr (is_same_v<ExecutionPolicy, std::execution::sequenced_policy>) {
            return FindTopDocuments(raw_query, status);
        }
        return SearchServer::FindTopDocuments(std::execution::par,
            raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
                return document_status == status;
            });
    }
    
    template <class ExecutionPolicy>
    std::vector<Document> SearchServer::FindTopDocuments(const ExecutionPolicy& policy, const std::string_view& raw_query) const {
        if constexpr (is_same_v<ExecutionPolicy, std::execution::sequenced_policy>) {
            return FindTopDocuments(raw_query);
        }
        return SearchServer::FindTopDocuments(std::execution::par,
            raw_query, [](int document_id, DocumentStatus document_status, int rating) {
                return document_status == DocumentStatus::ACTUAL;
            });
    }

template <typename DocumentPredicate>
    std::vector<Document> SearchServer::FindAllDocuments(const Query& query,
                                      DocumentPredicate document_predicate) const {
    	using namespace std;

        map<int, double> document_to_relevance;
        for (const string_view& word : query.plus_words) {
            if (SearchServer::word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for (const auto [document_id, term_freq] : SearchServer::word_to_document_freqs_.at(word)) {
                const auto& document_data = SearchServer::documents_.at(document_id);
                if (document_predicate(document_id, document_data.status, document_data.rating)) {
                    document_to_relevance[document_id] += term_freq * inverse_document_freq;
                }
            }
        }

        for (const string_view& word : query.minus_words) {
            if (SearchServer::word_to_document_freqs_.count(static_cast<string>(word)) == 0) {
                continue;
            }
            for (const auto [document_id, _] : SearchServer::word_to_document_freqs_.at(word)) {
                document_to_relevance.erase(document_id);
            }
        }

        vector<Document> matched_documents;
        for (const auto [document_id, relevance] : document_to_relevance) {
            matched_documents.push_back(
                {document_id, relevance, SearchServer::documents_.at(document_id).rating});
        }

        return matched_documents;
    }

template <typename DocumentPredicate>
    std::vector<Document> SearchServer::FindAllDocuments(std::execution::parallel_policy, const Query& query,
                                      DocumentPredicate document_predicate) const {
    	using namespace std;

        ConcurrentMap<int, double> document_to_relevance(100);
    
        for_each(std::execution::par,
                 query.plus_words.begin(), query.plus_words.end(),
                 [this, &document_predicate, &document_to_relevance](const auto& word) {
                     if (word_to_document_freqs_.count(static_cast<string>(word)) == 0) {
                         return;
                     }
                     const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
                     for_each(std::execution::par,
                              word_to_document_freqs_.at(word).begin(),
                              word_to_document_freqs_.at(word).end(),
                              [this, &document_predicate, &document_to_relevance, inverse_document_freq](const auto& id_freq/*[document_id, term_freq]*/) {
                                  const auto& document_data = documents_.at(id_freq.first);
                                  if (document_predicate(id_freq.first, document_data.status, document_data.rating)) {
                                      document_to_relevance[id_freq.first].ref_to_value += id_freq.second * inverse_document_freq;
                                  }
                              });
                 });
    
        for_each(std::execution::par,
                 query.minus_words.begin(), query.minus_words.end(),
                 [this, &document_to_relevance](const auto& word) {
                     if (word_to_document_freqs_.count(static_cast<string>(word)) == 0) {
                         return;
                     }
                     for_each(std::execution::par,
                              word_to_document_freqs_.at(word).begin(), word_to_document_freqs_.at(word).end(),
                              [&document_to_relevance](const auto& id_freq/*[document_id, _]*/) {
                                  document_to_relevance.Erase(id_freq.first);
                              });
                 });
    
        
        //document_to_relevance.BuildOrdinaryMap();
        vector<Document> matched_documents;
        for (const auto [document_id, relevance] : document_to_relevance.BuildOrdinaryMap()) {
            matched_documents.push_back(
                {document_id, relevance, SearchServer::documents_.at(document_id).rating});
        }

        return matched_documents;
        
    }

