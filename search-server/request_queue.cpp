#include <algorithm>

#include "request_queue.h"

using namespace std;

RequestQueue::RequestQueue(const SearchServer& search_server) : search_server_(search_server) {}

vector<Document> RequestQueue::AddFindRequest(const string& raw_query, DocumentStatus status) {
    auto matched_documents = search_server_.FindTopDocuments(raw_query, status);
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

std::vector<Document> RequestQueue::AddFindRequest(const string& raw_query) {
    auto matched_documents = search_server_.FindTopDocuments(raw_query);
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

int RequestQueue::GetNoResultRequests() const {
        return count_if(requests_.begin(), requests_.end(), [](const auto& req) {
            if (!req.is_non_empty) {
                return true;
            }
            return false;
        });
    }