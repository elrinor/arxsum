#ifndef __ARX_COLLECTIONS_H__
#define __ARX_COLLECTIONS_H__

#include "config.h"
#include <map>
#include <vector>
#include "smart_ptr.h"
#include "Mpl.h"

#ifdef ARX_USE_BOOST
#  include <boost/array.hpp>
namespace arx {
  using boost::array;
}
#else
namespace arx {
  /**
   * array template provides stl interface to c arrays.
   */
  template<class T, std::size_t N>
  class array {
  public:
    T elems[N];

  public:
    typedef T              value_type;
    typedef T*             iterator;
    typedef const T*       const_iterator;
    typedef T&             reference;
    typedef const T&       const_reference;
    typedef std::size_t    size_type;
    typedef std::ptrdiff_t difference_type;

    iterator begin()             { return elems; }
    const_iterator begin() const { return elems; }
    iterator end()               { return elems + N; }
    const_iterator end() const   { return elems + N; }

    typedef std::reverse_iterator<iterator> reverse_iterator;
    typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

    reverse_iterator rbegin()             { return reverse_iterator(end()); }
    const_reverse_iterator rbegin() const { return const_reverse_iterator(end()); }
    reverse_iterator rend()               { return reverse_iterator(begin()); }
    const_reverse_iterator rend() const   { return const_reverse_iterator(begin()); }

    reference operator[](size_type i) { 
      assert(i < N); 
      return elems[i];
    }

    const_reference operator[](size_type i) const {
      assert(i < N); 
      return elems[i];
    }

    reference at(size_type i) { rangecheck(i); return elems[i]; }
    const_reference at(size_type i) const { rangecheck(i); return elems[i]; }

    reference front()             { return elems[0]; }
    const_reference front() const { return elems[0]; }
    reference back()              { return elems[N - 1]; }
    const_reference back() const  { return elems[N - 1]; }

    static size_type size()     { return N; }
    static bool empty()         { return false; }
    static size_type max_size() { return N; }
    enum  { static_size = N };

    void swap (array<T, N>& y) {
      std::swap_ranges(begin(), end(), y.begin());
    }

    const T* data() const { return elems; }
    T* data()             { return elems; }
    T* c_array()          { return elems; }

    template <typename T2>
    array<T, N>& operator= (const array<T2, N>& r) {
      std::copy(r.begin(), r.end(), begin());
      return *this;
    }

    void assign(const T& value) {
      std::fill_n(begin(), size(), value);
    }

    static void rangecheck(size_type i) {
      if(i >= size())
        throw std::range_error("array<>: index out of range");
    }
  };

  template<class T>
  class array<T, 0> {
  public:
    typedef T              value_type;
    typedef T*             iterator;
    typedef const T*       const_iterator;
    typedef T&             reference;
    typedef const T&       const_reference;
    typedef std::size_t    size_type;
    typedef std::ptrdiff_t difference_type;

    iterator begin()             { return iterator(reinterpret_cast<T*>(this)); }
    const_iterator begin() const { return const_iterator(reinterpret_cast<const T*>(this)); }
    iterator end()               { return begin(); }
    const_iterator end() const   { return begin(); }

    typedef std::reverse_iterator<iterator> reverse_iterator;
    typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

    reverse_iterator rbegin()             { return reverse_iterator(end()); }
    const_reverse_iterator rbegin() const { return const_reverse_iterator(end()); }
    reverse_iterator rend()               { return reverse_iterator(begin()); }
    const_reverse_iterator rend() const   { return const_reverse_iterator(begin()); }

    reference operator[](size_type i)             { return failed_rangecheck(); }
    const_reference operator[](size_type i) const { return failed_rangecheck(); }
    reference at(size_type i)                     { return failed_rangecheck(); }
    const_reference at(size_type i) const         { return failed_rangecheck(); }
    reference front()                             { return failed_rangecheck(); }
    const_reference front() const                 { return failed_rangecheck(); }
    reference back()                              { return failed_rangecheck(); }
    const_reference back() const                  { return failed_rangecheck(); }

    static size_type size() { return 0; }
    static bool empty() { return true; }
    static size_type max_size() { return 0; }
    enum { static_size = 0 };

    void swap (array<T,0>& y) {}

    const T* data() const { return 0; }
    T* data()             { return 0; }
    T* c_array()          { return 0; }

    template <typename T2>
    array<T, 0>& operator= (const array<T2, 0>& ) { 
      return *this;
    }

    void assign (const T& ) {}

    static reference failed_rangecheck () {
      std::range_error e("attempt to access element of an empty array");
      throw e;
    }
  };

  template<class T, std::size_t N>
  bool operator== (const array<T, N>& x, const array<T, N>& y) {
    return std::equal(x.begin(), x.end(), y.begin());
  }
  template<class T, std::size_t N>
  bool operator< (const array<T, N>& x, const array<T, N>& y) {
    return std::lexicographical_compare(x.begin(),x.end(),y.begin(),y.end());
  }
  template<class T, std::size_t N>
  bool operator!= (const array<T, N>& x, const array<T, N>& y) {
    return !(x == y);
  }
  template<class T, std::size_t N>
  bool operator> (const array<T, N>& x, const array<T, N>& y) {
    return y < x;
  }
  template<class T, std::size_t N>
  bool operator<= (const array<T, N>& x, const array<T, N>& y) {
    return !(y < x);
  }
  template<class T, std::size_t N>
  bool operator>= (const array<T, N>& x, const array<T, N>& y) {
    return !(x < y);
  }

  template<class T, std::size_t N>
  inline void swap (array<T, N>& x, array<T, N>& y) {
    x.swap(y);
  }
} // namespace arx
#endif // ARX_USE_BOOST

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


  /**
   * Map is a Java-inspired std::map wrapper with a reference-counted 
   * pointer semantics.
   */
  template<class KeyType, class MappedType, class Comparator = std::less<KeyType>, class Allocator = std::allocator<std::pair<const KeyType, MappedType> > > class Map {
  private:
    typedef std::map<KeyType, MappedType, Comparator, Allocator> container_type;
    shared_ptr<container_type> impl;
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


  /**
   * ArraySlice wraps an stl container and provides a simple interface to
   * access an interval of this container. It operates with indexes, not iterators, 
   * and therefore manipulations with the container which ArraySlice was created 
   * from does not invalidate the ArraySlice instance. 
   * WARNING: by default, ArraySlice stores a reference to the target container, therefore 
   * destruction of target container will invalidate the ArraySlice instance.
   * @param T a container type to wrap
   * @param StoreMode determines how to store a target container inside ArraySlice instance.
   *   By default it is stored by reference, but you can force a by-value storage by setting
   *   StoreMode = arx::_identity.
   */
  template<class T, template<class> class StoreMode = reference_> class ArraySlice {
  public:
    typedef typename T::value_type value_type;
    typedef typename T::size_type size_type;
    typedef typename T::allocator_type allocator_type;
  private:
    typename if_<is_same<T, ArrayList<value_type, allocator_type> >, T, typename StoreMode<T>::type>::type a;
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


  template<class Y>
  ArraySlice<Y> createSlice(Y& a, typename Y::size_type lo, typename Y::size_type hi) {
    return ArraySlice<Y>(a, lo, hi);
  }
  template<class Y>
  ArraySlice<Y> createSlice(ArraySlice<Y>& a, typename ArraySlice<Y>::size_type lo, typename ArraySlice<Y>::size_type hi) {
    return ArraySlice<Y>(a.a, a.lo + lo, a.lo + hi);
  }
} // namespace arx

#endif
