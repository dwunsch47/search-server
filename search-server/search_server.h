#pragma once

#include <string>
#include <vector>
#include <algorithm>
#include <tuple>
#include <map>
#include <iostream>
#include <set>
#include <execution>
#include <string_view>

#include "string_processing.h"
#include "document.h"
#include "log_duration.h"
#include "concurrent_map.h"

const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double EPSILON = 1e-6;

class SearchServer {
public:

    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words);

    explicit SearchServer(const std::string& stop_words_text);
    explicit SearchServer(const std::string_view stop_words_text);

    void AddDocument(int document_id, const std::string_view document, DocumentStatus status,
        const std::vector<int>& ratings);

    template <typename Policy, typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(const Policy& policy, const std::string_view raw_query, DocumentPredicate document_predicate) const;
    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(const std::string_view raw_query, DocumentPredicate document_predicate) const;
    
    std::vector<Document> FindTopDocuments(const std::string_view raw_query, DocumentStatus status) const;
    std::vector<Document> FindTopDocuments(const std::execution::sequenced_policy&, const std::string_view raw_query, DocumentStatus status) const;
    std::vector<Document> FindTopDocuments(const std::execution::parallel_policy&, const std::string_view raw_query, DocumentStatus status) const;
    
    std::vector<Document> FindTopDocuments(const std::string_view raw_query) const;
    std::vector<Document> FindTopDocuments(const std::execution::sequenced_policy&, const std::string_view raw_query) const;
    std::vector<Document> FindTopDocuments(const std::execution::parallel_policy&, const std::string_view raw_query) const;

    int GetDocumentCount() const;

    std::set<int>::const_iterator begin() const;
    std::set<int>::const_iterator end() const;

    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const std::string_view raw_query,
        int document_id) const;
    
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const std::execution::sequenced_policy&, const std::string_view raw_query,
        int document_id) const;
    
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const std::execution::parallel_policy&, const std::string_view raw_query,
        int document_id) const;

    const std::map<std::string_view, double>& GetWordFrequencies(int document_id) const;
    
    void RemoveDocument(int document_id);
    void RemoveDocument(const std::execution::sequenced_policy& , int document_id);
    void RemoveDocument(const std::execution::parallel_policy& , int document_id);

private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
        std::string word;
    };
    const std::set<std::string, std::less<>> stop_words_;
    std::map<std::string_view, std::map<int, double>> word_to_document_freqs_;
    std::map<int, std::map<std::string_view, double>> id_to_word_freqs_;
    std::map<int, DocumentData> documents_;
    std::set<int> document_ids_;

    bool IsStopWord(const std::string_view word) const;

    static bool IsValidWord(const std::string_view word);

    std::vector<std::string_view> SplitIntoWordsNoStop(const std::string_view text) const;

    static int ComputeAverageRating(const std::vector<int>& ratings);

    struct QueryWord {
        std::string_view data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(const std::string_view text) const;

    struct Query {
        std::vector<std::string_view> plus_words;
        std::vector<std::string_view> minus_words;
    };
    
    Query ParseQuery(const std::string_view text, const bool to_sort) const;

    double ComputeWordInverseDocumentFreq(const std::string_view word) const;

    template <typename Policy, typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const Policy& policy, const Query& query,
        DocumentPredicate document_predicate) const;
};

template <typename StringContainer>
SearchServer::SearchServer(const StringContainer& stop_words)
    : stop_words_(MakeUniqueNonEmptyStrings(stop_words)) {
    if (!all_of(stop_words_.begin(), stop_words_.end(), IsValidWord)) {
        throw std::invalid_argument("Some of stop words are invalid");
    }
}

template <typename Policy, typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(const Policy& policy, const std::string_view raw_query,
    DocumentPredicate document_predicate) const {
    const auto query = ParseQuery(raw_query, true);

    auto matched_documents = FindAllDocuments(policy, query, document_predicate);

    std::sort(policy, matched_documents.begin(), matched_documents.end(),
        [](const Document& lhs, const Document& rhs) {
            if (std::abs(lhs.relevance - rhs.relevance) < EPSILON) {
                return lhs.rating > rhs.rating;
            }
            else {
                return lhs.relevance > rhs.relevance;
            }
        });
    if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
        matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    }

    return matched_documents;
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(const std::string_view raw_query, DocumentPredicate document_predicate) const {
    return FindTopDocuments(std::execution::seq, raw_query, document_predicate);
}

template <typename Policy, typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(const Policy& policy, const Query& query,
    DocumentPredicate document_predicate) const {
    ConcurrentMap<int, double> document_to_relevance(document_ids_.size());
    
    std::for_each(policy, 
            query.plus_words.begin(), query.plus_words.end(),
            [this, &document_to_relevance, &document_predicate, &policy] (const std::string_view word) {
               if (word_to_document_freqs_.count(word) > 0) {
                   const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
                   for_each(policy,
                           word_to_document_freqs_.at(word).begin(), word_to_document_freqs_.at(word).end(),
                           [this, &document_to_relevance, &document_predicate, &inverse_document_freq] (const auto& document) {
                               const auto& document_data = documents_.at(document.first);
                               if (document_predicate(document.first, document_data.status, document_data.rating)) {
                                   document_to_relevance[document.first].ref_to_value += document.second * inverse_document_freq;
                               }
                           });
               } 
            });
    
    std::for_each(policy,
            query.minus_words.begin(), query.minus_words.end(),
            [this, &document_to_relevance, &policy] (const std::string_view word) {
                if (word_to_document_freqs_.count(word) > 0) {
                    std::for_each(policy,
                                 word_to_document_freqs_.at(word).begin(), word_to_document_freqs_.at(word).end(),
                                 [this, &document_to_relevance] (const auto& document) {
                                     document_to_relevance.Erase(document.first);
                                 });
                }
            });

    const std::map<int, double> ordinary_map = document_to_relevance.BuildOrdinaryMap();
    
    std::vector<Document> matched_documents;
    for (const auto [document_id, relevance] : ordinary_map) {
        matched_documents.push_back(
            { document_id, relevance, documents_.at(document_id).rating });
    }
    return matched_documents;
}