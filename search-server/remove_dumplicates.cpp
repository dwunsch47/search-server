#include <set>
#include <map>
#include <string>

#include "remove_duplicates.h"

using namespace std;

void RemoveDuplicates(SearchServer& search_server) {
	set<int> for_removal;

	set<set<string>> document_keeper;
	
	for (const int document_id : search_server) {
		set<string> current_id_words;
		map<string, double> current_id_to_word_freq = search_server.GetWordFrequencies(document_id);
		for (auto&& [word, freq] : current_id_to_word_freq) {
			current_id_words.insert(word);
		}
		if (document_keeper.count(current_id_words) == 0) {
			document_keeper.insert(current_id_words);
		}
		else {
			cout << "Found duplicate document id "s << document_id << endl;
			for_removal.insert(document_id);
		}
	}

	for (const int& id : for_removal) {
		search_server.RemoveDocument(id);
	}
}