#include "request_queue.h"

    std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentStatus status) {
    	++RequestQueue::request_num_;
    	if(RequestQueue::request_num_ > RequestQueue::min_in_day_)
    	{
    		RequestQueue::requests_.pop_front();
    	    --RequestQueue::request_num_;
    	}
    	RequestQueue::requests_.push_back({RequestQueue::server_.FindTopDocuments(raw_query, status)});
    	return server_.FindTopDocuments(raw_query, status);
    }
    std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query) {
    	++RequestQueue::request_num_;
    	if(RequestQueue::request_num_ > RequestQueue::min_in_day_)
    	{
    		RequestQueue::requests_.pop_front();
    	    --RequestQueue::request_num_;
    	}
    	RequestQueue::requests_.push_back({RequestQueue::server_.FindTopDocuments(raw_query)});
    	return server_.FindTopDocuments(raw_query);
    }
    int RequestQueue::GetNoResultRequests() const {
    	using namespace std;
    	int count = 0;
    	deque<QueryResult> dq_tmp(requests_);
    	while (!dq_tmp.empty())
    	{
    	    if (dq_tmp.back().documents_.empty())
    	    	++count;
    	    dq_tmp.pop_back();
    	}
    	return count;
    }
