#ifndef SPAN_HPP
#define SPAN_HPP

#include <functional>

template <typename T>
class Span {
  T* begin_;
  T* end_;

public:
  Span() : begin_(nullptr), end_(nullptr) { }
  Span(T* begin, T* end) : begin_(begin), end_(end) { }
  Span(T* begin, size_t size) : begin_(begin), end_(begin + size) { }

  inline T* begin() const { return begin_; }
  inline T* end() const { return end_; }

  inline T* begin() { return begin_; }
  inline T* end() { return end_; }

  inline size_t len() { return end() - begin(); }
  T& operator [] (size_t index) { return begin()[index]; }
};


template <typename T>
T* find_single_in_span(Span<T> ts, std::function<bool(const T&)> predicate) {
  T* ans = nullptr;
  for (auto& t : ts) {
    if (predicate(t)) {
      assert(ans == nullptr && "Multiple items match the criteria!");
      ans = &t;
    }
  }
  assert(ans != nullptr && "No section matches the criteria!");
  return ans;
}

#endif