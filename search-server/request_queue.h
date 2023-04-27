#pragma once
#include "search_server.h"
#include <vector>
#include <deque>

class RequestQueue {
public:
    explicit RequestQueue(const SearchServer& search_server);
    // сделаем "обёртки" для всех методов поиска, чтобы сохранять результаты для нашей статистики
    template <typename DocumentPredicate>
    vector<Document> AddFindRequest(const string& raw_query, DocumentPredicate document_predicate);
    vector<Document> AddFindRequest(const string& raw_query, DocumentStatus status);
    vector<Document> AddFindRequest(const string& raw_query);
    int GetNoResultRequests() const;
private:
    struct QueryResult {
        //vector<Document> documents_;
        QueryResult(int size) 
            :docs_count(size) {}
        int docs_count;
    };
    deque<QueryResult> requests_;
    const static int min_in_day_ = 1440;
    int request_num_ = 0;
    const SearchServer *server_;
};

template <typename DocumentPredicate>
vector<Document> RequestQueue::AddFindRequest(const string& raw_query, DocumentPredicate document_predicate) {
        ++request_num_;
        if(request_num_ > min_in_day_)
        {
        	requests_.pop_front();
        	--request_num_;
        }
        requests_.push_back(QueryResult(server_->FindTopDocuments(raw_query, document_predicate).size()));
        return server_->FindTopDocuments(raw_query, document_predicate);
    }
