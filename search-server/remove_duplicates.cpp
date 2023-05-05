#include "remove_duplicates.h"

void RemoveDuplicates(SearchServer& search_server) { 
    std::set<std::set<std::string>> res; 
    std::vector<int> duplicates_id; 

    for(auto it = search_server.begin(); it != search_server.end(); ++it) { 
        auto get_words = search_server.GetWordFrequencies(*it); 
        std::set<std::string> words; 

        for(auto [word, tf] : get_words) { 
            words.insert(word); 
        } 

        auto emplace_bool = res.emplace(words);
        if(!emplace_bool.second) {    // if element exists
            duplicates_id.push_back(*it); 
            continue;
        } 
    } 

    for(auto id : duplicates_id) { 
        std::cout << "Found duplicate document id " << id << std::endl; 
        search_server.RemoveDocument(id); 
    } 
} 
