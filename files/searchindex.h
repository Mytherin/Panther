#pragma once

#include "textfile.h"
#include "utils.h"

#include <list>
#include <unordered_map>

struct SearchEntry {
	std::string display_name;
	std::string display_subtitle;
	std::string text;
	std::shared_ptr<TextFile> data;
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

	TrieNode() : entry(nullptr), b(0) { }
	TrieNode(byte b) : entry(nullptr), b(b) { }
};

struct SearchIndex {
	std::list<SearchEntry> entries;
	TrieNode root;

	SearchEntry* AddEntry(SearchEntry entry);
	void RemoveEntry(SearchEntry* entry);

	static int IndexScore(const std::string& str, const std::string& search_term);
	static std::vector<SearchEntry*> Search(SearchIndex* index, const std::vector<SearchEntry*>& entries, const std::string& search_term, size_t max_entries);
};

