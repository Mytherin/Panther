#pragma once

#include "textbuffer.h"

struct PGTextPosition {
	PGTextBuffer* buffer = nullptr;
	lng position = 0;

	lng GetPosition(PGTextBuffer* start_buffer);
	char* data() { return buffer->buffer + position; }

	PGTextPosition(PGTextBuffer* buffer, const char* ptr) : buffer(buffer), position(ptr - buffer->buffer) {}
	PGTextPosition() : buffer(nullptr), position(0) {}
	PGTextPosition(PGTextBuffer* buffer) : buffer(buffer), position(0) {}
	PGTextPosition(PGTextBuffer* buffer, lng position) : buffer(buffer), position(position) {}

	inline char operator* () { return buffer ? buffer->buffer[position] : '\0'; }

	inline bool Offset(const lng offset) {
		if (buffer == nullptr) {
			return false;
		}
		position += offset;
		while (position >= (lng)buffer->current_size) {
			if (!buffer->next) return false;
			position -= buffer->current_size;
			buffer = buffer->next;
		}
		while (position < 0) {
			buffer = buffer->prev;
			if (!buffer) return false;
			position += buffer->current_size;
		}
		return true;
	}
};

inline bool operator< (const PGTextPosition& lhs, const PGTextPosition& rhs) {
	return (lhs.buffer->index < rhs.buffer->index) ||
		(lhs.buffer->index == rhs.buffer->index && lhs.position < rhs.position);
}
inline bool operator> (const PGTextPosition& lhs, const PGTextPosition& rhs) { return rhs < lhs; }
inline bool operator<=(const PGTextPosition& lhs, const PGTextPosition& rhs) { return !(lhs > rhs); }
inline bool operator>=(const PGTextPosition& lhs, const PGTextPosition& rhs) { return !(lhs < rhs); }
inline bool operator== (const PGTextPosition& lhs, const PGTextPosition& rhs) {
	return (lhs.buffer == rhs.buffer && lhs.position == rhs.position);
}
inline bool operator!= (const PGTextPosition& lhs, const PGTextPosition& rhs) { return !(lhs == rhs); }

inline PGTextPosition operator+ (PGTextPosition lhs, lng rhs) {
	lhs.Offset(rhs);
	return lhs;
}
inline PGTextPosition operator- (const PGTextPosition& lhs, lng rhs) {
	return lhs + (-rhs);
}

struct PGTextRange {
	PGTextBuffer* start_buffer;
	lng start_position;
	PGTextBuffer* end_buffer;
	lng end_position;
	std::shared_ptr<PGTextBuffer> owned_data;

	PGTextRange(std::string text);
	PGTextRange(PGTextPosition start, PGTextPosition end);
	PGTextRange(PGTextBuffer* start_buffer, lng start_position, PGTextBuffer* end_buffer, lng end_position);
	PGTextRange();

	std::string GetString() const;
	// "data" has to be lowercase
	int ascii_strcasecmp(const char* data, size_t length) const;
	int _memcmp(const char* data, size_t length) const;
	PGTextPosition _memchr(int value) const;
	PGTextPosition _memrchr(int value) const;
	PGTextPosition _memcasechr(int value) const;
	PGTextPosition _memcaserchr(int value) const;

	PGTextPosition startpos() const { return PGTextPosition(start_buffer, start_position); }
	PGTextPosition endpos() const { return PGTextPosition(end_buffer, end_position); }

	char* begin() const { return start_buffer->buffer + start_position; }
	char* end_start() const {
		return
			start_buffer == end_buffer ?
			start_buffer->buffer + end_position :
			start_buffer->buffer + start_buffer->current_size;
	}
	char* end() const { return end_buffer->buffer + end_position; }

	void remove_prefix(size_t length);
private:
	template<void* T(const void *s, int c, size_t n)>
	PGTextPosition _reverse_buffer_lookup(int value) const;
	template<int T(const void* a, const void* b, size_t len)>
	int _buffer_comparison(const char* data, size_t length) const;
	template<void* T(const void *s, int c, size_t n)>
	PGTextPosition _buffer_lookup(int value) const;
};


inline bool operator< (const PGTextRange& lhs, const PGTextRange& rhs) {
	return lhs.end_buffer->index < rhs.start_buffer->index || 
		(lhs.end_buffer->index == rhs.start_buffer->index && lhs.end_position < rhs.start_position);
}
inline bool operator> (const PGTextRange& lhs, const PGTextRange& rhs) { return rhs < lhs; }
