#pragma once

#include <atomic>
#include <cassert>
#include <cstdlib>

namespace test {

template <typename T, class Comparator>
class OrderedList {
 private:
  struct Node;

 public:
  explicit OrderedList(Comparator compare);

  OrderedList(const OrderedList&) = delete;
  OrderedList& operator=(const OrderedList&) = delete;

  void Insert(const T& key);
  bool Contains(const T& key) const;

  class Iterator {
   public:
    explicit Iterator(const OrderedList* list);

    bool Valid() const;

    const T& key() const;

    void Next();
    void Prev();
    void Seek(const T& target);
    void SeekToFirst();
    void SeekToLast();
   
    T& operator* ();
    Iterator& operator++ ();
    bool operator!= (const Iterator&);

   private:
    const OrderedList* list_;
    Node* node_;
  };

  Iterator& begin();
  Iterator& end();

 private:
  Node* NewNode(const T& key);
  
  bool Equal(const T& a, const T& b) const { return (compare_(a, b) == 0); }
  bool KeyIsAfterNode(const T& key, Node* n) const;

  Node* FindGreaterOrEqual(const T& key, Node** prev) const;
  Node* FindLessThan(const T& key) const;
  Node* FindLast() const;

  Iterator begin_;
  Iterator end_;
  Comparator const compare_;
  Node* const head_;  
};

template <typename T, class Comparator>
struct OrderedList<T, Comparator>::Node {
  explicit Node(const T& k) : key(k) {}

  T key;

  Node* Next() {
    return next_.load(std::memory_order_acquire);
  }

  Node* NoBarrierNext() {
    return next_.load(std::memory_order_relaxed);
  }

  void SetNext(Node* x) {
    next_.store(x, std::memory_order_release);
  }

  void NoBarrierSetNext(Node* x) {
    next_.store(x, std::memory_order_relaxed);
  }

 private:
  std::atomic<Node*> next_;
};

template <typename T, class Comparator>
typename OrderedList<T, Comparator>::Node* OrderedList<T, Comparator>::NewNode(
    const T& key) {
  return new Node(key);
}

template <typename T, class Comparator>
bool OrderedList<T, Comparator>::KeyIsAfterNode(const T& key, Node* n) const {
  return (n != nullptr) && (compare_(n->key, key) < 0);
}

template <typename T, class Comparator>
typename OrderedList<T, Comparator>::Node*
OrderedList<T, Comparator>::FindGreaterOrEqual(const T& key, 
                                               Node** prev) const {
  Node* x = head_;
  while(x != nullptr && KeyIsAfterNode(key, x)) {
    if (prev != nullptr) *prev = x;
    x = x->Next();
  }
  return x;
}

template <typename T, class Comparator>
typename OrderedList<T, Comparator>::Node*
OrderedList<T, Comparator>::FindLessThan(const T& key) const {
  Node* x = head_;
  while(x != nullptr && compare_(x->Next()->key, key) < 0) {
    x = x->Next();
  }
  return x;
}

template <typename T, class Comparator>
typename OrderedList<T, Comparator>::Node*
OrderedList<T, Comparator>::FindLast() const {
  Node* x = head_;
  while(x != nullptr && x->Next() != nullptr) {
    x = x->Next();
  }
  return x;
}

template <typename T, class Comparator>
OrderedList<T, Comparator>::OrderedList(Comparator compare)
    : compare_(compare),
      head_(NewNode(0)),
      begin_(this),
      end_(this) {
  head_->SetNext(nullptr);
  begin_.SeekToFirst();
}

template <typename T, class Comparator>
void OrderedList<T, Comparator>::Insert(const T& key) {
  Node* prev = nullptr;
  Node* x = FindGreaterOrEqual(key, &prev);
  assert(x == nullptr || !Equal(key, x->key));
  x = NewNode(key);
  x->NoBarrierSetNext(prev->NoBarrierNext());
  prev->SetNext(x);
}

template <typename T, class Comparator>
bool OrderedList<T, Comparator>::Contains(const T& key) const {
  Node* x = FindGreaterOrEqual(key, nullptr);
  return (x != nullptr && Equal(key, x->key));
}

template <typename T, class Comparator>
typename OrderedList<T, Comparator>::Iterator&
OrderedList<T, Comparator>::begin() {
  begin_.SeekToFirst();
  return begin_;
}

template <typename T, class Comparator>
typename OrderedList<T, Comparator>::Iterator&
OrderedList<T, Comparator>::end() {
  return end_;
}

template <typename T, class Comparator>
inline OrderedList<T, Comparator>::Iterator::Iterator(const OrderedList* list) {
  list_ = list;
  node_ = nullptr;
}

template <typename T, class Comparator>
inline bool OrderedList<T, Comparator>::Iterator::Valid() const {
  return node_ != nullptr;
}

template <typename T, class Comparator>
inline const T& OrderedList<T, Comparator>::Iterator::key() const {
  assert(Valid());
  return node_->key;
}

template <typename T, class Comparator>
inline void OrderedList<T, Comparator>::Iterator::Next() {
  assert(Valid());
  node_ = node_->Next();
}

template <typename T, class Comparator>
inline void OrderedList<T, Comparator>::Iterator::Prev() {
  assert(Valid());
  node_ = list_->FindLessThan(node_->key);
  if (node_ == list_->head_) {
    node_ = nullptr;
  }
}

template <typename T, class Comparator>
inline void OrderedList<T, Comparator>::Iterator::SeekToFirst() {
  node_ = list_->head_->Next();
}

template <typename T, class Comparator>
inline void OrderedList<T, Comparator>::Iterator::SeekToLast() {
  node_ = list_->FindLast();
  if (node_ == list_->head_) {
    node_ = nullptr;
  }
}

template <typename T, class Comparator>
T& OrderedList<T, Comparator>::Iterator::operator* () {
  assert(Valid());
  return node_->key;
}

template <typename T, class Comparator>
typename OrderedList<T, Comparator>::Iterator& 
OrderedList<T, Comparator>::Iterator::operator++ () {
  assert(Valid());
  node_ = node_->Next();
  return *this;
}

template <typename T, class Comparator>
bool OrderedList<T, Comparator>::Iterator::operator!= (
    const OrderedList<T, Comparator>::Iterator& iter) {
  return node_ != iter.node_;
}

} // namespace test