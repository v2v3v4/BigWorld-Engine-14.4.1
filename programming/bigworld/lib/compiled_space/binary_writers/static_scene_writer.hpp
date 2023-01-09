#ifndef STATIC_SCENE_WRITER_HPP
#define STATIC_SCENE_WRITER_HPP

#include "cstdmf/bw_string.hpp"
#include "cstdmf/bw_vector.hpp"

#include "resmgr/datasection.hpp"

#include "../static_scene_types.hpp"
#include "math/loose_octree.hpp"
#include "cstdmf/lookup_table.hpp"
#include "space_writer.hpp"

namespace BW {
	
class Sphere;

namespace CompiledSpace {

class StringTableWriter;
class BinaryFormatWriter;

class IStaticSceneWriterHandler
{
public:
	typedef ISpaceWriter::ConversionContext ConversionContext;

	IStaticSceneWriterHandler();
	virtual ~IStaticSceneWriterHandler();

	void configure( StringTableWriter* pStringTable, 
		AssetListWriter* pAssetList );

	virtual SceneTypeSystem::RuntimeTypeID typeID() const = 0;

	virtual size_t numObjects() const = 0;
	virtual const AABB& worldBounds( size_t idx ) const = 0;
	virtual const Matrix& worldTransform( size_t idx ) const = 0;

	virtual bool writeData( BinaryFormatWriter& writer ) = 0;

protected:
	StringTableWriter& strings();
	AssetListWriter& assets();

private:
	StringTableWriter* pStringTable_; 
	AssetListWriter* pAssetList_;

};

class StaticSceneWriter :
	public ISpaceWriter
{
public:
	StaticSceneWriter( FourCC formatMagic );
	~StaticSceneWriter();

	virtual bool initialize( const DataSectionPtr& pSpaceSettings,
		const CommandLine& commandLine );
	virtual void postProcess();
	virtual bool write( BinaryFormatWriter& writer );

	void setPartitionSizeHint( float nodeSizeHint );

	bool write( BinaryFormatWriter& writer, AABB& bounds );

	// Caution: order in which things are added matters!
	// If you change order, bump the version number.
	void addType( IStaticSceneWriterHandler* typeWriter );
	
	bool hasType( SceneTypeSystem::RuntimeTypeID typeID );

	size_t typeIndexToSceneIndex( IStaticSceneWriterHandler* typeWriter, size_t typeIndex );
	AABB boundBox() const;

private:
	
	void generateObjectBounds( BW::vector< AABB > & sceneObjectBounds,
		AABB & sceneBB ) const;
	void generateObjectSphereBounds( BW::vector< AABB > & sceneObjectBounds,
		BW::vector< Sphere > & sceneObjectSphereBounds );
	void generateOctree( const vector< AABB > & sceneObjectBounds,
		const AABB & sceneBB, DynamicLooseOctree & octree, 
		LookUpTable< vector<DynamicLooseOctree::NodeDataReference> > & octreeNodeContents );
	
	struct Type
	{
		SceneTypeSystem::RuntimeTypeID typeID_;
		IStaticSceneWriterHandler* typeWriter_;
	};
	
	FourCC formatMagic_; 
	BW::vector<Type> types_;

	float partitionSizeHint_;
};

} // namespace CompiledSpace
} // namespace BW


#endif // STATIC_SCENE_WRITER_HPP
