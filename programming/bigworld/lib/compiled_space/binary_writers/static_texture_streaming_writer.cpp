#include "pch.hpp"

#include "static_texture_streaming_writer.hpp"

#include "binary_format_writer.hpp"
#include "string_table_writer.hpp"
#include "asset_list_writer.hpp"
#include "chunk_converter.hpp"

#include "cstdmf/string_builder.hpp"
#include "moo/streaming_texture.hpp"
#include "moo/texture_usage.hpp"
#include "moo/base_texture.hpp"
#include "speedtree/speedtree_renderer.hpp"
#include "model/super_model.hpp"

namespace BW {
namespace CompiledSpace {

class StaticTextureStreamingWriter::Detail
{
public:
	static void convertUsagesToDefs(
		StringTableWriter & strings,
		AssetListWriter & assetList,
		Moo::ModelTextureUsageContext & usages,
		BW::vector<StaticTextureStreamingTypes::Usage> & defs )
	{
		const Moo::ModelTextureUsage * pUsages = NULL;
		size_t numUsages = 0;
		usages.getUsages( &pUsages, &numUsages );

		for (size_t index = 0; index < numUsages; ++index)
		{
			const Moo::ModelTextureUsage & usage = pUsages[index];
			
			Moo::BaseTexturePtr pTexture = usage.pTexture_;
			if (!pTexture)
			{
				continue;
			}

			const BW::string & resourceID = pTexture->resourceID();

			StaticTextureStreamingTypes::Usage def;
			def.bounds_ = usage.bounds_;
			def.uvDensity_ = usage.uvDensity_;
			def.worldScale_ = usage.worldScale_;
			def.resourceID_ = assetList.addAsset( 
				AssetListTypes::ASSET_TYPE_TEXTURE,
				resourceID,
				strings);

			defs.push_back( def );
		}
	}

	struct CachedUsages
	{
		AABB localBoundingBox_;
		BW::map< Moo::BaseTexturePtr, float > textureUVDensity_;
	};

	typedef BW::unordered_map< BW::string, CachedUsages > Cache;
	Cache modelCache_;
	Cache treeCache_;

	void applyCachedUsages( const Matrix & worldTransform, 
		const CachedUsages & usages, 
		Moo::ModelTextureUsageGroup * usageGroup )
	{
		if (usages.localBoundingBox_.insideOut())
		{
			return;
		}

		for (auto iter = usages.textureUVDensity_.begin();
			iter != usages.textureUVDensity_.end(); 
			++iter)
		{
			Moo::BaseTexturePtr texture = iter->first;
			float density = iter->second;
			
			usageGroup->setTextureUsage( texture.get(), density );
		}

		AABB bounds = usages.localBoundingBox_;
		bounds.transformBy( worldTransform );

		usageGroup->setWorldScale_FromTransform( worldTransform );
		usageGroup->setWorldBoundSphere( Sphere( bounds ) );
		usageGroup->applyObjectChanges();
	}

	void extractCachedUsageData( 
		Moo::ModelTextureUsageContext * context,
		CachedUsages * usages )
	{
		// Iterate over all the usages and extract textures and densities.
		const Moo::ModelTextureUsage * pUsages = NULL;
		size_t numUsages = 0;
		context->getUsages( &pUsages, &numUsages );

		for (size_t index = 0; index < numUsages; ++index)
		{
			const Moo::ModelTextureUsage & usage = pUsages[index];

			Moo::BaseTexturePtr pTexture = usage.pTexture_;
			usages->textureUVDensity_[pTexture] = usage.uvDensity_;
		}
	}

	void generateUsageCache_model( const BW::vector< BW::string > & models,
		CachedUsages * pUsages )
	{
		SuperModel superModel( models );

		BoundingBox modelBB;
		superModel.localVisibilityBox( modelBB );
		pUsages->localBoundingBox_ = modelBB;

		Moo::ModelTextureUsageContext context;
		Moo::ModelTextureUsageGroup::Data data;
		Moo::ModelTextureUsageGroup usageGroup( data, context );
		superModel.generateTextureUsage( usageGroup );

		extractCachedUsageData( &context, pUsages );
	}

