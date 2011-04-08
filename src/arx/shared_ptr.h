#ifndef __ARX_SHARED_PTR__
#define __ARX_SHARED_PTR__

#include "config.h"

#ifdef ARX_USE_BOOST
#include <boost/shared_ptr.hpp>
namespace arx {
	using boost::shared_ptr;
}
#else

#ifdef ARX_LINUX
#error "Linux non-boost implementation of shared_ptr template is currently not available. Please define ARX_USE_BOOST in config.h to allow using boost."
#endif

#ifdef ARX_WIN

// Windows.h nasty macros
#ifdef min
#  undef min
#  define MIN_UNDEFFED
#endif
#ifdef max
#  undef max
#  define MAX_UNDEFFED
#endif

#include <cassert>
#include <exception>
#include <algorithm>
#include <Windows.h>

namespace arx {

namespace detail {

class counted_base {
private:
	counted_base(counted_base const&);
	counted_base& operator= (counted_base const&);

	long use_count;
public:
	counted_base(): use_count(1) {}

	virtual ~counted_base() {}

	virtual void dispose() = 0;

	virtual void destroy() {
		delete this;
	}

	void add_ref_copy() {
		InterlockedIncrement(reinterpret_cast<long volatile*> (&use_count));
	}

	void release() {
		if(InterlockedDecrement(reinterpret_cast<long volatile*> (&use_count)) == 0) {
			dispose();
			destroy();
		}
	}
};

template<class T> 
class counted_impl: public counted_base {
private:
	T* px;

	counted_impl(counted_impl const&);
	counted_impl& operator= (counted_impl const&);

public:
	explicit counted_impl(T* px): px(px) {}

	virtual void dispose() {
		delete px;
	}
};

class shared_count {
private:
	counted_base* pi;

public:
	shared_count(): pi(0) {}

	template<class Y> 
	explicit shared_count(Y* p): pi(0) {
		pi = new counted_impl<Y>(p);
		if(pi == 0) {
			delete p;
			throw std::bad_alloc();
		}
	}

	shared_count(shared_count const& r): pi(r.pi) {
		if(pi != 0) 
			pi->add_ref_copy();
	}

	shared_count& operator= (shared_count const& r) {
		counted_base* tmp = r.pi;
		if(tmp != pi) {
			if(tmp != 0)
				tmp->add_ref_copy();
			if(pi != 0)
				pi->release();
			pi = tmp;
		}
		return *this;
	}

	~shared_count() {
		if(pi != 0)
			pi->release();
	}

	void swap(shared_count& r) {
		counted_base* tmp = r.pi;
		r.pi = pi;
		pi = tmp;
	}

};

}

/** 
* A boost-inspired counted pointer. Win32 platform only.
* Thread-safe.
*/
template<class T> 
class shared_ptr {
private:
	template<class Y> friend class shared_ptr;

	T* px;
	detail::shared_count pn;

public:
	typedef shared_ptr<T> this_type;
	typedef T element_type;
	typedef T value_type;
	typedef T* pointer;
	typedef T& reference;

	shared_ptr(): px(0), pn() {}
	template<class Y> 
	explicit shared_ptr(Y* p): px(p), pn(p) {}

	void swap(shared_ptr<T> & other) {
		std::swap(px, other.px);
		pn.swap(other.pn);
	}

	bool operator! () const {
		return px == 0;
	}

	typedef T* this_type::* unspecified_bool_type;

	operator unspecified_bool_type() const {
		return (px == 0)?(0):(&this_type::px);
	}

	reference operator* () const {
		assert(px != 0);
		return *px;
	}

	pointer operator-> () const {
		assert(px != 0);
		return px;
	}

	shared_ptr& operator=(shared_ptr const& r) {
		px = r.px;
		pn = r.pn;
		return *this;
	}

	template<class Y>
	shared_ptr& operator=(shared_ptr<Y> const& r)	{
		px = r.px;
		pn = r.pn;
		return *this;
	}

	pointer get() const {
		return px;
	}

	template<class Y> 
	void reset(Y * p) {
		assert(p == 0 || p != px); // catch self-reset errors
		this_type(p).swap(*this);
	}
};

}

#ifdef MIN_UNDEFFED
#  define min(x, y) ((x > y)?(y):(x))
#  undef MIN_UNDEFFED
#endif
#ifdef MAX_UNDEFFED
#  define max(x, y) ((x > y)?(x):(y))
#  undef MAX_UNDEFFED
#endif

#endif // ARX_WIN

#endif // ARX_USE_BOOST

#endif // __ARX_SHARED_PTR__