//Вставьте сюда своё решение из урока «Очередь запросов» темы «Стек, очередь, дек».‎

#include "request_queue.h"

/*explicit*/ RequestQueue::RequestQueue(const SearchServer& search_server): server_(&search_server) {
    }

vector<Document> RequestQueue::AddFindRequest(const string& raw_query, DocumentStatus status) {
    	++request_num_;
    	if(request_num_ > min_in_day_)
    	{
    	    requests_.pop_front();
    	    --request_num_;
    	}
    	requests_.push_back({server_->FindTopDocuments(raw_query, status)});
    	return server_->FindTopDocuments(raw_query, status);
    }
vector<Document> RequestQueue::AddFindRequest(const string& raw_query) {
    	++request_num_;
    	if(request_num_ > min_in_day_)
    	{
    	    requests_.pop_front();
    	    --request_num_;
    	}
    	requests_.push_back({server_->FindTopDocuments(raw_query)});
    	return server_->FindTopDocuments(raw_query);
    }
int RequestQueue::GetNoResultRequests() const {
    	return count_if(requests_.begin(), requests_.end(), [](QueryResult doc){return doc.documents_.empty();});
    }


