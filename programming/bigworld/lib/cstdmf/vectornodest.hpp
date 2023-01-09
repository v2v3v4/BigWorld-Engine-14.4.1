
#ifndef VECTORNODEST_HPP
#define VECTORNODEST_HPP


#include "cstdmf/bw_vector.hpp"
#include <algorithm>

#include "cstdmf/stdmf.hpp"
#include "cstdmf/debug.hpp"

#ifdef CODE_INLINE
#define INLINE inline
#else
#define INLINE
#endif

BW_BEGIN_NAMESPACE

/**
 *  This class implements a vector that doesn't delete its elements.
 */
template< class _Ty, class _A = BW::StlAllocator< _Ty >,
	class _BASE = BW::vector< _Ty, _A > >
class VectorNoDestructor : public _BASE
{
	typedef _BASE BASE_TYPE;

public:
	typedef typename BASE_TYPE::size_type size_type;
	typedef typename BASE_TYPE::iterator iterator;
	typedef typename BASE_TYPE::const_iterator const_iterator;
	typedef typename BASE_TYPE::reverse_iterator reverse_iterator;
	typedef typename BASE_TYPE::const_reverse_iterator const_reverse_iterator;

	VectorNoDestructor() :
		nElements_( 0 )
	{

	}

	INLINE void push_back( const _Ty & element )
	{
		if (BASE_TYPE::size() <= nElements_)
		{
			BASE_TYPE::push_back( element );
			nElements_ = BASE_TYPE::size();
		}
		else
		{
			*( this->begin() + nElements_ ) = element;
			nElements_++;
		}
	}

	INLINE iterator end()
	{
		return this->begin() + nElements_;
	}

	INLINE const_iterator end() const
	{
		return this->begin() + nElements_;
	}

	INLINE reverse_iterator rbegin()
	{
		return reverse_iterator(this->begin() + nElements_);
	}

	INLINE const_reverse_iterator rbegin() const
	{
		return const_reverse_iterator(this->begin() + nElements_);
	}

	INLINE reverse_iterator rend()
	{
		return reverse_iterator(this->begin());
	}

	INLINE const_reverse_iterator rend() const
	{
		return const_reverse_iterator(this->begin());
	}


	INLINE size_type size()
	{
		return nElements_;
	}

	INLINE size_type size() const
	{
		return nElements_;
	}

	INLINE bool empty() const
	{
		return nElements_ == 0;
	}

	INLINE void resize( size_type sz )
	{
		if (BASE_TYPE::size() < sz )
		{
			BASE_TYPE::resize( sz );
		}

		nElements_ = sz;
	}

	INLINE void clear()
	{
		nElements_ = 0;
	}

	INLINE _Ty &back()
	{
		MF_ASSERT_DEBUG( nElements_ > 0 );
		return *(this->begin() + nElements_ - 1 );
	}

	INLINE const _Ty &back() const
	{
		MF_ASSERT_DEBUG( nElements_ > 0 );
		return *(this->begin() + nElements_ - 1 );
	}

	INLINE void pop_back()
	{
		MF_ASSERT_DEBUG( nElements_ > 0 );
		nElements_ --;
	}

	INLINE void erase( typename BASE_TYPE::iterator _PP )
	{
		std::copy(_PP + 1, end(), _PP);
		nElements_ --;
	}

	INLINE void assign( typename BASE_TYPE::iterator _first, typename BASE_TYPE::iterator _last )
	{
		reserve( nElements_ + std::distance(_first, _last ) );
		clear();
		for ( typename BASE_TYPE::iterator _it = _first;
			_it != _last; ++_it )
		{
			push_back( *_it );
		}
	}

private:
	size_type nElements_;
};



template< class _Ty, class _A = BW::StlAllocator<_Ty> >
class AVectorNoDestructor : public VectorNoDestructor< _Ty, _A, BW::vector<_Ty,_A> >
{
};

BW_END_NAMESPACE

#endif


