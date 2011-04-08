#ifndef __MAP_H__
#define __MAP_H__

#include "config.h"
#include <map>
#include <boost/shared_ptr.hpp>

/**
  * Map is a Java-inspired std::map wrapper with a reference-counted 
  * pointer semantics.
  */
template<class KeyType, class MappedType, class Comparator = std::less<KeyType>, class Allocator = std::allocator<std::pair<const KeyType, MappedType> > > class Map {
private:
  typedef std::map<KeyType, MappedType, Comparator, Allocator> container_type;
  boost::shared_ptr<container_type> impl;
public:
  typedef typename container_type::key_type key_type;
  typedef typename container_type::mapped_type mapped_type;
  typedef typename container_type::referent_type referent_type;
  typedef typename container_type::key_compare key_compare;
  typedef typename container_type::value_compare value_compare;
  typedef typename container_type::allocator_type allocator_type;
  typedef typename container_type::size_type size_type;
  typedef typename container_type::difference_type difference_type;
  typedef typename container_type::pointer pointer;
  typedef typename container_type::const_pointer const_pointer;
  typedef typename container_type::reference reference;
  typedef typename container_type::const_reference const_reference;
  typedef typename container_type::iterator iterator;
  typedef typename container_type::const_iterator const_iterator;
  typedef typename container_type::reverse_iterator reverse_iterator;
  typedef typename container_type::const_reverse_iterator const_reverse_iterator;
  typedef typename container_type::value_type value_type;

  Map(): impl(new container_type()) {}

  void erase(iterator pos) {
    this->impl->erase(pos);
  }

  size_type erase(const key_type& key) {
    return this->impl->erase(key);
  }

  void erase(iterator first, iterator last) {
    return this->impl->erase(first, last);
  }

  mapped_type& operator[](const key_type& key) {
    return this->impl->operator[](key);
  }

  iterator begin() {
    return this->impl->begin();
  }

  const_iterator begin() const {
    return this->impl->begin();
  }

  iterator end() {
    return this->impl->end();
  }

  const_iterator end() const {
    return this->impl->end();
  }

  reverse_iterator rbegin() {
    return this->impl->rbegin();
  }

  const_reverse_iterator rbegin() const {
    return this->impl->rbegin();
  }

  reverse_iterator rend() {
    return this->impl->rend();
  }

  const_reverse_iterator rend() const {
    return this->impl->rend();
  }

  size_type size() const {
    return this->impl->size();
  }

  bool empty() const {
    return this->impl->empty();
  }

  std::pair<iterator, bool> insert(const value_type& value) {
    return this->impl->insert(value);
  }

  void clear() {
    this->impl->clear();
  }

  iterator find(const key_type& key) {
    return this->impl->find(key);
  }

  const_iterator find(const key_type& _Keyval) const {
    return this->impl->find(key);
  }
};
  
  
#endif // __MAP_H__