	void generateUsageCache_tree( const char * resource, uint32 seed,
		CachedUsages * pUsages )
	{
#if SPEEDTREE_SUPPORT
		BW::speedtree::SpeedTreeRenderer speedtree;
		speedtree.load( resource, seed, Matrix::identity );

		pUsages->localBoundingBox_ = speedtree.boundingBox();

		Moo::ModelTextureUsageContext context;
		Moo::ModelTextureUsageGroup::Data data;
		Moo::ModelTextureUsageGroup usageGroup( data, context );
		speedtree.generateTextureUsage( usageGroup );

		extractCachedUsageData( &context, pUsages );
#endif
	}

	const CachedUsages & fetchUsageData_tree( const char * resource, uint32 seed )
	{
		// Create the key for the cache
		const char * separator = "||";
		const uint numCharsInStringUint32 = 14; // billion with commas and negative
		StringBuilder builder( strlen(resource) + 
			strlen(separator) + numCharsInStringUint32 + 1);
		builder.append( resource );
		builder.append( separator );
		builder.appendf( "%d", seed );

		// If this assertion is hit, then we didn't allocate enough space
		MF_ASSERT( !builder.isFull() );
		
		// Lookup the usage in the cache
		const char * key = builder.string();
		auto findIter = treeCache_.find( key );
		CachedUsages * pCachedUsages;
		if (findIter != treeCache_.end())
		{
			pCachedUsages = &findIter->second;
		}
		else
		{
			pCachedUsages = &treeCache_[key];
			generateUsageCache_tree( resource, seed, pCachedUsages );
		}

		return *pCachedUsages;
	}

