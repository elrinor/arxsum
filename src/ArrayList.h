#ifndef __ARRAY_LIST_H__
#define __ARRAY_LIST_H__

#include "config.h"
#include <boost/shared_ptr.hpp>
#include <vector>

/**
  * ArrayList is a Java-inspired std::vector wrapper with a reference-counted 
  * pointer semantics.
  */
template<class Type, class Allocator = std::allocator<Type> > class ArrayList {
private:
  typedef std::vector<Type, Allocator> container_type;
  boost::shared_ptr<container_type> vec;
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
    vec->resize(newSize);
  }

  int indexOf(const value_type& value) const {
    for(unsigned int i = 0; i < size(); i++)
      if(this->operator[](i) == value)
        return i;
    return -1;
  }

  int lastIndexOf(const value_type& value) {
    for(unsigned int i = size() - 1; i >= 0; i--)
      if(this->operator[](i) == value)
        return i;
    return -1;
  }

  bool contains(const value_type& value) const {
    return indexOf(value) != -1;
  }

  /*value_type remove(size_type index) {
    value_type value = this->operator[](index);
    erase(index);
    return value;
  }*/

  bool remove(const value_type& value) {
    int index = indexOf(value);
    if(index < 0)
      return false;
    else {
      erase(index);
      return true;
    }
  }
};

#endif // __ARRAY_LIST_H__
