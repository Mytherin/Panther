#pragma once

#include "searchindex.h"

#include <queue>
#include <unordered_set>


SearchEntry* SearchIndex::AddEntry(SearchEntry e) {
	TrieNode* node = &this->root;
	for (size_t i = 0; i < e.display_name.size(); i++) {
		byte b = e.display_name[i];
		TrieNode* next = nullptr;
		for (auto it = node->leaves.begin(); it != node->leaves.end(); it++) {
			if ((*it)->b == b) {
				next = it->get();
				break;
			}
		}
		if (!next) {
			next = new TrieNode(b);
			node->leaves.push_back(std::unique_ptr<TrieNode>(next));
		}
		if (i == e.display_name.size() - 1) {
			if (!node->entry) {
				// we only insert the entry if it is not already present
				entries.push_back(e);
				SearchEntry* entry = &entries.back();
				node->entry = entry;
			}
			return node->entry;
		}
		node = next;
	}
	assert(0);
	return nullptr;
}

void SearchIndex::RemoveEntry(SearchEntry* entry) {
	TrieNode* node = &this->root;
	for (size_t i = 0; i < entry->display_name.size(); i++) {
		byte b = entry->display_name[i];
		TrieNode* next = nullptr;
		for (auto it = node->leaves.begin(); it != node->leaves.end(); it++) {
			if ((*it)->b == b) {
				next = it->get();
				break;
			}
		}
		if (!next) {
			return;
		}
		if (i == entry->display_name.size() - 1) {
			node->entry = nullptr;
		}
		node = next;
	}
	// FIXME: erase unnecessary nodes in the trie as well
	// FIXME: <entry> remove from entries
}

struct SearchTermIndex {
	TrieNode* node;
	int index;
	int depth;
	int score;
	friend bool operator<(const SearchTermIndex& l, const SearchTermIndex& r) {
		return l.score > r.score;
	}
};

int SearchIndex::IndexScore(const std::string& str, const std::string& search_term) {
	// compute the index score of how much "search_term" matches "str"
	size_t search_index = 0;
	int score = 0;
	for (size_t i = 0; i < str.size(); i++) {
		if (search_index == search_term.size()) {
			// if we have completely matched the search_term, we stop
			// we apply a small penalty for the size of "str"
			// (i.e. smaller strings that fully match get more points than longer ones)
			score -= str.size() - i;
			return score;
		} else if (str[i] == search_term[search_index]) {
			// if the current character matches, we increase the score
			// and move to the next character in the search term
			// matches earlier in "str" are worth more points
			// hence we divide by (i - search_index + 1)
			score += 10000 / (i - search_index + 1);
			search_index++;
		} else {
			// if the current position does not match and the string has not
			// been fully matched the score is reduced by 20%
			score = score - score / 5;
		}
	}
	return score;
}

std::vector<SearchEntry*> SearchIndex::Search(SearchIndex* ind, const std::vector<SearchEntry*>& additional_entries, const std::string& search_term, size_t max_entries) {
	// we search the trie for search_term, and return at most max_entries results
	// we do fuzzy matching here, and return the n best matches
	// we compute the score as is done in SearchIndex::IndexScore incrementally over the trie
	// each character we expand changes the score

	// we expand nodes using a priority queue, and don't expand nodes which
	// we know will not be able to get in the top n returned entries anymore
	
	// priority queue of all open nodes
	std::priority_queue<SearchTermIndex> open_nodes;
	// priority queue of top n found results
	std::priority_queue<SearchRank> results;
	// we create a blacklist of entries: any entries that appear in additional_entries
	// should not be added again later
	std::unordered_set<std::string> blacklist;

	// first we iterate over the additional supplied entries
	// for these we just directly compute the score and add them to the results
	for (auto it = additional_entries.begin(); it != additional_entries.end(); it++) {
		int score = (*it)->basescore + IndexScore((*it)->display_name, search_term) * (*it)->multiplier;
		bool insert_entry = false;
		if (results.size() < max_entries) {
			insert_entry = true;
		} else if (results.top().score < score) {
			results.pop();
			insert_entry = true;
		}
		if (insert_entry) {
			SearchRank r(*it, score);
			results.push(r);
			blacklist.insert((*it)->text);
		}
	}

	if (ind) {
		// we start our search at the root node
		SearchTermIndex index;
		index.node = &ind->root;
		index.index = 0;
		index.score = 0;
		index.depth = 0;
		open_nodes.push(index);
		while (open_nodes.size() > 0) {
			SearchTermIndex current = open_nodes.top();
			open_nodes.pop();
			TrieNode* node = current.node;
			if (node->entry) {
				// if the node has an entry we insert if its score is good enough
				current.score = node->entry->basescore + node->entry->multiplier * current.score;
				bool insert_entry = false;
				if (results.size() < max_entries) {
					insert_entry = true;
				} else if (results.top().score < current.score) {
					results.pop();
					insert_entry = true;
				}
				if (insert_entry) {
					if (blacklist.count(node->entry->text) == 0) {
						SearchRank r(node->entry, current.score);
						results.push(r);
					}
				}
			}
			for (auto it = node->leaves.begin(); it != node->leaves.end(); it++) {
				// for each of the leaves, compute the score based on the next character
				SearchTermIndex next;
				next.depth = current.depth + 1;
				next.node = it->get();
				if (current.index == search_term.size()) {
					next.index = current.index;
					next.score = current.score - 1;
				} else if (next.node->b == search_term[current.index]) {
					next.index = current.index + 1;
					next.score = current.score + 10000 / (current.depth - current.index + 1);
				} else {
					next.index = current.index;
					next.score = current.score - current.score / 5;
				}
				if (results.size() >= max_entries) {
					// if we already have n results we determine if we should stop branching
					// based on the maximum possible score we can possible get in this branch
					// assuming the next [search_term.size() - current.index] characters 
					// match the query exactly, we compute the maximum possible score
					// if this is lower than the lowest score in the top n entries, we stop
					size_t maximum_score = next.score + (search_term.size() - next.index) * 10000 / (next.depth - next.index + 1);
					if (maximum_score <= results.top().score) {
						continue;
					}
				}
				open_nodes.push(next);
			}
		}
	}
	std::vector<SearchEntry*> entries;
	while (results.size() > 0) {
		SearchEntry* entry = results.top().entry;
		results.pop();
		entries.push_back(entry);
	}
	// the priority queue was in reverse order (i.e. top() was the worst match)
	// so we reverse the results before returning them
	std::reverse(entries.begin(), entries.end());
	return entries;
}

