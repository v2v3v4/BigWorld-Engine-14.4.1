#ifndef GEOMETRY_MAPPING_HPP
#define GEOMETRY_MAPPING_HPP

#include "cstdmf/bw_string.hpp"
#include "cstdmf/smartpointer.hpp"

#include "math/matrix.hpp"

#include "chunk_space.hpp"
#include "resmgr/datasection.hpp"


BW_BEGIN_NAMESPACE

class BoundingBox;

class Vector3;

/**
 *	This class is a mapping of a resource directory containing chunks
 *	into a chunk space.
 *
 *	@note Only its chunk space and chunks queued to load retain references
 *	to this object.
 */
class GeometryMapping : public SafeReferenceCount
{
public:
	GeometryMapping( ChunkSpacePtr pSpace, const Matrix & m,
		const BW::string & path, DataSectionPtr pSettings );
	~GeometryMapping();

	ChunkSpacePtr	pSpace() const			{ return pSpace_; }

	const Matrix &	mapper() const			{ return mapper_; }
	const Matrix &	invMapper() const		{ return invMapper_; }

	const BW::string & path() const		{ return path_; }
	const BW::string & name() const		{ return name_; }

	DataSectionPtr	pDirSection();
	static DataSectionPtr openSettings( const BW::string & path );

	Chunk* findChunkByName( const BW::string & identifier, bool createIfNotFound );

	/// The following accessors return the world-space grid bounds of this
	/// mapping, after the transform is applied. These bounds are expanded
	/// to include even the slightest intersection of the mapping with a
	/// grid square in the space's coordinate system
	int minGridX() const					{ return minGridX_; }
	int maxGridX() const 					{ return maxGridX_; }
	int minGridY() const 					{ return minGridY_; }
	int maxGridY() const 					{ return maxGridY_; }

	/// The following accessors return the bounds of this mapping in its
	/// own local coordinate system.
	int minLGridX() const					{ return minLGridX_; }
	int maxLGridX() const 					{ return maxLGridX_; }
	int minLGridY() const 					{ return minLGridY_; }
	int maxLGridY() const 					{ return maxLGridY_; }

	/// The following accessors return the bounds of the sub grid loaded
	virtual int minSubGridX() const			{ return minLGridX_; }
	virtual int maxSubGridX() const 		{ return maxLGridX_; }
	virtual int minSubGridY() const 		{ return minLGridY_; }
	virtual int maxSubGridY() const 		{ return maxLGridY_; }

	/// Utility coordinate space mapping functions
	void gridToLocal( int x, int y, int& lx, int& ly ) const;
	void boundsToGrid( const BoundingBox& bb,
		int& minGridX, int& minGridZ,
		int& maxGridX, int& maxGridZ ) const;
	void gridToBounds( int minGridX, int minGridY,
		int maxGridX, int maxGridY,
		BoundingBox& retBB ) const;
	bool inLocalBounds( const int gridX, const int gridZ ) const;
	bool inWorldBounds( const int gridX, const int gridZ ) const;

	bool condemned()						{ return condemned_; }
	void condemn();

	BW::string outsideChunkIdentifier( const Vector3 & localPoint,
		bool checkBounds = true ) const;
	BW::string outsideChunkIdentifier( int x, int z, bool checkBounds = true ) const;
	static bool gridFromChunkName( const BW::string& chunkName, int16& x, int16& z );
	bool chunkFileExists( const BW::string& chunkId ) const;
	BW::set<BW::string> gatherInternalChunks() const;
	BW::set<BW::string> gatherChunks() const;

	static float getGridSize( DataSectionPtr pSettings );

private:
	ChunkSpacePtr	pSpace_;

	Matrix			mapper_;
	Matrix			invMapper_;

	BW::string		path_;
	BW::string		name_;
	DataSectionPtr	pDirSection_;

	int				minGridX_;
	int				maxGridX_;
	int				minGridY_;
	int				maxGridY_;

	int				minLGridX_;
	int				maxLGridX_;
	int				minLGridY_;
	int				maxLGridY_;

	bool			condemned_;
	bool			singleDir_;
};


/**
 *	This class is an interface used to create GeometryMapping instances. It is
 *	used by the server to create a derived type.
 */
class GeometryMappingFactory
{
public:
	virtual ~GeometryMappingFactory() {}

	virtual GeometryMapping * createMapping( ChunkSpacePtr pSpace,
				const Matrix & m, const BW::string & path,
				DataSectionPtr pSettings ) = 0;
};

BW_END_NAMESPACE

#endif // GEOMETRY_MAPPING_HPP
