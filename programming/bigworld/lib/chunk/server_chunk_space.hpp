#ifndef SERVER_CHUNK_SPACE_HPP
#define SERVER_CHUNK_SPACE_HPP

#include "base_chunk_space.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This class is used to store chunk information about a space. It is used by
 *	the server.
 */
class ServerChunkSpace : public BaseChunkSpace
{
public:
	ServerChunkSpace( ChunkSpaceID id );

	void addHomelessItem( ChunkItem * /*pItem*/ ) {};
//	void delHomelessItem( ChunkItem * pItem );

	void focus();

	/**
	 *	This class is used to store the grid of columns.
	 */
	class ColumnGrid
	{
	public:
		ColumnGrid() :
			xOrigin_( 0 ),
			zOrigin_( 0 ),
			xSize_( 0 ),
			zSize_( 0 )
		{
		}

		void rect( int xOrigin, int zOrigin, int xSize, int zSize );

		Column *& operator()( int x, int z )
		{
			static Column * pNullCol = NULL;
			return this->inSpan( x, z ) ?
				grid_[ (x - xOrigin_) + xSize_ * (z - zOrigin_) ] :
				pNullCol;
		}

		const Column * operator()( int x, int z ) const
		{
			return this->inSpan( x, z ) ?
				grid_[ (x - xOrigin_) + xSize_ * (z - zOrigin_) ] :
				NULL;
		}

		bool inSpan( int x, int z ) const
		{
			return xOrigin_ <= x && x < xOrigin_ + xSize_ &&
					zOrigin_ <= z && z < zOrigin_ + zSize_;
		}

		int xBegin() const	{	return xOrigin_; }
		int xEnd() const		{	return xOrigin_ + xSize_; }
		int zBegin() const	{	return zOrigin_; }
		int zEnd() const		{	return zOrigin_ + zSize_; }

	private:
		BW::vector< Column * > grid_;

		int		xOrigin_;
		int		zOrigin_;

		int		xSize_;
		int		zSize_;
	};

	void seeChunk( Chunk * );

protected:
	void recalcGridBounds();

	ColumnGrid					currentFocus_;

};

BW_END_NAMESPACE

#endif // SERVER_CHUNK_SPACE_HPP
