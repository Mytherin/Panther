// Copyright 2001-2010 The RE2 Authors.  All Rights Reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef RE2_STRINGPIECE_H_
#define RE2_STRINGPIECE_H_

// A string-like object that points to a sized piece of memory.
//
// Functions or methods may use const StringPiece& parameters to accept either
// a "const char*" or a "string" value that will be implicitly converted to
// a StringPiece.  The implicit conversion means that it is often appropriate
// to include this .h file in other files rather than forward-declaring
// StringPiece as would be appropriate for most other Google classes.
//
// Systematic usage of StringPiece is encouraged as it will reduce unnecessary
// conversions from "const char*" to "string" and back again.
//
//
// Arghh!  I wish C++ literals were "string".

#include <stddef.h>
#include <string.h>
#include <algorithm>
#include <iosfwd>
#include <iterator>
#include <string>
#include <malloc.h>

#include "../../../utils.h"
#include "../../../textbuffer.h"

struct PGTextPosition {
  PGTextBuffer* buffer = nullptr;
  lng position = 0;

  lng GetPosition(PGTextBuffer* start_buffer) {
    lng current_position = 0;
    PGTextBuffer* current_buffer = start_buffer;
    while(current_buffer != buffer) {
      current_position += current_buffer->current_size;
      current_buffer = current_buffer->next;
    }
    current_position += position;
    return current_position;
  }

  char* data() { return buffer->buffer + position; }

  PGTextPosition(PGTextBuffer* buffer, const char* ptr) : buffer(buffer), position(ptr - buffer->buffer) { }
  PGTextPosition() : buffer(nullptr), position(0) { }
  PGTextPosition(PGTextBuffer* buffer) : buffer(buffer), position(0) { }
  PGTextPosition(PGTextBuffer* buffer, lng position) : buffer(buffer), position(position) { }

  inline char operator* (){ return buffer ? buffer->buffer[position] : '\0'; }


  void Offset(lng offset) {
    if (buffer == nullptr) {
      return;
    }
    position += offset;
    while(position >= (lng) buffer->current_size) {
      if (!buffer->next) return;
      position -= buffer->current_size;
      buffer = buffer->next;
    }
    while(position < 0) {
      buffer = buffer->prev;
      if (!buffer) return;
      position += buffer->current_size;
    }
  }
};

inline bool operator< (const PGTextPosition& lhs, const PGTextPosition& rhs) { 
  return (lhs.buffer->start_line < rhs.buffer->start_line) || 
    (lhs.buffer->start_line == rhs.buffer->start_line && lhs.position < rhs.position);
}
inline bool operator> (const PGTextPosition& lhs, const PGTextPosition& rhs){ return rhs < lhs; }
inline bool operator<=(const PGTextPosition& lhs, const PGTextPosition& rhs){ return !(lhs > rhs); }
inline bool operator>=(const PGTextPosition& lhs, const PGTextPosition& rhs){ return !(lhs < rhs); }
inline bool operator== (const PGTextPosition& lhs, const PGTextPosition& rhs){ 
  return (lhs.buffer == rhs.buffer && lhs.position == rhs.position);
}
inline bool operator!= (const PGTextPosition& lhs, const PGTextPosition& rhs){ return !(lhs == rhs); }


inline PGTextPosition operator+ (PGTextPosition lhs, lng rhs) {
  lhs.Offset(rhs);
  return lhs;
}
inline PGTextPosition operator- (const PGTextPosition& lhs, lng rhs) {
  return lhs + (-rhs);
}

#include <memory>

namespace re2 {

class StringPiece {
 public:
  typedef char value_type;
  typedef char* pointer;
  typedef const char* const_pointer;
  typedef char& reference;
  typedef const char& const_reference;
  typedef const char* const_iterator;
  typedef const_iterator iterator;
  typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
  typedef const_reverse_iterator reverse_iterator;
  typedef size_t size_type;
  typedef ptrdiff_t difference_type;
  static const size_type npos = static_cast<size_type>(-1);

  // We provide non-explicit singleton constructors so users can pass
  // in a "const char*" or a "string" wherever a "StringPiece" is
  // expected.
  StringPiece()
      : data_(NULL), size_(0), owned_data_(nullptr) {}
  StringPiece(const std::string& str)
      : data_(str.data()), size_(str.size()), owned_data_(nullptr) {}
  StringPiece(const char* str)
      : data_(str), size_(str == NULL ? 0 : strlen(str)), owned_data_(nullptr) {}
  StringPiece(const char* str, size_type len, bool own_data = false)
      : data_(str), size_(len), owned_data_(own_data ? (char*) str : nullptr) {}


  const_iterator begin() const { return data_; }
  const_iterator end() const { return data_ + size_; }
  const_reverse_iterator rbegin() const {
    return const_reverse_iterator(data_ + size_);
  }
  const_reverse_iterator rend() const {
    return const_reverse_iterator(data_);
  }

  size_type size() const { return size_; }
  size_type length() const { return size_; }
  bool empty() const { return size_ == 0; }

  const_reference operator[](size_type i) const { return data_[i]; }
  const_pointer data() const { return data_; }

  void remove_prefix(size_type n) {
    data_ += n;
    size_ -= n;
  }

  void remove_suffix(size_type n) {
    size_ -= n;
  }

  void set(const char* str) {
    data_ = str;
    size_ = str == NULL ? 0 : strlen(str);
  }

  void set(const char* str, size_type len) {
    data_ = str;
    size_ = len;
  }

  std::string as_string() const {
    return std::string(data_, size_);
  }

