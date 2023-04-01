#include <cmath>
#include <numeric>
#include <vector>
#include <tuple>
#include <map>
#include <set>
#include <execution>

#include "search_server.h"

using namespace std;

SearchServer::SearchServer(const string& stop_words_text) :
    SearchServer(SplitIntoWords(stop_words_text)) {}

SearchServer::SearchServer(const string_view stop_words_text) :
    SearchServer(SplitIntoWords(stop_words_text)) {}

void SearchServer::AddDocument(int document_id, string_view document, DocumentStatus status,
    const vector<int>& ratings) {
    if ((document_id < 0) || (documents_.count(document_id) > 0)) {
        throw invalid_argument("Invalid document_id"s);
    }
    
    documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status, string(document) });
    const auto words = SplitIntoWordsNoStop(documents_.at(document_id).word);
    const double inv_word_count = 1.0 / words.size();
    for (const string_view word : words) {
        word_to_document_freqs_[word][document_id] += inv_word_count;
        id_to_word_freqs_[document_id][word] += inv_word_count;
    }
    document_ids_.insert(document_id);
}


vector<Document> SearchServer::FindTopDocuments(const string_view raw_query, DocumentStatus status) const {
    return FindTopDocuments(execution::seq, 
        raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
            return document_status == status;
        });
}

vector<Document> SearchServer::FindTopDocuments(const execution::sequenced_policy&, const string_view raw_query, DocumentStatus status) const {
    return FindTopDocuments(raw_query, status);
}

vector<Document> SearchServer::FindTopDocuments(const execution::parallel_policy&, const string_view raw_query, DocumentStatus status) const {
    return FindTopDocuments(execution::par, 
        raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
            return document_status == status;
        });
}

vector<Document> SearchServer::FindTopDocuments(const string_view raw_query) const {
    return FindTopDocuments(execution::seq, raw_query, DocumentStatus::ACTUAL);
}

vector<Document> SearchServer::FindTopDocuments(const execution::sequenced_policy&, const string_view raw_query) const {
    return FindTopDocuments(execution::seq, raw_query, DocumentStatus::ACTUAL);
}

vector<Document> SearchServer::FindTopDocuments(const execution::parallel_policy&, const string_view raw_query) const {
    return FindTopDocuments(execution::par, raw_query, DocumentStatus::ACTUAL);
}


set<int>::const_iterator SearchServer::begin() const {
    return document_ids_.begin();
}

set<int>::const_iterator SearchServer::end() const {
    return document_ids_.end();
}

int SearchServer::GetDocumentCount() const {
    return documents_.size();
}

tuple<vector<string_view>, DocumentStatus> SearchServer::MatchDocument(const string_view raw_query,
    int document_id) const {
    return MatchDocument(execution::seq, raw_query, document_id);
}

tuple<vector<string_view>, DocumentStatus> SearchServer::MatchDocument(const execution::sequenced_policy&, const string_view raw_query,
        int document_id) const {
    if (!document_ids_.count(document_id)){
            using namespace std::string_literals;
            throw std::out_of_range("out of range"s);
    }
    const Query query = ParseQuery(raw_query, true);
    
    const auto check_word = [this, document_id](string_view word) {
                               const auto it = word_to_document_freqs_.find(word);
                               return it->second.count(document_id) && it != word_to_document_freqs_.end();};
    
    bool has_stop_words = any_of(execution::seq, query.minus_words.begin(), query.minus_words.end(),
                           check_word);
    
    if (has_stop_words) {
        return { {}, documents_.at(document_id).status};
    }
    
    vector<string_view> matched_words(query.plus_words.size());
    auto last_copy = copy_if(execution::seq,
        query.plus_words.begin(), query.plus_words.end(),
        matched_words.begin(),
        check_word
    );
    sort(matched_words.begin(), last_copy);
    auto last_unique = unique(matched_words.begin(), last_copy);
    matched_words.erase(last_unique, matched_words.end());
    return { matched_words, documents_.at(document_id).status };
}

