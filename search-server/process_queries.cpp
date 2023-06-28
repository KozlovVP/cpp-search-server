#include "process_queries.h"

std::vector<std::vector<Document>> ProcessQueries(const SearchServer& search_server, const std::vector<std::string>& queries)
{
	std::vector<std::vector<Document>> documents_lists(queries.size());
	transform(execution::par, queries.begin(), queries.end(), documents_lists.begin(), [&search_server](const std::string& local_query){return search_server.FindTopDocuments(local_query);});
	return documents_lists;
}

std::list<Document> ProcessQueriesJoined(const SearchServer& search_server, const std::vector<std::string>& queries)
{
	/*std::list<Document> documents_list;
	for(const auto& query : queries)
	{
		for(const auto& document : search_server.FindTopDocuments(query))
		{
			documents_list.push_back(document);
		}
	}*/
	vector<vector<Document>> tmp_doc_vector = ProcessQueries(search_server, queries);
	std::list<Document> documents_list;
	for(auto it = tmp_doc_vector.begin(); it!=tmp_doc_vector.end(); ++it)
	{
		for(const auto document : *it)
		{
			documents_list.push_back(document);
		}
	}
	return documents_list;
}
