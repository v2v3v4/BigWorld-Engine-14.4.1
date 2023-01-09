#ifndef LOOKUP_TABLE_HPP
#define LOOKUP_TABLE_HPP

#include "cstdmf/bw_vector.hpp"

namespace BW
{
	template<
		typename ValueT, 
		class ContainerT = BW::vector<ValueT>, 
		typename KeyT = size_t >
	class LookUpTable
	{
	public:
		typedef KeyT IndexType;
		typedef ValueT ValueType;
		typedef typename ContainerT::iterator iterator;
		typedef typename ContainerT::const_iterator const_iterator;
		
		LookUpTable()
		{

		}

		LookUpTable( size_t reserveSize )
		{
			table_.reserve( reserveSize );
		}

		bool exists( IndexType idxT ) const
		{
			size_t idx = static_cast<size_t>( idxT );
			return idx < table_.size();
		}

		const ValueT& at( IndexType idxT ) const
		{
			this->checkIndex( idxT );
			return table_[ static_cast<size_t>(idxT) ];
		}

		ValueT& at( IndexType idxT )
		{
			this->checkIndex( idxT );
			return table_[ static_cast<size_t>(idxT) ];
		}

		const ValueT& operator[]( IndexType idxT ) const
		{
			return this->at( idxT );
		}

		ValueT& operator[]( IndexType idxT )
		{
			return this->at( idxT );
		}

		IndexType size() const
		{
			return static_cast<IndexType>( table_.size() );
		}

		ContainerT & storage()
		{
			return table_;
		}

		iterator find(const IndexType& index)
		{
			if (index < table_.size())
			{
				return begin() + index;
			}
			else
			{
				return end();
			}
		}

		iterator begin()
		{
			return table_.begin();
		}

		iterator end()
		{
			return table_.end();
		}

		const_iterator begin() const
		{
			return table_.begin();
		}

		const_iterator end() const
		{
			return table_.end();
		}

		void clear() const
		{
			table_.clear();
		}

	private:
		void checkIndex( IndexType idxT ) const
		{
			size_t idx = static_cast<size_t>( idxT );
			if (idx >= table_.size())
			{
				table_.resize( idx+1 );
			}
		}

	private:
		mutable ContainerT table_;
	};
}


#endif // LOOKUP_TABLE_HPP