tuple<vector<string_view>, DocumentStatus> SearchServer::MatchDocument(const execution::parallel_policy&, const string_view raw_query,
        int document_id) const {
    if (!document_ids_.count(document_id)){
            using namespace std::string_literals;
            throw std::out_of_range("out of range"s);
    }
    
    const Query query = ParseQuery(raw_query, false);
    
    const auto check_word = [this, document_id](string_view word) {
                               const auto it = word_to_document_freqs_.find(word);
                               return it->second.count(document_id) && it != word_to_document_freqs_.end();};
    
    bool has_stop_words = any_of(execution::par, query.minus_words.begin(), query.minus_words.end(),
                           check_word);
    
    if (has_stop_words) {
        return { {}, documents_.at(document_id).status};
    }
    
    vector<string_view> matched_words(query.plus_words.size());
    auto last_copy = copy_if(execution::par,
        query.plus_words.begin(), query.plus_words.end(),
        matched_words.begin(),
        check_word
    );
    sort(execution::par, matched_words.begin(), last_copy);
    auto last_unique = unique(execution::par, matched_words.begin(), last_copy);
    matched_words.erase(last_unique, matched_words.end());
    return { matched_words, documents_.at(document_id).status };
}

bool SearchServer::IsStopWord(const string_view word) const {
    return stop_words_.count(word) > 0;
}

bool SearchServer::IsValidWord(const string_view word) {
    return none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
        });
}

vector<string_view> SearchServer::SplitIntoWordsNoStop(const string_view text) const {
    vector<string_view> words;
    for (const string_view word : SplitIntoWords(text)) {
        if (!IsValidWord(word)) {
            throw invalid_argument("Word " + string(word) + " is invalid");
        }
        if (!IsStopWord(word)) {
            words.push_back(word);
        }
    }
    return words;
}

int SearchServer::ComputeAverageRating(const vector<int>& ratings) {
    if (ratings.empty()) {
        return 0;
    }
    int rating_sum = accumulate(ratings.begin(), ratings.end(), 0);
    return rating_sum / static_cast<int>(ratings.size());
}

SearchServer::QueryWord SearchServer::ParseQueryWord(const string_view text) const {
    if (text.empty()) {
        throw invalid_argument("Query word is empty"s);
    }
    string_view word = text;
    bool is_minus = false;
    if (word[0] == '-') {
        is_minus = true;
        word.remove_prefix(1);
    }
    if (word.empty() || word[0] == '-' || !IsValidWord(word)) {
        throw invalid_argument("Query word "s + string(text) + " is invalid");
    }

    return { word, is_minus, IsStopWord(word) };
}

SearchServer::Query SearchServer::ParseQuery(const string_view text, const bool to_sort) const {
    Query result;
    for (const string_view word : SplitIntoWords(text)) {
        const auto query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                result.minus_words.push_back(query_word.data);
            }
            else {
                result.plus_words.push_back(query_word.data);
            }
        }
    }
    if (to_sort) {
        sort(result.plus_words.begin(), result.plus_words.end());
        sort(result.minus_words.begin(), result.minus_words.end());
        auto last_plus = unique(result.plus_words.begin(), result.plus_words.end());
        auto last_minus = unique(result.minus_words.begin(), result.minus_words.end());
        result.plus_words.erase(last_plus, result.plus_words.end());
        result.minus_words.erase(last_minus, result.minus_words.end());
    }
    return result;
}

double SearchServer::ComputeWordInverseDocumentFreq(const string_view word) const {
    return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}

const map<string_view, double>& SearchServer::GetWordFrequencies(int document_id) const {
    static map<string_view, double> empty;
    if (document_ids_.find(document_id) == document_ids_.end()) {
        return empty;
    }
    return id_to_word_freqs_.at(document_id);
}

void SearchServer::RemoveDocument(int document_id) {
    for (auto [word, freq] : GetWordFrequencies(document_id)) {
        auto it = word_to_document_freqs_[word].find(document_id);
        if (it != word_to_document_freqs_[word].end()) {
            word_to_document_freqs_[word].erase(it);
        }
    }
    documents_.erase(document_id);
    document_ids_.erase(document_id);
    id_to_word_freqs_.erase(document_id);
}

void SearchServer::RemoveDocument(const std::execution::sequenced_policy& , int document_id) {
    RemoveDocument(document_id);
}


void SearchServer::RemoveDocument(const std::execution::parallel_policy& , int document_id) {
    if (id_to_word_freqs_.count(document_id) == 0) {
        return;
    }
    
    const map<string_view, double>& tmp = GetWordFrequencies(document_id);
    vector<string_view> words(tmp.size());
    transform(execution::par,
              tmp.begin(), tmp.end(),
              words.begin(),
              [](const auto& items) { return items.first;}
              );
    for_each(execution::par,
             words.begin(), words.end(),
             [this, document_id] (auto word) {
                 word_to_document_freqs_[word].erase(document_id);
             });
    
    documents_.erase(document_id);
    document_ids_.erase(document_id);
    id_to_word_freqs_.erase(document_id);
}