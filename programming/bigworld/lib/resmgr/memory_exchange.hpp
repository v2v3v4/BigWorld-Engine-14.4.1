#ifndef MEMORY_EXCHANGE_HPP
#define MEMORY_EXCHANGE_HPP

BW_BEGIN_NAMESPACE

template <class TYPE>
class MemoryExchange
{
public:
	MemoryExchange( int exchangeSize, int blockSize ) :
		exchangeSize_( exchangeSize ),
   		blockSize_( blockSize )	{};

	~MemoryExchange();

	void * getMemory( TYPE *& pFreeList, int & freeListSize,
			TYPE *& pSectionBlock, int & sectionBlockUsed );
	void addToFreeList( void * addr, TYPE *& pFreeList, int & freeListSize );

private:
	int giveFree( TYPE *& pFreeList );
	int getFree( TYPE *& pFreeList );

	int exchangeSize() const	{ return exchangeSize_; }

	struct ExchangeList
	{
		TYPE * pFreeBeg_;
		TYPE * pFreeEnd_;
	};
	BW::vector<ExchangeList>	vector_;
	SimpleMutex lock_;
	BW::vector<char*> memoryBlocks_;
	const int exchangeSize_;
	const int blockSize_;
};

/**
 *	Destructor - frees all the allocated memory.
 */
template <class TYPE>
MemoryExchange<TYPE>::~MemoryExchange()
{
	for ( uint i = 0; i < memoryBlocks_.size(); i++ )
		bw_safe_delete_array(memoryBlocks_[i]);

	memoryBlocks_.clear();
}

/**
 *	This function balances free sections between threads.
 */
template <class TYPE>
int MemoryExchange<TYPE>::giveFree( TYPE *& pFreeList )
{
	int count = exchangeSize_;

	TYPE * pGiveListLast = pFreeList;

	while ((pGiveListLast != NULL) && (--count > 0))
	{
		pGiveListLast = *(TYPE**)pGiveListLast;
	}

	if (pGiveListLast == NULL)
	{
		ERROR_MSG( "giveFree: Did not have %d to give (%d short).\n",
			exchangeSize_, count );
		return 0;
	}

	SimpleMutexHolder permission( lock_ );

	ExchangeList exch;
	exch.pFreeBeg_ = pFreeList;
	exch.pFreeEnd_ = pGiveListLast;
	pFreeList = *(TYPE**)pGiveListLast; // Our list is now the rest.
	vector_.push_back( exch );

	return exchangeSize_;
}


template <class TYPE>
int MemoryExchange<TYPE>::getFree( TYPE *& pFreeList )
{
	SimpleMutexHolder permission( lock_ );

	if (!vector_.empty())
	{
		ExchangeList exch = vector_.back();
		vector_.pop_back();

		*(TYPE**)exch.pFreeEnd_ = pFreeList;
		pFreeList = exch.pFreeBeg_;

		return exchangeSize_;
	}

	return 0;
}

template <class TYPE>
void MemoryExchange< TYPE >::addToFreeList( void * addr,
									TYPE *& pFreeList, int & freeListSize )
{
	*(TYPE**)addr = pFreeList;
	pFreeList = (TYPE*)addr;

	if (++freeListSize >= 2 * exchangeSize_)
	{
		freeListSize -= this->giveFree( pFreeList );
	}
}

template <class TYPE>
void * MemoryExchange< TYPE >::getMemory(
		TYPE *& pFreeList, int & freeListSize,
		TYPE *& pSectionBlock, int & sectionBlockUsed )
{
	// look in free list
	if (pFreeList != NULL)
	{
		--freeListSize;
		TYPE * naddr = pFreeList;
		pFreeList = *(TYPE**)naddr;
		return naddr;
	}

	// make a new block
	if (pSectionBlock == NULL)
	{
		// unless we find something on the exchange market
		freeListSize += this->getFree( pFreeList );

		if (pFreeList)
		{
			--freeListSize;
			TYPE * naddr = pFreeList;
			pFreeList = *(TYPE**)naddr;
			return naddr;
		}

		char* memory = new char[ blockSize_*sizeof(TYPE) ];

		{
			SimpleMutexHolder permission( lock_ );
			memoryBlocks_.push_back( memory );
		}

		pSectionBlock = (TYPE *)memory;
		sectionBlockUsed = 0;
	}
	TYPE * naddr = &(pSectionBlock[ sectionBlockUsed++ ]);
	if (sectionBlockUsed == blockSize_)
		pSectionBlock = NULL;

	return naddr;
}

BW_END_NAMESPACE

#endif // MEMORY_EXCHANGE_HPP