  // We also define ToString() here, since many other string-like
  // interfaces name the routine that converts to a C++ string
  // "ToString", and it's confusing to have the method that does that
  // for a StringPiece be called "as_string()".  We also leave the
  // "as_string()" method defined here for existing code.
  std::string ToString() const {
    return std::string(data_, size_);
  }

  void CopyToString(std::string* target) const {
    target->assign(data_, size_);
  }

  void AppendToString(std::string* target) const {
    target->append(data_, size_);
  }

  size_type copy(char* buf, size_type n, size_type pos = 0) const;
  StringPiece substr(size_type pos = 0, size_type n = npos) const;

  int compare(const StringPiece& x) const {
    size_type min_size = std::min(size(), x.size());
    if (min_size > 0) {
      int r = memcmp(data(), x.data(), min_size);
      if (r < 0) return -1;
      if (r > 0) return 1;
    }
    if (size() < x.size()) return -1;
    if (size() > x.size()) return 1;
    return 0;
  }

  // Does "this" start with "x"?
  bool starts_with(const StringPiece& x) const {
    return x.empty() ||
           (size() >= x.size() && memcmp(data(), x.data(), x.size()) == 0);
  }

  // Does "this" end with "x"?
  bool ends_with(const StringPiece& x) const {
    return x.empty() ||
           (size() >= x.size() &&
            memcmp(data() + (size() - x.size()), x.data(), x.size()) == 0);
  }

  bool contains(const StringPiece& s) const {
    return find(s) != npos;
  }

  size_type find(const StringPiece& s, size_type pos = 0) const;
  size_type find(char c, size_type pos = 0) const;
  size_type rfind(const StringPiece& s, size_type pos = npos) const;
  size_type rfind(char c, size_type pos = npos) const;

 private:
  const_pointer data_;
  size_type size_;
  std::shared_ptr<char> owned_data_;
};

inline bool operator==(const StringPiece& x, const StringPiece& y) {
  StringPiece::size_type len = x.size();
  if (len != y.size()) return false;
  return x.data() == y.data() || len == 0 ||
         memcmp(x.data(), y.data(), len) == 0;
}

inline bool operator!=(const StringPiece& x, const StringPiece& y) {
  return !(x == y);
}

inline bool operator<(const StringPiece& x, const StringPiece& y) {
  StringPiece::size_type min_size = std::min(x.size(), y.size());
  int r = min_size == 0 ? 0 : memcmp(x.data(), y.data(), min_size);
  return (r < 0) || (r == 0 && x.size() < y.size());
}

inline bool operator>(const StringPiece& x, const StringPiece& y) {
  return y < x;
}

inline bool operator<=(const StringPiece& x, const StringPiece& y) {
  return !(x > y);
}

inline bool operator>=(const StringPiece& x, const StringPiece& y) {
  return !(x < y);
}

// Allow StringPiece to be logged.
std::ostream& operator<<(std::ostream& o, const StringPiece& p);

}  // namespace re2


struct PGRegexContext {
  PGTextBuffer* start_buffer;
  lng start_position;
  PGTextBuffer* end_buffer;
  lng end_position;

  PGRegexContext(re2::StringPiece text) {
    PGTextBuffer* buffer = new PGTextBuffer(text.data(), text.size(), 0);
    buffer->prev = nullptr;
    buffer->next = nullptr;
    buffer->buffer = (char*) text.data();
    buffer->current_size = text.size();

    this->start_buffer = buffer;
    this->start_position = 0;
    this->end_buffer = buffer;
    this->end_position = buffer->current_size;
  }

  PGRegexContext(PGTextPosition start, PGTextPosition end) : 
    start_buffer(start.buffer), start_position(start.position),
    end_buffer(end.buffer), end_position(end.position) { }

  PGRegexContext() : start_buffer(nullptr), start_position(0), end_buffer(nullptr), end_position(0) { }

  re2::StringPiece GetString() const {
    if (!start_buffer || !end_buffer) {
      return re2::StringPiece();
    }
    if (start_buffer == end_buffer) {
      return re2::StringPiece(start_buffer->buffer + start_position, end_position - start_position);
    }
    size_t size = 0;
    PGTextBuffer* buffer = start_buffer;
    lng position = start_position;
    while(buffer != end_buffer) {
      size += buffer->current_size - position;
      position = 0;
      buffer = buffer->next;
    }
    size += end_position - position;
    char* data = (char*) malloc(size * sizeof(char) + 1);
    if (!data) {
      return re2::StringPiece();
    }
    char* current_data = data;
    buffer = start_buffer;
    position = start_position;
    while(buffer != end_buffer) {
      size_t size = buffer->current_size - position;
      memcpy(current_data, buffer->buffer + position, size);
      current_data += size;
      position = 0;
      buffer = buffer->next;
    }
    memcpy(current_data, buffer->buffer + position, end_position - position);
    re2::StringPiece result = re2::StringPiece(data, size, true);
    return result;
  }

  // "data" has to be lowercase
  int ascii_strcasecmp(const char* data, size_t length) const;
  int _memcmp(const char* data, size_t length) const;
  PGTextPosition _memchr(int value) const;
  PGTextPosition _memrchr(int value) const;


  PGTextPosition startpos() const { return PGTextPosition(start_buffer, start_position); }
  PGTextPosition endpos() const { return PGTextPosition(end_buffer, end_position); }

  char* begin() const { return start_buffer->buffer + start_position; }
  char* end_start() const { return 
    start_buffer == end_buffer ? 
    start_buffer->buffer + end_position : 
    start_buffer->buffer + start_buffer->current_size; }
  char* end() const { return end_buffer->buffer + end_position; }

  void remove_prefix(size_t length);
};

#endif  // RE2_STRINGPIECE_H_
