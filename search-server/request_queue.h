#pragma once
#include "search_server.h"
#include <vector>
#include <deque>

class RequestQueue {
public:
    explicit RequestQueue(const SearchServer& search_server): server_(search_server) {
    }
    template <typename DocumentPredicate>
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate);
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus status);
    std::vector<Document> AddFindRequest(const std::string& raw_query);
    int GetNoResultRequests() const;

private:
    struct QueryResult {
        std::vector<Document> documents_;
    };
    std::deque<QueryResult> requests_;
    const static int min_in_day_ = 1440;
    int request_num_ = 0;
    const SearchServer& server_;
};

template <typename DocumentPredicate>
std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate) {
	using namespace std;
    ++RequestQueue::request_num_;
    if(RequestQueue::request_num_ > RequestQueue::min_in_day_)
    {
    	RequestQueue::requests_.pop_front();
    	--RequestQueue::request_num_;
    }
    RequestQueue::requests_.push_back({RequestQueue::server_.FindTopDocuments(raw_query, document_predicate)});
    return RequestQueue::server_.FindTopDocuments(raw_query, document_predicate);
}
