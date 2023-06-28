#include "search_server.h"

    void SearchServer::AddDocument(int document_id, std::string_view document, DocumentStatus status, const std::vector<int>& ratings)
    {
    	using namespace std;
    	if ((document_id < 0) || (documents_.count(document_id) > 0)) {
    		throw invalid_argument("Invalid document_id"s);
    	}
    	raw_documents_.push_back(string(document.begin(), document.end()));
    	const auto words = SplitIntoWordsNoStop(string_view(raw_documents_.back()));

    	const double inv_word_count = 1.0 / words.size();
    	for (const auto word : words) {
    		word_to_document_freqs_[word][document_id] += inv_word_count;
    	    document_to_word_freqs_[document_id][word] += inv_word_count;
    	}
    	documents_.emplace(document_id, DocumentData{ComputeAverageRating(ratings), status});
    	document_ids_.insert(document_id);
    }

    void SearchServer::RemoveDocument(int document_id)
    {
    	if(documents_.count(document_id) == 0)
    		return;
    	documents_.erase(document_id);
    	document_ids_.erase(document_id);
    	for(const auto& [word, tf] : document_to_word_freqs_.at(document_id))
    		word_to_document_freqs_[word].erase(document_id);

    	document_to_word_freqs_.erase(document_id);
    }

    void SearchServer::RemoveDocument(const execution::sequenced_policy&, int document_id)
    {
    	SearchServer::RemoveDocument(document_id);
    }

    void SearchServer::RemoveDocument(const execution::parallel_policy&, int document_id)
    {
    	if(documents_.count(document_id) == 0)
    	{
    		return;
    	}
    	vector<const string_view*> words(document_to_word_freqs_[document_id].size());
    	transform(execution::par, document_to_word_freqs_[document_id].begin(), document_to_word_freqs_[document_id].end(), words.begin(),
    			[this](const auto& word){return &(word.first);});
    	for_each(execution::par, words.begin(), words.end(), [this, document_id](const string_view* word){word_to_document_freqs_[*word].erase(document_id);});

    	document_to_word_freqs_.erase(document_id);
    	documents_.erase(document_id);
    	document_ids_.erase(document_id);

    }

    std::vector<Document> SearchServer::FindTopDocuments(const std::string_view& raw_query, DocumentStatus status) const {
        return SearchServer::FindTopDocuments(
            raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
                return document_status == status;
            });
    }

    std::vector<Document> SearchServer::FindTopDocuments(const std::string_view& raw_query) const {
        return SearchServer::FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
    }


    int SearchServer::GetDocumentCount() const {
        return documents_.size();
    }

    std::set<int>::iterator SearchServer::begin()
    {
    	return document_ids_.begin();
    }

    std::set<int>::iterator SearchServer::end()
    {
    	return document_ids_.end();
    }

    const map<std::string_view, double>& SearchServer::GetWordFrequencies(int document_id)const
    {
    	if(document_to_word_freqs_.count(document_id) == 0)
    	{
    		static const map<std::string_view, double> empty_map;
    		return empty_map;
    	}
    	return document_to_word_freqs_.at(document_id);
    }

    std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const std::string_view& raw_query,
                                                            int document_id) const {
        	using namespace std;
            const auto query = ParseQuery(raw_query);

        	if(any_of(query.minus_words.begin(), query.minus_words.end(),//если хоть одно минус-слово встретилось, то возвращаем пустой вектор
        	        [this,document_id](const string_view& word) {return word_to_document_freqs_.at(word).count(document_id);}))
        	{
        	    return {vector<string_view>(), documents_.at(document_id).status};
        	}

            vector<string_view> matched_words;
            for (const string_view& word : query.plus_words) {
                if (word_to_document_freqs_.count(word) == 0) {
                    continue;
                }
                if (word_to_document_freqs_.at(word).count(document_id)) {
                    matched_words.push_back(word);
                }
            }
            /*for (const string_view& word : query.minus_words) {
                if (word_to_document_freqs_.count(word) == 0) {
                    continue;
                }
                if (SearchServer::word_to_document_freqs_.at(word).count(document_id)) {
                    matched_words.clear();
                    break;
                }
            }*/
            return {matched_words, documents_.at(document_id).status};
        }

        std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const execution::parallel_policy&, const std::string_view& raw_query, int document_id) const
        {
        	using namespace std;
        	pair<vector<string_view>, vector<string_view>> query;//аналог структуры query из двух векторов

        	for(const auto& word : SplitIntoWords(raw_query))// аналог ParseQuery
        	{
        		const auto query_word = ParseQueryWord(word);
        		if(!query_word.is_stop)
        		{
        			if(query_word.is_minus)
        			{
        				query.first.push_back(query_word.data);
        			}
        			else
        			{
        				query.second.push_back(query_word.data);
        			}
        		}
        	}

        	if(any_of(execution::par, query.first.begin(), query.first.end(),//если хоть одно минус-слово встретилось, то возвращаем пустой вектор
        	        [this,document_id](const string_view& word) {return word_to_document_freqs_.at(word).count(document_id);}))
        	{
        	    return {vector<string_view>(), documents_.at(document_id).status};
        	}

        	vector<string_view> matched_words(query.second.size());
        	const auto it = copy_if(execution::par, query.second.begin(), query.second.end(), matched_words.begin(),//если плюс-слово нашлось в документе, то копируем
        	        [this, document_id, &matched_words](const string_view& word) {
        	            return word_to_document_freqs_.at(word).count(document_id);
        	        });

        	matched_words.resize(distance(matched_words.begin(), it));//обрезаем вектор результата
        	sort(execution::par, matched_words.begin(), it);
        	const auto last = unique(execution::par, matched_words.begin(), it);
        	matched_words.erase(last, it);

        	return {matched_words, documents_.at(document_id).status};
        }

        std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const execution::sequenced_policy&, const std::string_view& raw_query, int document_id) const
        {
        	return SearchServer::MatchDocument(raw_query, document_id);
        }

    bool SearchServer::IsStopWord(const string_view word) const
    {
    	return stop_words_.count(word);
    }

    bool SearchServer::IsValidWord(const std::string_view& word) {
        // A valid word must not contain special characters
    	using namespace std;
        return none_of(word.begin(), word.end(), [](char c) {
            return c >= '\0' && c < ' ';
        });
    }

    std::vector<std::string_view> SearchServer::SplitIntoWordsNoStop(const std::string_view text) const
    {
    	using namespace std;
    	vector<string_view> words;
    	for(const string_view word : SplitIntoWords(text))
    	{
    		if (!SearchServer::IsValidWord(word)) {
    			throw invalid_argument("Word "s + static_cast<string>(word) + " is invalid"s);
    		}
    		if (!SearchServer::IsStopWord(word)) {
    		    words.push_back(word);
    		}
    	}
    	return words;
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

    SearchServer::QueryWord SearchServer::ParseQueryWord(const std::string_view& text) const {
    	using namespace std;
        if (text.empty()) {
            throw invalid_argument("Query word is empty"s);
        }
        string_view word = text;
        bool is_minus = false;
        if (word[0] == '-') {
            is_minus = true;
            word = word.substr(1);
        }
        if (word.empty() || word[0] == '-' || !SearchServer::IsValidWord(word)) {
            throw invalid_argument("Query word "s + static_cast<string>(text) + " is invalid");
        }

        return {word, is_minus, SearchServer::IsStopWord(word)};
    }

    SearchServer::Query SearchServer::ParseQuery(const std::string_view& text) const {
    	using namespace std;
        SearchServer::Query result;
        for (const auto& word : SplitIntoWords(text)) {
            const auto query_word = ParseQueryWord(word);
            if (!query_word.is_stop) {
                if (query_word.is_minus) {
                    //result.minus_words.insert(query_word.data);
                	result.minus_words.push_back(query_word.data);
                } else {
                    //result.plus_words.insert(query_word.data);
                	result.plus_words.push_back(query_word.data);
                }
            }
        }
        sort(result.minus_words.begin(), result.minus_words.end());
        sort(result.plus_words.begin(), result.plus_words.end());
    	auto last = std::unique(result.minus_words.begin(), result.minus_words.end());
    	result.minus_words.erase(last, result.minus_words.end());
    	last = std::unique(result.plus_words.begin(), result.plus_words.end());
    	result.plus_words.erase(last, result.plus_words.end());
        return result;
    }


    // Existence required
    double SearchServer::ComputeWordInverseDocumentFreq(const std::string_view& word) const {
    	using namespace std;
        return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
    }

