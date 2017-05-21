#pragma once

#include "textfile.h"
#include "utils.h"

#include <list>
#include <unordered_map>

struct SearchEntry {
	std::list<SearchEntry>::iterator iterator;
	std::string display_name;
	std::string display_subtitle;
	std::string text;
	std::shared_ptr<TextView> data;
	std::string str_data;
	void* ptr_data;
	double multiplier;
	double basescore;
};

struct SearchRank {
	SearchEntry* entry;
	double score;

	SearchRank(SearchEntry* entry, double score) : entry(entry), score(score) {}

	friend bool operator<(const SearchRank& l, const SearchRank& r) {
		return l.score > r.score;
	}
	friend bool operator> (const SearchRank& lhs, const SearchRank& rhs) { return rhs < lhs; }
	friend bool operator<=(const SearchRank& lhs, const SearchRank& rhs) { return !(lhs > rhs); }
	friend bool operator>=(const SearchRank& lhs, const SearchRank& rhs) { return !(lhs < rhs); }
	friend bool operator==(const SearchRank& lhs, const SearchRank& rhs) { return lhs.score == rhs.score; }
	friend bool operator!=(const SearchRank& lhs, const SearchRank& rhs) { return !(lhs == rhs); }
};

struct TrieNode {
	byte b;
	SearchEntry* entry;
	std::vector<std::unique_ptr<TrieNode>> leaves;

	TrieNode() : entry(), b(0) { }
	TrieNode(byte b) : entry(), b(b) { }
};

struct SearchIndex {
	SearchIndex();
	~SearchIndex();

	std::list<SearchEntry> entries;
	TrieNode root;

	// optional glob that allows ignoring of files that match a specific pattern
	PGGlobSet ignore_glob;

	std::unique_ptr<PGMutex> lock;

	// Adds an entry to the search index
	// thread unsafe: should only be called when "lock" is held
	void AddEntry(SearchEntry entry);
	// Removes an entry with the given name from the search index
	// thread unsafe: should only be called when "lock" is held
	void RemoveEntry(std::string name);

	// Returns the matched score of a given string for a given search term
	// this is the score used by SearchIndex::Search() to rank entries
	static int IndexScore(const std::string& str, const std::string& search_term);
	// Searches the index for the best <max_entries> SearchEntries that matches 
	// the given search_term. Additional entries can be provided in the <entries> parameter.
	// Note that the index is optional. 
	// This function is thread safe, and will get the the lock from "index" when required
	static std::vector<SearchEntry*> Search(std::vector<std::shared_ptr<SearchIndex>>& indices,
		const std::vector<SearchEntry*>& entries,
		const std::string& search_term,
		size_t max_entries);
};