	const CachedUsages & fetchUsageData_model( 
		const BW::vector< BW::string > & models )
	{
		// Create the key for the cache
		BW::string key;
		for (auto iter = models.begin(); iter != models.end(); ++iter)
		{
			const BW::string & model = *iter;
			key += model + "||";
		}

		// Lookup the usage in the cache
		auto findIter = treeCache_.find( key );
		CachedUsages * pCachedUsages;
		if (findIter != treeCache_.end())
		{
			pCachedUsages = &findIter->second;
		}
		else
		{
			pCachedUsages = &treeCache_[key];
			generateUsageCache_model( models, pCachedUsages );
		}

		return *pCachedUsages;
	}
};

StaticTextureStreamingWriter::StaticTextureStreamingWriter() :
	detail_( new Detail() )
{

}


StaticTextureStreamingWriter::~StaticTextureStreamingWriter()
{
}


bool StaticTextureStreamingWriter::initialize( 
	const DataSectionPtr & pSpaceSettings,
	const CommandLine & commandLine )
{
	// Nothing required
	return true;
}


void StaticTextureStreamingWriter::postProcess()
{
	// Sort all the usages by the texture resource that
	// they're defining usage for. This should improve cache performance
	// when processing usages, as all the usages of the same texture
	// will be processed together at the same time.
	struct UsageCompare
	{
		bool operator()( const StaticTextureStreamingTypes::Usage& a,
			const StaticTextureStreamingTypes::Usage& b )
		{
			return a.resourceID_ < b.resourceID_;
		}
	};
	std::sort( usages_.begin(), usages_.end(), UsageCompare() ); 

	// TODO: At this point, could probably optimise the usage set even more
	// and consolidate usages of the same texture that occupy the same region
	// of space. This would reduce the number of iterations required during
	// runtime.

	TRACE_MSG( "%d texture streaming usages baked.\n", usages_.size() );
}


bool StaticTextureStreamingWriter::write( BinaryFormatWriter& writer )
{
	BinaryFormatWriter::Stream * stream =
		writer.appendSection(
		StaticTextureStreamingTypes::FORMAT_MAGIC,
		StaticTextureStreamingTypes::FORMAT_VERSION );
	MF_ASSERT( stream != NULL );

	stream->write( usages_ );

	return true;
}


void StaticTextureStreamingWriter::convertModel( 
	const ConversionContext & ctx,
	const DataSectionPtr & pItemDS, const BW::string & uid )
{
	const bool isShell = false;
	addFromChunkModel( pItemDS, ctx.chunkTransform, 
		isShell, strings(), assets() );
}


void StaticTextureStreamingWriter::convertShell( 
	const ConversionContext & ctx,
	const DataSectionPtr & pItemDS, const BW::string & uid )
{
	const bool isShell = true;
	addFromChunkModel( pItemDS, ctx.chunkTransform, 
		isShell, strings(), assets() );
}


void StaticTextureStreamingWriter::convertSpeedTree( 
	const ConversionContext & ctx,
	const DataSectionPtr & pItemDS, const BW::string & uid )
{
	addFromChunkTree( pItemDS, ctx.chunkTransform, 
		strings(), assets() );
}


bool StaticTextureStreamingWriter::addFromChunkModel( 
	const DataSectionPtr & pItemDS,
	const Matrix & chunkTransform,
	bool isShell,
	StringTableWriter & stringTable,
	AssetListWriter & assetList )
{
	// Attempt to load the related supermodel and collect its
	// texture usage data
	MF_ASSERT( pItemDS != NULL );

	// Models
	BW::vector< BW::string > models;
	pItemDS->readStrings( "resource", models );
	if (models.empty())
	{
		ERROR_MSG( "StaticModelWriter: ChunkModel has no resources specified.\n" );
		return false;
	}

	// Transform
	Matrix worldTransform = chunkTransform;
	worldTransform.preMultiply(
		pItemDS->readMatrix34( "transform", Matrix::identity ) );

	// Now take this information and make it generate texture usage info:
	Moo::ModelTextureUsageContext context;
	Moo::ModelTextureUsageGroup::Data data;
	Moo::ModelTextureUsageGroup usageGroup( data, context );

	const Detail::CachedUsages & usages = 
		detail_->fetchUsageData_model( models );
	detail_->applyCachedUsages( worldTransform, usages, &usageGroup );

	// Now convert the usages generated to definitions
	Detail::convertUsagesToDefs( stringTable, assetList, context, usages_ );
	
	return true;
}


bool StaticTextureStreamingWriter::addFromChunkTree( 
	const DataSectionPtr & pItemDS,
	const Matrix & chunkTransform,
	StringTableWriter & stringTable,
	AssetListWriter & assetList )
{
	// Attempt to load the speedtree model and collect its usage data
	MF_ASSERT( pItemDS != NULL );

	// SpeedTree
	BW::string resource = pItemDS->readString( "spt" );
	if (resource.empty())
	{
		ERROR_MSG( "StaticTextureStreamingWriter: "
			"ChunkTree has no speedtree specified.\n" );
		return false;
	}

	// Seed
	uint32 seed = pItemDS->readInt( "seed", 1 );

	// Transform
	Matrix worldTransform = chunkTransform;
	worldTransform.preMultiply(
		pItemDS->readMatrix34( "transform", Matrix::identity ) );

	// Now take this information and make it generate texture usage info:

	Moo::ModelTextureUsageContext context;
	Moo::ModelTextureUsageGroup::Data data;
	Moo::ModelTextureUsageGroup usageGroup( data, context );
	
	const Detail::CachedUsages & usages = 
		detail_->fetchUsageData_tree( resource.c_str(), seed );
	detail_->applyCachedUsages( worldTransform, usages, &usageGroup );

	// Now convert the usages generated to definitions
	Detail::convertUsagesToDefs( stringTable, assetList, context, usages_ );

	return true;
}

} // namespace CompiledSpace
} // namespace BW
