#include "string_processing.h"

std::vector<std::string_view> SplitIntoWords(const std::string_view& text)
{
	using namespace std;
    vector<string_view> words;
    auto next_it = text.begin();
    for(auto it = text.begin(); it!= text.end(); ++it)
    {
    	if(*it == ' ')
    	{
    		if(next_it != it)
    		{
    			words.push_back(string_view(next_it, (it - next_it)));
    			next_it = it+1;
    		}
    		else
    		{
    			++next_it;
    		}
    	}
    }
    if(next_it != text.end())
    {
    	words.push_back(string_view(next_it, (text.end() - next_it)));
    }
    return words;
}
