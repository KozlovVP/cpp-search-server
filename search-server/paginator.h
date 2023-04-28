#pragma once
#include <vector>

template <typename Iterator>
class IteratorRange
{
public:
    IteratorRange(Iterator begin, Iterator end, size_t size)
        :begin_(begin), end_(end), size_(size) {}
    
    auto begin() const {return begin_;}
    auto end() const {return end_;}
    auto size() const {return size_;}
    
private:
    Iterator begin_, end_;
    size_t size_;
};

template <typename Iterator>
class Paginator 
{
public:
    Paginator(Iterator range_begin, Iterator range_end, size_t size)
    {
        for (int i = 0; i < distance(range_begin, range_end); i+=size)
        {
            //IteratorRange<Iterator> temp;
            if (distance(range_begin + i, range_end) >= size)
            {
                IteratorRange<Iterator> temp(range_begin + i, range_begin + i + size, size);
                /*temp.begin = range_begin + i;
                temp.end = range_begin + i + size;
                temp.size = size;*/
                pages_.push_back(temp);
            }
            else
            {
                IteratorRange<Iterator> temp(range_begin + i, range_end, distance(temp.begin(), temp.end()));
                /*temp.begin = range_begin + i;
                temp.end = range_end;
                temp.size = distance(temp.begin, temp.end);*/
                pages_.push_back(temp);
                break;  
            }
        }
    }
    
    auto begin() const {return pages_.begin();}
    auto end() const {return pages_.end();}
    
    std::vector<IteratorRange<Iterator>> pages_;
};


template <typename Container>
auto Paginate(const Container& c, size_t page_size) {
    return Paginator(begin(c), end(c), page_size);
}
