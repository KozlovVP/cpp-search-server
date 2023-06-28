#pragma once
#include <cmath>
#include <vector>
#include <iostream>
#include "document.h"

template <typename T_Iterator>
class IteratorRange{
public:
	IteratorRange(T_Iterator it_begin, T_Iterator it_end, size_t size): it_begin_(it_begin), it_end_(it_end), size_(size){}
	~IteratorRange(){}
	T_Iterator begin()const
	{
		return it_begin_;
	}
	T_Iterator end()const
	{
		return it_end_;
	}
	size_t size()const
	{
		return size_;
	}
private:
	T_Iterator it_begin_;
	T_Iterator it_end_;
	int size_;
};

template <typename It>
class Paginator{
public:
	Paginator(It it_begin, It it_end, size_t size)
	{
		using namespace std;
		total_docs_ = distance(it_begin, it_end);
		pages_number_ = ceil(total_docs_/(size*1.0));
		for(size_t i = 0; i < pages_number_-1; ++i)
		{
			pages_.push_back({it_begin+size*i, it_begin+size*(i+1), size});
		}
		pages_.push_back({it_begin+size*(pages_number_-1), it_end, size});
	}
	~Paginator()
	{}

	auto begin() const
	{
		return pages_.begin();
	}
	auto end() const
	{
		return pages_.end();
	}

private:
	size_t total_docs_;
	size_t pages_number_;
	std::vector<IteratorRange<It>> pages_;
};

template <typename It>
std::ostream& operator<<(std::ostream& os, IteratorRange<It> page)
{
	for(auto it = page.begin(); it != page.end(); ++it)
	{
		os << *it;
	}
	return os;
}

template <typename Container>
auto Paginate(const Container& c, size_t page_size)
{
	return Paginator(c.begin(), c.end(), page_size);
}

