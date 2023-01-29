#pragma once
#include <string>
#include <vector>
#include <deque>

#include "search_server.h"
#include "document.h"

class RequestQueue {
public:
    explicit RequestQueue(const SearchServer& search_server);
   
    template <typename DocumentPredicate>
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate);
    
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus status);
    
    std::vector<Document> AddFindRequest(const std::string& raw_query);
    
    int GetNoResultRequests() const;
private:
    struct QueryResult {
        bool is_non_empty = false;
    };
    std::deque<QueryResult> requests_;
    const static int min_in_day_ = 1440;
    const SearchServer& search_server_;
}; 

template <typename DocumentPredicate>
std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate) {
    auto matched_documents = search_server_.FindTopDocuments(raw_query, document_predicate);
    QueryResult q_result;
    if (!matched_documents.empty()) {
        q_result.is_non_empty = true;
    }
    if (requests_.size() == min_in_day_) {
        requests_.pop_front();
    }
    requests_.push_back(q_result);
    return matched_documents;
}