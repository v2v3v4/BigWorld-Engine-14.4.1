#include "pch.hpp"

#include "resmgr/bwresource.hpp"

#include "string_table_writer.hpp"
#include "asset_list_writer.hpp"
#include "binary_format_writer.hpp"
#include "static_scene_model_writer.hpp"
#include "chunk_converter.hpp"

#include "../asset_list_types.hpp"

namespace BW {
namespace CompiledSpace {

namespace {

class IndicesRangeBuilder
{
public:
	IndicesRangeBuilder( BW::vector<StringTableTypes::Index>* pArray,
		StaticSceneModelTypes::IndexRange* pRange ) :
			pArray_( pArray ),
			pRange_( pRange ),
			lastSize_( pArray->size() )
	{
		pRange_->first_ = (uint16)-1;
		pRange_->last_ = (uint16)-1;
	}

	void add( StringTableTypes::Index idx )
	{
		// Make sure no one else has pushed something in the meantime.
		MF_ASSERT( pArray_->size() == lastSize_ );

		pArray_->push_back( idx );
		lastSize_ = pArray_->size();
		if (pRange_->first_ == uint16(-1))
		{
			pRange_->first_ = (uint16)(lastSize_-1);
		}

		pRange_->last_ = (uint16)(lastSize_-1);
	}


private:
	BW::vector<StringTableTypes::Index>* pArray_; 
	StaticSceneModelTypes::IndexRange* pRange_;
	size_t lastSize_;
};

// ----------------------------------------------------------------------------
BW::string extractVisualName( const BW::string& modelName )
{
	DataSectionPtr pDS = BWResource::openSection( modelName );
	if (!pDS)
	{
		ERROR_MSG( "Could not open '%s'\n", modelName.c_str() );
		return "";
	}

	if (pDS->findChild( "nodefullVisual" ))
	{
		return pDS->readString( "nodefullVisual" ) + ".visual";
	}
	else if (pDS->findChild( "nodelessVisual" ))
	{
		return pDS->readString( "nodelessVisual" ) + ".visual";
	}
	else
	{
		return "";
	}
}

// ----------------------------------------------------------------------------
AABB extractBounds( const BW::string& modelName )
{
	BW::string visualName = extractVisualName( modelName );
	if (visualName.empty())
	{
		return BoundingBox::s_insideOut_;
	}

	DataSectionPtr pDS = BWResource::openSection( visualName );
	if (!pDS)
	{
		ERROR_MSG( "Could not open '%s'\n", visualName.c_str() );
		return BoundingBox::s_insideOut_;
	}

	DataSectionPtr pBB = pDS->openSection( "boundingBox" );
	if (!pBB)
	{
		ERROR_MSG( "Visual '%s' has no bounding box\n", visualName.c_str() );
		return BoundingBox::s_insideOut_;
	}

	AABB result;
	result.setBounds( pBB->readVector3("min"), pBB->readVector3("max") );

	// Now override that result with the model's visibility BB if there is one.
	// the model's visibility BB takes into account animation of the object
	// and should be larger than the other BB. If it isn't, we should probably
	// end up taking the merger of the two bb's to put into the bounding box.

	DataSectionPtr pModelFile = BWResource::openSection( modelName );
	if (!pModelFile)
	{
		ERROR_MSG( "Could not open '%s'\n", modelName.c_str() );
		return BoundingBox::s_insideOut_;
	}
	DataSectionPtr bb = pModelFile->openSection("visibilityBox");
	if (bb)
	{
		Vector3 minBB = bb->readVector3("min", Vector3(0.f,0.f,0.f));
		Vector3 maxBB = bb->readVector3("max", Vector3(0.f,0.f,0.f));
		result.addBounds(minBB);
		result.addBounds(maxBB);
	}

	return result;
}

// ----------------------------------------------------------------------------
AABB extractBounds( const BW::vector<BW::string>& models )
{
	AABB result( Vector3::ZERO, Vector3::ZERO );

	for (size_t i = 0; i < models.size(); ++i)
	{
		AABB bounds = extractBounds( models[i] );
		if (!bounds.insideOut())
		{
			result.addBounds( bounds );
		}
	}

	return result;
}

} // anonymous namespace

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
StaticSceneModelWriter::StaticSceneModelWriter()
{

}


// ----------------------------------------------------------------------------
StaticSceneModelWriter::~StaticSceneModelWriter()
{

}


// ----------------------------------------------------------------------------
 SceneTypeSystem::RuntimeTypeID StaticSceneModelWriter::typeID() const
 {
 	return StaticSceneTypes::StaticObjectType::STATIC_MODEL;
 }
	

// ----------------------------------------------------------------------------
size_t StaticSceneModelWriter::numObjects() const
{
	return models_.size();
}


// ----------------------------------------------------------------------------
bool StaticSceneModelWriter::writeData( BinaryFormatWriter& writer )
{
	BinaryFormatWriter::Stream* stream =
		writer.appendSection( 
		StaticSceneModelTypes::FORMAT_MAGIC, 
		StaticSceneModelTypes::FORMAT_VERSION );
	MF_ASSERT( stream != NULL );
	
	stream->write( models_ );
	stream->write( stringIndices_ );

	return true;
}


// ----------------------------------------------------------------------------
void StaticSceneModelWriter::convertModel( const ConversionContext& ctx,
	const DataSectionPtr& pItemDS, const BW::string& uid )
{
	const bool isShell = false;
	addFromChunkModel( pItemDS, ctx.chunkTransform, 
		isShell, strings(), assets() );
}


// ----------------------------------------------------------------------------
void StaticSceneModelWriter::convertShell( const ConversionContext& ctx,
	const DataSectionPtr& pItemDS, const BW::string& uid )
{
	const bool isShell = true;
	addFromChunkModel( pItemDS, ctx.chunkTransform, 
		isShell, strings(), assets() );
}


// ----------------------------------------------------------------------------
bool StaticSceneModelWriter::addFromChunkModel( const DataSectionPtr& pItemDS,
	const Matrix& chunkTransform,
	bool isShell,
	StringTableWriter& stringTable,
	AssetListWriter& assetList )
{
	MF_ASSERT( pItemDS != NULL );

	StaticSceneModelTypes::Model result;
	memset( &result, 0, sizeof(result) );

	// Models
	BW::vector<BW::string> models;
	pItemDS->readStrings( "resource", models );
	if (models.empty())
	{
		ERROR_MSG( "StaticModelWriter: ChunkModel has no resources specified.\n" );
		return false;
	}

	IndicesRangeBuilder resourceNameRange( &stringIndices_, &result.resourceIDs_ );
	for (size_t i = 0; i < models.size(); ++i)
	{
		auto stringTableIdx = assetList.addAsset(
				CompiledSpace::AssetListTypes::ASSET_TYPE_MODEL,
				models[i], stringTable );

		resourceNameRange.add( stringTableIdx );
	}

	// Animation
	DataSectionPtr pAnimSec = pItemDS->openSection( "animation" );
	if (pAnimSec)
	{
		result.animationName_ =
			stringTable.addString( pAnimSec->readString( "name" ) );

		result.animationMultiplier_ =
			pAnimSec->readFloat( "frameRateMultiplier", 1.f );
	}
	else
	{
		result.animationName_ = StringTableTypes::INVALID_INDEX;
	}

	// Dyes
	BW::vector<DataSectionPtr> dyeSecs;
	pItemDS->openSections( "dye", dyeSecs );

	IndicesRangeBuilder tintDyeRange( &stringIndices_, &result.tintDyes_ );
	for( BW::vector<DataSectionPtr>::iterator iter = dyeSecs.begin();
		iter != dyeSecs.end(); ++iter )
	{
		BW::string dye = ( *iter )->readString( "name" );
		BW::string tint = ( *iter )->readString( "tint" );

		tintDyeRange.add( stringTable.addString( dye ) );
		tintDyeRange.add( stringTable.addString( tint ) );
	}

	// Transform
	result.worldTransform_ = chunkTransform;
	result.worldTransform_.preMultiply(
		pItemDS->readMatrix34( "transform", Matrix::identity ) );

	// Bounding box
	AABB bounds = extractBounds( models );
	if (bounds.insideOut())
	{
		ERROR_MSG( "StaticModelWriter: Invalid bounding box.\n" );
		return false;
	}

	bounds.transformBy( result.worldTransform_ );

	// Various flags
	bool castsShadow = true;
	castsShadow = pItemDS->readBool( "castsShadow", castsShadow );
	castsShadow = pItemDS->readBool( "editorOnly/castsShadow", castsShadow ); // check legacy
	if (castsShadow)
	{
		result.flags_ |= StaticSceneModelTypes::FLAG_CASTS_SHADOW;
	}

	if (pItemDS->readBool( "reflectionVisible" ))
	{
		result.flags_ |= StaticSceneModelTypes::FLAG_REFLECTION_VISIBLE;
	}

	if (isShell)
	{
		result.flags_ |= StaticSceneModelTypes::FLAG_IS_SHELL;
	}

	models_.push_back( result );
	worldBounds_.push_back( bounds );

	return true;
}


// ----------------------------------------------------------------------------
const AABB& StaticSceneModelWriter::worldBounds( size_t idx ) const
{
	MF_ASSERT( idx < worldBounds_.size() );
	return worldBounds_[idx];
}


// ----------------------------------------------------------------------------
const Matrix& StaticSceneModelWriter::worldTransform( size_t idx ) const
{
	MF_ASSERT( idx < models_.size() );
	return models_[idx].worldTransform_;
}

} // namespace CompiledSpace
} // namespace BW
