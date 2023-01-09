#ifndef SERVER_GEOMETRY_MAPPINGS_HPP
#define SERVER_GEOMETRY_MAPPINGS_HPP

#include "geometry_mapper.hpp"

#include "chunk/geometry_mapping.hpp"
#include "math/rectt.hpp"
#include "network/basictypes.hpp"

#include "cstdmf/bw_set.hpp"


BW_BEGIN_NAMESPACE

class EdgeGeometryMapping;


/**
 *	This class maintains a collection of EdgeGeometryMapping instances for a
 *	single space.
 */
class EdgeGeometryMappings : public GeometryMappingFactory
{
public:
	EdgeGeometryMappings( GeometryMapper & mapper );
	~EdgeGeometryMappings();

	// Override form GeometryMappingFactory
	virtual GeometryMapping * createMapping( ChunkSpacePtr pSpace,
			const Matrix & m, const BW::string & path,
			DataSectionPtr pSettings );

	void removeMapping( EdgeGeometryMapping * pMapping );

	bool tickLoading( const Vector3 & minB, const Vector3 & maxB,
			bool unloadOnly, SpaceID spaceID );
	void prepareNewlyLoadedChunksForDelete();
	void calcLoadedRect( BW::Rect & loadedRect ) const;

	bool getLoadableRects( BW::list< BW::Rect > & rects ) const;

	bool isFullyUnloaded() const;

private:
	typedef BW::set< EdgeGeometryMapping * > Mappings;
	Mappings mappings_;

	GeometryMapper & mapper_;
};

BW_END_NAMESPACE

#endif // SERVER_GEOMETRY_MAPPINGS_HPP
