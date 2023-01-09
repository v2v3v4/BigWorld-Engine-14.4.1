#ifndef SPACE_HELPERS_HPP
#define SPACE_HELPERS_HPP


#include "common/grid_coord.hpp"
#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "worldeditor/world/world_manager.hpp"
#include "chunk/chunk_space.hpp"
#include "chunk/geometry_mapping.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This method turns a biased grid coordinate ( 0 .. width, 0 .. height ) into
 *	a chunk-style grid coordinate ( min.x .. min.x + width )
 */
inline void offsetGrid( const GridCoord& localToWorld, uint16 x, uint16 z, int16& offsetX, int16& offsetZ )
{
	offsetX = x;
	offsetZ = z;
	offsetX += localToWorld.x;
	offsetZ += localToWorld.y;
}


/**
 *	This method turns an offset grid coordinate ( min.x .. min.x + width/2 ) into
 *	a biased (unsigned) grid coordinate ( 0..width, 0..height )
 */
inline void biasGrid( const GridCoord& localToWorld, int16 x, int16 z, uint16& biasedX, uint16& biasedZ )
{
	biasedX = x;
	biasedZ = z;
	biasedX -= localToWorld.x;
	biasedZ -= localToWorld.y;

	MF_ASSERT( biasedX<32768 )
	MF_ASSERT( biasedZ<32768 )
}

//can return false, in case we picked up a stray .thumbnail.dds file ( e.g.
//space.thumbnail.dds )
inline bool gridFromThumbnailFileName( const char* fileName, int16& x, int16& z )
{
	BW_GUARD;

	//Assume name is dir/path/path/.../chunkNameo.thumbnail.dds for speed
	//Assume name is dir/path/path/.../chunkNameo.cData for speed
	const char* f = fileName;
	while ( *f ) f++;

	//subtract "xxxxxxxxo.cdata" which is always the last part of an
	//outside chunk identifier.
	if (( f-fileName >= 15 ) && (*(f-7) == 'o'))
	{
		BW::string chunkName( fileName, f - 6 );
		return WorldManager::instance().geometryMapping()->gridFromChunkName( chunkName, x, z );
	}

	return false;
}

inline void chunkID( BW::string& retChunkName, int16 gridX, int16 gridZ, bool checkBounds = true )
{
	BW_GUARD;

	GeometryMapping* dirMap = WorldManager::instance().geometryMapping();
	if (dirMap != NULL)
	{
		const float gridSize  = dirMap->pSpace()->gridSize();
		Vector3 localPos( (float)(gridX*gridSize), 0.f, (float)(gridZ*gridSize) );
		localPos.x += gridSize/2.f;
		localPos.z += gridSize/2.f;
	    retChunkName = dirMap->outsideChunkIdentifier( localPos, checkBounds );
	}
    else
        retChunkName = BW::string();
}


inline void chunkID( BW::wstring& retChunkName, int16 gridX, int16 gridZ, bool checkBounds = true  )
{
	BW_GUARD;

	GeometryMapping* dirMap = WorldManager::instance().geometryMapping();
	if (dirMap == NULL)
	{
		retChunkName = BW::wstring();
		return;
	}
	const float gridSize  = dirMap->pSpace()->gridSize();
	Vector3 localPos( (float)(gridX*gridSize), 0.f, (float)(gridZ*gridSize) );
	localPos.x += gridSize/2.f;
	localPos.z += gridSize/2.f;
	bw_ansitow( dirMap->outsideChunkIdentifier( localPos, checkBounds ), retChunkName );
}

inline bool inGridBounds( int16 gridX, int16 gridZ )
{
	BW_GUARD;
	
	GeometryMapping* dirMap = WorldManager::instance().geometryMapping();
	return dirMap && dirMap->inLocalBounds( gridX, gridZ );
}

inline BW::string thumbnailFilename( const BW::string& pathName, const BW::string& chunkName )
{
	BW_GUARD;

	return pathName + chunkName + ".cdata/thumbnail.dds";
}


inline bool thumbnailExists(  const BW::string& pathName, const BW::string& chunkName )
{
	BW_GUARD;

	DataSectionPtr pSection = BWResource::openSection( pathName + chunkName + ".cdata", false );
	if ( !pSection )
		return false;
	DataSectionPtr pThumbSection = pSection->openSection( "thumbnail.dds" );
	if ( !pThumbSection )
		return false;

	BinaryPtr data = pThumbSection->asBinary();
	return data != NULL && data->len() > 0;
}

BW_END_NAMESPACE

#endif // SPACE_HELPERS_HPP
