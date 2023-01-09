/**
 * binary_block.hpp
 */

#ifndef _BINARY_BLOCK_HEADER
#define _BINARY_BLOCK_HEADER

#include "cstdmf/smartpointer.hpp"

#include <iostream>

BW_BEGIN_NAMESPACE

class BinaryBlock;
typedef SmartPointer<BinaryBlock> BinaryPtr;

/**
 * This class is a simple wrapper around a block of binary data. It is designed
 * to be used so that blocks of memory can be reference counted and passed
 * around with smart pointers.
 */
class BinaryBlock : public SafeReferenceCount
{
public:
	BinaryBlock(	const void* data, size_t len,
					const char * allocator,
					BinaryPtr pOwner = 0 );
	BinaryBlock(	const void* data, size_t len,
					const char * allocator,
					bool externallyOwned );
	BinaryBlock(	std::istream& stream, std::streamsize len,
					const char * allocator );

	~BinaryBlock();

	/// This method returns a pointer to the block of binary data
	const void *	data() const	{ return data_; }
	const char *	cdata() const
	{ return reinterpret_cast< const char * >( this->data() ); }
	char *	cdata()
	{ return reinterpret_cast< char * >( data_ ); }

	/// This method returns the length of the binary data
	int				len() const		{ return (int)len_; }

	BinaryPtr		pOwner()		{ return pOwner_; }

	/// These methods compress/decompress a BinaryBlock
	static const int RAW_COMPRESSION		=  0;
	static const int DEFAULT_COMPRESSION	=  6;
	static const int BEST_COMPRESSION		= 10;
	BinaryPtr compress(int level = DEFAULT_COMPRESSION) const;
	BinaryPtr decompress() const;
	bool isCompressed() const;
	int compare( BinaryBlock& that ) const;

	bool canZip() const;
	void canZip( bool newVal );

	static bool memoryCritical() { return s_memoryCritical_; }
	static void memoryCritical( bool val ) { s_memoryCritical_ = val; }

private:
	void initCopydata( const void* data, size_t len );

	static bool s_memoryCritical_;

	void *			data_;
	std::streamsize	len_;
	bool			externallyOwned_;
	BinaryPtr		pOwner_;

	bool canZip_;

#if ENABLE_RESOURCE_COUNTERS
	BW::string		allocator_;
#endif

private:
	BinaryBlock(const BinaryBlock&);
	BinaryBlock& operator=(const BinaryBlock&);
};

/**
 *	Implements a streambuf that reads a binary data section.
 */
class BinaryInputBuffer : public std::streambuf
{
public:
	BinaryInputBuffer(BinaryPtr data) :
		data_(data)
	{
		this->std::streambuf::setg(
			this->data_->cdata(), this->data_->cdata(),
			this->data_->cdata() + this->data_->len());
	}

private:
	BinaryPtr data_;
};

BW_END_NAMESPACE

#endif // _BINARY_BLOCK_HEADER

