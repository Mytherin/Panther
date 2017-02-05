// Copyright 2004 The RE2 Authors.  All Rights Reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include "re2/stringpiece.h"

#include <ostream>

#include "util/util.h"

void PGRegexContext::remove_prefix(size_t length) {
  while(start_buffer) {
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

int PGRegexContext::ascii_strcasecmp(const char* data, size_t length) const {
  BUFFER_COMPARISON(_ascii_strcasecmp);
}

int PGRegexContext::_memcmp(const char* data, size_t length) const {
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

PGTextPosition PGRegexContext::_memrchr(int value) const {
  PGTextBuffer* buffer = end_buffer;
  lng end = end_position;
  while(buffer) {
    lng start = (buffer == start_buffer ?  start_position : 0);
    void* ptr = memrchr(buffer->buffer + start, value, end - start);
    if (ptr != nullptr) {
      return PGTextPosition(buffer, (char*) ptr);
    }
    if (buffer == start_buffer) {
      break;
    }
    buffer = buffer->next;
    end = buffer->current_size;
  }
  return PGTextPosition(nullptr, (lng) 0);
}


PGTextPosition PGRegexContext::_memchr(int value) const {
  PGTextBuffer* buffer = start_buffer;
  lng start = start_position;
  while(buffer) {
    lng buffer_left = (buffer == end_buffer ?  end_position : buffer->current_size) - start;
    void* ptr = memchr(buffer->buffer + start, value, buffer_left);
    if (ptr != nullptr) {
      return PGTextPosition(buffer, (char*) ptr);
    }
    if (buffer == end_buffer) {
      break;
    }
    buffer = buffer->next;
    start = 0;
  }
  return PGTextPosition(nullptr, (lng) 0);
}

namespace re2 {

const StringPiece::size_type StringPiece::npos;  // initialized in stringpiece.h

StringPiece::size_type StringPiece::copy(char* buf, size_type n,
                                         size_type pos) const {
  size_type ret = std::min(size_ - pos, n);
  memcpy(buf, data_ + pos, ret);
  return ret;
}

StringPiece StringPiece::substr(size_type pos, size_type n) const {
  if (pos > size_) pos = size_;
  if (n > size_ - pos) n = size_ - pos;
  return StringPiece(data_ + pos, n);
}

StringPiece::size_type StringPiece::find(const StringPiece& s,
                                         size_type pos) const {
  if (pos > size_) return npos;
  const_pointer result = std::search(data_ + pos, data_ + size_,
                                     s.data_, s.data_ + s.size_);
  size_type xpos = result - data_;
  return xpos + s.size_ <= size_ ? xpos : npos;
}

StringPiece::size_type StringPiece::find(char c, size_type pos) const {
  if (size_ <= 0 || pos >= size_) return npos;
  const_pointer result = std::find(data_ + pos, data_ + size_, c);
  return result != data_ + size_ ? result - data_ : npos;
}

StringPiece::size_type StringPiece::rfind(const StringPiece& s,
                                          size_type pos) const {
  if (size_ < s.size_) return npos;
  if (s.size_ == 0) return std::min(size_, pos);
  const_pointer last = data_ + std::min(size_ - s.size_, pos) + s.size_;
  const_pointer result = std::find_end(data_, last, s.data_, s.data_ + s.size_);
  return result != last ? result - data_ : npos;
}

StringPiece::size_type StringPiece::rfind(char c, size_type pos) const {
  if (size_ <= 0) return npos;
  for (size_t i = std::min(pos + 1, size_); i != 0;) {
    if (data_[--i] == c) return i;
  }
  return npos;
}

std::ostream& operator<<(std::ostream& o, const StringPiece& p) {
  o.write(p.data(), p.size());
  return o;
}

}  // namespace re2
