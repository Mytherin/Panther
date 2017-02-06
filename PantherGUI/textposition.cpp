
#include "textposition.h"

lng PGTextPosition::GetPosition(PGTextBuffer* start_buffer) {
	lng current_position = 0;
	PGTextBuffer* current_buffer = start_buffer;
	while (current_buffer != buffer) {
		current_position += current_buffer->current_size;
		current_buffer = current_buffer->next;
	}
	current_position += position;
	return current_position;
}

void PGTextPosition::Offset(lng offset) {
	if (buffer == nullptr) {
		return;
	}
	position += offset;
	while (position >= (lng)buffer->current_size) {
		if (!buffer->next) return;
		position -= buffer->current_size;
		buffer = buffer->next;
	}
	while (position < 0) {
		buffer = buffer->prev;
		if (!buffer) return;
		position += buffer->current_size;
	}
}

PGTextRange::PGTextRange(std::string text) : owned_data(nullptr) {
	PGTextBuffer* buffer = new PGTextBuffer(text.data(), text.size(), 0);
	buffer->prev = nullptr;
	buffer->next = nullptr;
	buffer->buffer = (char*)text.data();
	buffer->current_size = text.size();
	owned_data.reset(buffer);

	this->start_buffer = buffer;
	this->start_position = 0;
	this->end_buffer = buffer;
	this->end_position = buffer->current_size;
}

PGTextRange::PGTextRange(PGTextPosition start, PGTextPosition end) :
	start_buffer(start.buffer), start_position(start.position),
	end_buffer(end.buffer), end_position(end.position), owned_data(nullptr) {
}

PGTextRange::PGTextRange(PGTextBuffer* start_buffer, lng start_position, PGTextBuffer* end_buffer, lng end_position) :
	start_buffer(start_buffer), start_position(start_position),
	end_buffer(end_buffer), end_position(end_position), owned_data(nullptr) {

}

PGTextRange::PGTextRange() : 
	start_buffer(nullptr), start_position(0), 
	end_buffer(nullptr), end_position(0), owned_data(nullptr) {
}

std::string PGTextRange::GetString() const {
	if (!start_buffer || !end_buffer) {
		return std::string();
	}
	if (start_buffer == end_buffer) {
		return std::string(start_buffer->buffer + start_position, end_position - start_position);
	}
	size_t size = 0;
	PGTextBuffer* buffer = start_buffer;
	lng position = start_position;
	while (buffer != end_buffer) {
		size += buffer->current_size - position;
		position = 0;
		buffer = buffer->next;
	}
	size += end_position - position;
	char* data = (char*)malloc(size * sizeof(char) + 1);
	if (!data) {
		return std::string();
	}
	char* current_data = data;
	buffer = start_buffer;
	position = start_position;
	while (buffer != end_buffer) {
		size_t size = buffer->current_size - position;
		memcpy(current_data, buffer->buffer + position, size);
		current_data += size;
		position = 0;
		buffer = buffer->next;
	}
	memcpy(current_data, buffer->buffer + position, end_position - position);
	std::string result = std::string(data, size);
	free(data);
	return result;
}

void PGTextRange::remove_prefix(size_t length) {
	while (start_buffer) {
		size_t buffer_left = start_buffer->current_size - start_position;
		if (length > buffer_left) {
			length -= buffer_left;
			start_buffer = start_buffer->next;
			start_position = 0;
		} else {
			start_position += length;
			return;
		}
	}
	// prefix was longer than the buffer
	start_buffer = nullptr;
	start_position = 0;
}


// Avoid possible locale nonsense in standard strcasecmp.
// The string a is known to be all lowercase.
static int _ascii_strcasecmp(const char* a, const char* b, size_t len) {
	const char *ae = a + len;

	for (; a < ae; a++, b++) {
		uint8_t x = *a;
		uint8_t y = *b;
		if ('A' <= y && y <= 'Z')
			y += 'a' - 'A';
		if (x != y)
			return x - y;
	}
	return 0;
}

#define BUFFER_COMPARISON(function) \
  PGTextBuffer* buffer = start_buffer; \
  lng start = start_position; \
  while(buffer) { \
    lng buffer_left = (buffer == end_buffer ?  end_position : buffer->current_size) - start; \
    if (buffer_left < (lng) length) { \
      /* length is bigger than buffer length; consume the entire buffer */ \
      /* then check the next buffer */ \
      int retval = function(data, buffer->buffer + start, buffer_left); \
      if (retval != 0) { \
        /* early out if the comparison fails */ \
        return retval; \
      } \
      data += buffer_left;  \
      length -= buffer_left; \
    } else { \
      /* length fits within the buffer, finish the comparison */ \
      return function(data, buffer->buffer + start, length); \
    } \
    if (buffer == end_buffer) { \
      break; \
    } \
    buffer = buffer->next; \
    start = 0; \
  } \
  /* we ran out of text */ \
  return -1;

int PGTextRange::ascii_strcasecmp(const char* data, size_t length) const {
	BUFFER_COMPARISON(_ascii_strcasecmp);
}

int PGTextRange::_memcmp(const char* data, size_t length) const {
	BUFFER_COMPARISON(memcmp);
}

#if !defined(__linux__)  /* only Linux seems to have memrchr */
static void* memrchr(const void* s, int c, size_t n) {
	const unsigned char* p = (const unsigned char*)s;
	for (p += n; n > 0; n--)
		if (*--p == c)
			return (void*)p;

	return NULL;
}
#endif

PGTextPosition PGTextRange::_memrchr(int value) const {
	PGTextBuffer* buffer = end_buffer;
	lng end = end_position;
	while (buffer) {
		lng start = (buffer == start_buffer ? start_position : 0);
		void* ptr = memrchr(buffer->buffer + start, value, end - start);
		if (ptr != nullptr) {
			return PGTextPosition(buffer, (char*)ptr);
		}
		if (buffer == start_buffer) {
			break;
		}
		buffer = buffer->next;
		end = buffer->current_size;
	}
	return PGTextPosition(nullptr, (lng)0);
}


PGTextPosition PGTextRange::_memchr(int value) const {
	PGTextBuffer* buffer = start_buffer;
	lng start = start_position;
	while (buffer) {
		lng buffer_left = (buffer == end_buffer ? end_position : buffer->current_size) - start;
		void* ptr = memchr(buffer->buffer + start, value, buffer_left);
		if (ptr != nullptr) {
			return PGTextPosition(buffer, (char*)ptr);
		}
		if (buffer == end_buffer) {
			break;
		}
		buffer = buffer->next;
		start = 0;
	}
	return PGTextPosition(nullptr, (lng)0);
}