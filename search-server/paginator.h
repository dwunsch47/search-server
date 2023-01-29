#pragma once

#include <iostream>

template<typename Iterator>
class IteratorRange {
public:
    IteratorRange(Iterator begin, Iterator end);
    auto begin() const;
    auto end() const;
    
private:
    Iterator begin_;
    Iterator end_;
};

template<typename Iterator>
IteratorRange<Iterator>::IteratorRange(Iterator begin, Iterator end) :
        begin_(begin), end_(end) {}

template<typename Iterator>
auto IteratorRange<Iterator>::begin() const {
    return begin_;
}

template<typename Iterator>
auto IteratorRange<Iterator>::end() const {
    return end_;
}

template<typename Iterator>
class Paginator {
public:
    explicit Paginator(Iterator begin, Iterator end, size_t page_size);
    auto begin() const;
    auto end() const;
private:
    std::vector<IteratorRange<Iterator>> pages_;
    size_t page_size_;
};

template<typename Iterator>
Paginator<Iterator>::Paginator(Iterator begin, Iterator end, size_t page_size):page_size_(page_size) {
    Iterator it = begin;
    for (; it + page_size < end; it += page_size){
        pages_.push_back(IteratorRange(it, it + page_size));
    }
    if (it < end) {
        pages_.push_back(IteratorRange(it, end));
    }
}

template<typename Iterator>
auto Paginator<Iterator>::begin() const {
    return pages_.begin();
}

template<typename Iterator>
auto Paginator<Iterator>::end() const {
    return pages_.end();
}

template<typename Iterator>
std::ostream& operator<<(std::ostream& out, IteratorRange<Iterator> result) {
    for (auto it = result.begin(); it < result.end(); ++it) {
        out << *it;
    }
    return out;
}

template <typename Container>
auto Paginate(const Container& c, size_t page_size) {
    return Paginator(begin(c), end(c), page_size);
}