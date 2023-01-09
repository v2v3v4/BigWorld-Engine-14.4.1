#include "pch.hpp"

#include "static_scene_writer.hpp"
#include "string_table_writer.hpp"
#include "binary_format_writer.hpp"
#include "octree_writer.hpp"

#include "cstdmf/command_line.hpp"
#include "cstdmf/lookup_table.hpp"
#include "math/loose_octree.hpp"
#include "math/boundbox.hpp"
#include "math/math_extra.hpp"
#include "math/sphere.hpp"

namespace BW {
namespace CompiledSpace {

class StringTableWriter;

namespace {
	const char * PARTITION_SIZE_HINT = "partitionSizeHint";

	void readPartitionSizeHint( const CommandLine& cmdLine, float * partitionSizeHint)
	{
		const char *pHint = cmdLine.getParam( PARTITION_SIZE_HINT );
		if (!pHint)
		{
			return;
		}
		*partitionSizeHint = (float)atof( pHint );
	}
}
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
IStaticSceneWriterHandler::IStaticSceneWriterHandler() :
	pStringTable_( NULL ),
	pAssetList_( NULL )
{

}


IStaticSceneWriterHandler::~IStaticSceneWriterHandler()
{

}


// ----------------------------------------------------------------------------
void IStaticSceneWriterHandler::configure( StringTableWriter* pStringTable, 
	AssetListWriter* pAssetList )
{
	pStringTable_ = pStringTable;
	pAssetList_ = pAssetList;
}


// ----------------------------------------------------------------------------
StringTableWriter& IStaticSceneWriterHandler::strings()
{
	MF_ASSERT( pStringTable_ );
	return *pStringTable_;
}


// ----------------------------------------------------------------------------
AssetListWriter& IStaticSceneWriterHandler::assets()
{
	MF_ASSERT( pAssetList_ );
	return *pAssetList_;
}


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
StaticSceneWriter::StaticSceneWriter( FourCC formatMagic )
	: formatMagic_( formatMagic )
	, partitionSizeHint_( -1.0f )
{

}


// ----------------------------------------------------------------------------
StaticSceneWriter::~StaticSceneWriter()
{

}


// ----------------------------------------------------------------------------
bool StaticSceneWriter::initialize( const DataSectionPtr& pSpaceSettings,
	const CommandLine& commandLine )
{
	float partitionSizeHint = -1.0f;
	readPartitionSizeHint( commandLine, &partitionSizeHint );
	setPartitionSizeHint( partitionSizeHint );

	// Iterate over all handlers and configure them
	for (auto it = types_.begin(); it != types_.end(); ++it)
	{
		IStaticSceneWriterHandler* typeHandler = it->typeWriter_;
		typeHandler->configure( &strings(), &assets() );
	}

	return true;
}


// ----------------------------------------------------------------------------
void StaticSceneWriter::setPartitionSizeHint( float partitionSizeHint )
{
	partitionSizeHint_ = partitionSizeHint;
}


// ----------------------------------------------------------------------------
bool StaticSceneWriter::write( BinaryFormatWriter& writer )
{
	vector< AABB > sceneObjectBounds;
	AABB sceneBB;
	generateObjectBounds( sceneObjectBounds, sceneBB );

	vector< Sphere > sceneObjectBoundSpheres;
	generateObjectSphereBounds( sceneObjectBounds, sceneObjectBoundSpheres );

	DynamicLooseOctree octree;
	LookUpTable< vector<DynamicLooseOctree::NodeDataReference> > octreeNodeContents;
	generateOctree( sceneObjectBounds, sceneBB, octree, octreeNodeContents );

	typedef StaticSceneTypes::TypeHeader TypeHeader;

	BinaryFormatWriter::Stream* stream =
		writer.appendSection( formatMagic_,
			StaticSceneTypes::FORMAT_VERSION );
	MF_ASSERT( stream != NULL );

	// Output the scene octree data
	if (!writeOctree( stream, octree, octreeNodeContents ))
	{
		return false;
	}
	stream->write( sceneObjectBounds );
	stream->write( sceneObjectBoundSpheres );

	// Types
	BW::vector<TypeHeader> typeHeaders;
	for (auto it = types_.begin(); it != types_.end(); ++it)
	{
		TypeHeader typeHeader;
		typeHeader.typeID_ = it->typeID_;
		typeHeader.numObjects_ = (uint32)it->typeWriter_->numObjects();

		typeHeaders.push_back( typeHeader );
	}

	stream->write( typeHeaders );

	// Let types write out their data into the container format
	for (auto it = types_.begin(); it != types_.end(); ++it)
	{
		TypeHeader typeHeader;
		typeHeader.typeID_ = it->typeID_;
		it->typeWriter_->writeData( writer );
	}

	return true;
}

// ----------------------------------------------------------------------------
void StaticSceneWriter::addType( IStaticSceneWriterHandler* typeWriter )
{
	MF_ASSERT( !this->hasType( typeWriter->typeID() ) );
	Type type = { typeWriter->typeID(), typeWriter };

	types_.push_back( type );
}


// ----------------------------------------------------------------------------
bool StaticSceneWriter::hasType( SceneTypeSystem::RuntimeTypeID typeID )
{
	for (auto it = types_.begin(); it != types_.end(); ++it)
	{
		if (it->typeID_ == typeID)
		{
			return true;
		}
	}

	return false;
}


// ----------------------------------------------------------------------------
size_t StaticSceneWriter::typeIndexToSceneIndex(
	IStaticSceneWriterHandler* typeWriter, size_t typeIndex )
{
	size_t baseIndex = 0;

	BW::vector<StaticSceneTypes::TypeHeader> typeHeaders;
	for (auto it = types_.begin(); it != types_.end(); ++it)
	{
		if (typeWriter == it->typeWriter_)
		{
			return baseIndex + typeIndex;
		}
		else
		{
			baseIndex += it->typeWriter_->numObjects();
		}
	}

	return (size_t)-1;
}


// ----------------------------------------------------------------------------
void StaticSceneWriter::postProcess()
{
	size_t result = 0;
	for (auto it = types_.begin(); it != types_.end(); ++it)
	{
		const IStaticSceneWriterHandler* typeHandler = it->typeWriter_;
		result += typeHandler->numObjects();
	}
	
	TRACE_MSG( "%d static objects.\n", result );
}


// ----------------------------------------------------------------------------
AABB StaticSceneWriter::boundBox() const
{
	vector< AABB > sceneObjectBounds;
	AABB sceneBB;
	generateObjectBounds( sceneObjectBounds, sceneBB );

	return sceneBB;
}


// ----------------------------------------------------------------------------
void StaticSceneWriter::generateObjectBounds( 
	vector< AABB > & sceneObjectBounds, AABB & outSceneBB ) const
{
	AABB sceneBB;
	for (auto it = types_.begin(); it != types_.end(); ++it)
	{
		const IStaticSceneWriterHandler* typeHandler = it->typeWriter_;
		size_t numObjects = typeHandler->numObjects();
		for (size_t objIndex = 0; objIndex < numObjects; ++objIndex)
		{
			const AABB& objBB = typeHandler->worldBounds( objIndex );

			sceneBB.addBounds( objBB );
			sceneObjectBounds.push_back( objBB );
		}
	}

	outSceneBB = sceneBB;
}


// ----------------------------------------------------------------------------
void StaticSceneWriter::generateObjectSphereBounds( 
	BW::vector< AABB > & sceneObjectBounds,
	BW::vector< Sphere > & sceneObjectSphereBounds )
{
	sceneObjectSphereBounds.reserve( sceneObjectBounds.size() );
	for (size_t index = 0; index < sceneObjectBounds.size(); ++index)
	{
		const AABB& bb = sceneObjectBounds[index];
		sceneObjectSphereBounds.push_back( Sphere( bb ) );
	}
}


// ----------------------------------------------------------------------------
void StaticSceneWriter::generateOctree( 
	const vector< AABB > & sceneObjectBounds,
	const AABB & sceneBB, DynamicLooseOctree & octree, 
	LookUpTable< vector<DynamicLooseOctree::NodeDataReference> > & octreeNodeContents )
{
	const float DEFAULT_DESIRED_NODE_WIDTH = 20.0f;
	const uint32 MAX_TREE_DEPTH = 8;
	float desiredNodeWidth = DEFAULT_DESIRED_NODE_WIDTH ;
	if (partitionSizeHint_ > 0.0f)
	{
		desiredNodeWidth = partitionSizeHint_;
	}

	// Calculate max depth to be having a node for every 5m of space
	float sceneMaxDim = max( max( sceneBB.width(), sceneBB.height() ), 
		sceneBB.depth());
	uint32 nodesPerEdge = static_cast<uint32>(sceneMaxDim / desiredNodeWidth);
	uint32 maxDepth = log2ceil(nodesPerEdge);

	maxDepth = Math::clamp( 1u, maxDepth, MAX_TREE_DEPTH );

	size_t numObjects = sceneObjectBounds.size();
	if (numObjects == 0)
	{
		return;
	}

	octree.initialise( sceneBB.centre(), sceneMaxDim, maxDepth);

	for (size_t objIndex = 0; objIndex < numObjects; ++objIndex)
	{
		const AABB& objBB = sceneObjectBounds[objIndex];

		DynamicLooseOctree::NodeIndex nodeId = octree.insert(objBB);
		DynamicLooseOctree::NodeDataReference dataRef = octree.dataOnLeaf(nodeId);

		vector<DynamicLooseOctree::NodeDataReference> & contents = 
			octreeNodeContents[dataRef];
		contents.push_back( (DynamicLooseOctree::NodeDataReference)objIndex );
	}

	octree.updateHeirarchy();

	// Prepare a static tree format for storage
	LooseOctreeHelpers::prepareStaticTreeStorage( octree );
}


} // namespace CompiledSpace
} // namespace BW
