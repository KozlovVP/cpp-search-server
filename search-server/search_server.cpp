//Вставьте сюда своё решение из урока «Очередь запросов» темы «Стек, очередь, дек».‎

#include "search_server.h"

SearchServer::SearchServer(const std::string& stop_words_text)
        : SearchServer(
            SplitIntoWords(stop_words_text))  // Invoke delegating constructor from string container
    {
    }

int SearchServer::ComputeAverageRating(const std::vector<int>& ratings) {
        if (ratings.empty()) {
            return 0;
        }
        int rating_sum = 0;
        for (const int rating : ratings) {
            rating_sum += rating;
        }
        return rating_sum / static_cast<int>(ratings.size());
    }

void SearchServer::AddDocument(int document_id, const std::string& document, DocumentStatus status,
                     const std::vector<int>& ratings) {
        if ((document_id < 0) || (documents_.count(document_id) > 0)) {
            using namespace std::string_literals;
            throw std::invalid_argument("Invalid document_id"s);
        }
        const auto words = SplitIntoWordsNoStop(document);
        
        
        const double inv_word_count = 1.0 / words.size();
        std::set<std::string> set_of_words;
        for (const std::string& word : words) {
            set_of_words.insert(word);
            word_to_document_freqs_[word][document_id] += inv_word_count;
            document_id_to_words_freq_[document_id][word] += inv_word_count;
        }
        documents_.emplace(document_id, DocumentData{ComputeAverageRating(ratings), status});
        document_ids_.insert(document_id);
        set_of_words_id_[set_of_words].insert(document_id);
    }

std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query, DocumentStatus status) const {
        return FindTopDocuments(
            raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
                return document_status == status;
            });
    }

std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query) const {
        return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
    }

int SearchServer::GetDocumentCount() const {
        return documents_.size();
    }

/*int SearchServer::GetDocumentId(int index) const {
        return document_ids_.at(index);
    }*/


std::tuple<std::vector<std::string>, DocumentStatus> SearchServer::MatchDocument(const std::string& raw_query, int document_id) const {
        const auto query = ParseQuery(raw_query);

        std::vector<std::string> matched_words;
        for (const std::string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.push_back(word);
            }
        }
        for (const std::string& word : query.minus_words) {
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

    std::set<int>::const_iterator SearchServer::begin() const
    {
        return document_ids_.begin();
    }
    std::set<int>::const_iterator SearchServer::end() const
    {
        return document_ids_.end();
    }

    const std::map<std::string, double>& SearchServer::GetWordFrequencies(int document_id) const {
        if (document_id_to_words_freq_.count(document_id) == 1) 
        {
            return document_id_to_words_freq_.at(document_id);
        }
        static std::map<std::string, double> empty_result;
        return empty_result;
    }

    void SearchServer::RemoveDocument(int document_id) {
        if (document_ids_.count(document_id) == 0) {return;}
        for (const auto& [word, id_freq] : document_id_to_words_freq_.at(document_id)) {
            word_to_document_freqs_.at(word).erase(document_id);
        }
        documents_.erase(document_id);
        auto it = find(document_ids_.begin(), document_ids_.end(), document_id);
        document_ids_.erase(it);
        document_id_to_words_freq_.erase(document_id);
    }

bool SearchServer::IsStopWord(const std::string& word) const {
        return stop_words_.count(word) > 0;
    }

bool SearchServer::IsValidWord(const std::string& word) {
        // A valid word must not contain special characters
        return none_of(word.begin(), word.end(), [](char c) {
            return c >= '\0' && c < ' ';
        });
    }

std::vector<std::string> SearchServer::SplitIntoWordsNoStop(const std::string& text) const {
        std::vector<std::string> words;
        for (const std::string& word : SplitIntoWords(text)) {
            if (!IsValidWord(word)) {
                using namespace std::string_literals;
                throw std::invalid_argument("Word "s + word + " is invalid"s);
            }
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }

SearchServer::QueryWord SearchServer::ParseQueryWord(const std::string& text) const {
        if (text.empty()) {
            using namespace std::string_literals;
            throw std::invalid_argument("Query word is empty"s);
        }
        std::string word = text;
        bool is_minus = false;
        if (word[0] == '-') {
            is_minus = true;
            word = word.substr(1);
        }
        if (word.empty() || word[0] == '-' || !IsValidWord(word)) {
            using namespace std::string_literals;
            throw std::invalid_argument("Query word "s + text + " is invalid"s);
        }

        return {word, is_minus, IsStopWord(word)};
    }

SearchServer::Query SearchServer::ParseQuery(const std::string& text) const {
        Query result;
        for (const std::string& word : SplitIntoWords(text)) {
            const auto query_word = ParseQueryWord(word);
            if (!query_word.is_stop) {
                if (query_word.is_minus) {
                    result.minus_words.insert(query_word.data);
                } else {
                    result.plus_words.insert(query_word.data);
                }
            }
        }
        return result;
    }

    // Existence required
double SearchServer::ComputeWordInverseDocumentFreq(const std::string& word) const {
        return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
    }
