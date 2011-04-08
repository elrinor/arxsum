#ifndef __COLLECTIONS_H__
#define __COLLECTIONS_H__

#include <vector>
#include <memory>
#include "shared_ptr.h"

namespace arx {

/**
 * ArrayList is a Java-inspired std::vector wrapper with a reference-counted 
 * pointer semantics.
 */
template<class Type, class Allocator = std::allocator<Type> > class ArrayList {
private:
  typedef std::vector<Type, Allocator> container_type;
  shared_ptr<container_type> vec;
public:
  typedef typename container_type::allocator_type allocator_type;
  typedef typename container_type::const_pointer const_pointer;
  typedef typename container_type::const_reference const_reference;
  typedef typename container_type::pointer pointer;
  typedef typename container_type::reference reference;
  typedef typename container_type::value_type value_type;
  typedef typename container_type::size_type size_type;
  typedef typename container_type::difference_type difference_type;
  
  typedef typename container_type::const_iterator const_iterator;
  typedef typename container_type::const_reverse_iterator const_reverse_iterator;
  typedef typename container_type::iterator iterator;
  typedef typename container_type::reverse_iterator reverse_iterator;

  ArrayList(): vec(new container_type()) {}

  reference at(size_type index) {
    return vec->at(index);
  }

  const_reference at(size_type index) const {
    return vec->at(index);
  }

  const_iterator begin() const {
    return vec->begin();
  }

  iterator begin() {
    return vec->begin();
  }

  void clear() {
    vec->clear();
  }

  bool empty() const {
    return vec->empty();
  }

  iterator end() {
    return vec->end();
  }

  const_iterator end() const {
    return vec->end();
  }

  reverse_iterator rbegin() {
    return vec->rbegin();
  }

  const_reverse_iterator rbegin() const {
    return vec->rbegin();
  }

  const_reverse_iterator rend() const {
    return vec->rend();
  }

  reverse_iterator rend() {
    return vec->rend();
  }

  size_type size() const {
    return vec->size();
  }

  value_type& operator[] (size_type index) {
    return vec->operator[](index);
  }

  const value_type& operator[] (size_type index) const {
    return vec->operator[](index);
  }

  void push_back(const value_type& val) {
    vec->push_back(val);
  }

  void push_back() {
    vec->push_back(value_type());
  }

  void add(const value_type& val) {
    this->push_back(val);
  }

  void add() {
    this->push_back();
  }

  iterator erase(iterator pos) {
    return vec->erase(pos);
  }

  iterator erase(iterator first, iterator last) {
    return vec->erase(first, last);
  }

  iterator erase(size_type pos) {
    return erase(begin() + pos);
  }

  iterator erase(size_type first, size_type last) {
    return erase(begin() + first, begin() + last);
  }

  void resize(size_type newSize, value_type filler) {
    vec->resize(newSize, filler);
  }

  void resize(size_type newSize) {
    if(newSize <= size())
      vec->resize(newSize);
    else while(size() != newSize)
      add(value_type());
  }
};

/**
 * ArraySlice wraps around stl container and provides a simple interface to
 * access an interval of this container in an stl-like manner. It operates with
 * indexes, not iterators, and therefore manipulations with the container which
 * ArraySlice was created from does not invalidate the ArraySlice instance.
 * WARNING: ArraySlice stores a reference to the target container, so if the target 
 * container variable leaves scope, ArraySlice becomes invalid.
 */
template<class T> class ArraySlice {
public:
  typedef typename T::value_type value_type;
  typedef typename T::size_type size_type;
private:
  T& a;
  size_type lo, hi;
  ArraySlice(T& a, size_type lo, size_type hi) : a(a), lo(lo), hi(hi) {}
  template<class Y> friend ArraySlice<Y> createSlice(Y&, typename Y::size_type, typename Y::size_type);
  template<class Y> friend ArraySlice<Y> createSlice(ArraySlice<Y>&, typename ArraySlice<Y>::size_type, typename ArraySlice<Y>::size_type);
public:
  value_type& operator[] (int index) {
    return a[this->lo + index];
  }
  const value_type& operator[] (int index) const {
    return a[this->lo + index];
  }
  size_type size() const {
    return this->hi - this->lo;
  }
};

/**
 * Specialization for ArrayList class which have a reference semantics.
 * This is a silly workaround for the problem described above (see WARNING in
 * ArraySlice<T> description).
 */
template<class T> class ArraySlice<ArrayList<T> > {
public:
  typedef typename ArrayList<T>::value_type value_type;
  typedef typename ArrayList<T>::size_type size_type;
private:
  ArrayList<T> a;
  size_type lo, hi;
  ArraySlice(ArrayList<T> a, size_type lo, size_type hi) : a(a), lo(lo), hi(hi) {}
  template<class Y> friend ArraySlice<Y> createSlice(Y&, typename Y::size_type, typename Y::size_type);
  template<class Y> friend ArraySlice<Y> createSlice(ArraySlice<Y>&, typename ArraySlice<Y>::size_type, typename ArraySlice<Y>::size_type);
public:
  value_type& operator[] (int index) {
    return a[this->lo + index];
  }
  const value_type& operator[] (int index) const {
    return a[this->lo + index];
  }
  size_type size() const {
    return this->hi - this->lo;
  }
};

template<class Y>
ArraySlice<Y> createSlice(Y& a, typename Y::size_type lo, typename Y::size_type hi) {
  return ArraySlice<Y>(a, lo, hi);
}
template<class Y>
ArraySlice<Y> createSlice(ArraySlice<Y>& a, typename ArraySlice<Y>::size_type lo, typename ArraySlice<Y>::size_type hi) {
  return ArraySlice<Y>(a.a, a.lo + lo, a.lo + hi);
}


/**
* Sorted list of fixed size that stores only the largest items.
* Has a reference-counted pointer semantics.
*/
template<class T> class KSortedList {
private:
  class KSortedListImpl {
  public:
    T* vec;
    T* p;
    int maxSize;
    int count;
    KSortedListImpl(int maxSize): vec(new T[maxSize]), p(vec), maxSize(maxSize), count(0) {}
    ~KSortedListImpl() {
      delete[] p;
    }
  };
  shared_ptr<KSortedListImpl> impl;
public:
  KSortedList(int maxSize) : impl(new KSortedListImpl(maxSize)) {}
  const T& operator[] (int index) const {
    return impl->vec[index];
  }
  int size() const {
    return impl->count;
  }
  int maxSize() const {
    return impl->maxSize;
  }
  void add(T elem) {
    int pos = impl->count;
    while(pos > 0 && !(impl->vec[pos - 1] < elem)) {
      if(pos < impl->maxSize)
        impl->vec[pos] = impl->vec[pos - 1];
      pos--;
    }
    if(pos < impl->maxSize)
      impl->vec[pos] = elem;
    if(impl->count < impl->maxSize)
      impl->count++;
  }
  void slide() {
    impl->vec++;
    impl->maxSize--;
    impl->count--;
  }
};

} // namespace arx

#endif
