#include "server_super_model.hpp"

#include "cstdmf/debug.hpp"
#include "cstdmf/stringmap.hpp"

#include "moo/node.hpp"
#include "moo/visual_common.hpp"
#include "model/model_common.hpp"

#include "physics2/bsp.hpp"

#include "resmgr/bwresource.hpp"
#include "resmgr/primitive_file.hpp"
#include "cstdmf/concurrency.hpp"

DECLARE_DEBUG_COMPONENT2( "Chunk", 0 );


BW_BEGIN_NAMESPACE

// TODO: Merge this implementation with Moo::EffectMaterial::load() somehow.
WorldTriangle::Flags calculateMaterialFlags( DataSectionPtr pMaterialSection )
{
	WorldTriangle::Flags flags = 0;
	BW::string mfmName =
		pMaterialSection->readString( "mfm" );

	DataSectionPtr pMFM;
	if (mfmName != "")
	{
		DataSectionPtr pMFMRoot =
			BWResource::instance().openSection( mfmName );

		if (pMFMRoot)
			pMFM = pMaterialSection;
	}

	DataSectionPtr pMaterial;
	if( pMFM )
		pMaterial = pMFM;
	else
		pMaterial = pMaterialSection;

	if (pMaterial)
	{
		flags = WorldTriangle::packFlags(
				pMaterial->readInt( "collisionFlags", 0 ),
				pMaterial->readInt( "materialKind", 0 ) );

		// __kyl__ (13/10/2006) Not sure why but the equivalent of this code is
		// commented out in Visual::Geometry::populateWorldTriangles().
		/*
		if (pMaterial->readBool( "Sorted" ) || pMaterial->readBool( "Alpha_test"))
		{
			flags |= TRIANGLE_TRANSPARENT;

			if (pMaterial->readBool( "Alpha_blended" ) &&
				pMaterial->readInt( "Src_Blend_type" ) == 4 &&	// SRC_ALPHA
				pMaterial->readInt( "Dest_Blend_type" ) == 5)	// INV_SRC_ALPHA
			{
				flags |= TRIANGLE_BLENDED;
			}
		}

		if (pMaterial->readBool( "Double_sided" ))
		{
			flags |= TRIANGLE_DOUBLESIDED;
		}
		*/
	}
	else
	{
		ERROR_MSG( "ServerModel::generateBSP: Failed to open %s\n",
			mfmName.c_str() );
	}

	return flags;
}

// -----------------------------------------------------------------------------
// Section: ServerVisual
// -----------------------------------------------------------------------------
/**
 * 	This is a skeleton Visual class for the server. Needed mainly to fit into
 * 	the VisualLoader framework.
 */
class ServerVisual
{
public:
	typedef BSPTree* BSPTreePtr;

	/**
	 * BSP cache. Indexed by the visual file path. The ownership of an added
	 * pointer is transferred to this cache.
	 */
	// TODO: Should do a proper cache and reference count BSPTree so that we
	// can delete them when we're done.
	class BSPCache
	{
		typedef BW::map< BW::string, BSPTree * > StringBSPTreeMap;
		StringBSPTreeMap cache_;

	public:
		void add( const BW::string& visualResID, BSPTree* pBSP )
		{
			cache_[ visualResID ] = pBSP;
		}

		BSPTree* find( const BW::string& visualResID )
		{
			StringBSPTreeMap::iterator iter = cache_.find( visualResID );
			return (iter != cache_.end()) ? iter->second : NULL;
		}

		static BSPCache& instance()
		{
			static BSPCache instance;
			return instance;
		}

		~BSPCache()
		{
			StringBSPTreeMap::iterator iter;
			for(iter = cache_.begin(); iter != cache_.end(); ++iter)
			{
				bw_safe_delete( iter->second );
			}
		}
	};

	/**
	 * TODO: to be documented.
	 */
	struct Material
	{
		WorldTriangle::Flags flags_;

		Material( DataSectionPtr pMaterialSection )
			: flags_( calculateMaterialFlags( pMaterialSection ) )
		{}

		WorldTriangle::Flags getFlags( int objectMaterialKind ) const
		{
			return (WorldTriangle::materialKind( flags_ ) != 0) ?
					flags_ :
					WorldTriangle::packFlags(
						WorldTriangle::collisionFlags( flags_ ),
						objectMaterialKind );
		}
	};

	/**
	 * Collection of Material. The ownership of an added pointer is transferred
	 * to this collection.
	 */
	class Materials : public BW::map< BW::string, Material* >
	{
		typedef BW::map< BW::string, Material* > BaseClass;
	public:
		const Material* find( const BW::string& identifier ) const
		{
			const_iterator iter = this->BaseClass::find( identifier );
			return (iter != this->end()) ? iter->second : NULL;
		}

		void add( const BW::string& identifier, Material* pMaterial )
		{
			if (!this->insert( value_type( identifier, pMaterial ) ).second)
				bw_safe_delete( pMaterial );
		}

		~Materials()
		{
			iterator iter = this->BaseClass::begin();
			iterator end  = this->BaseClass::end();
			for(; iter != end; ++iter)
			{
				bw_safe_delete( iter->second );
			}
		}
	};

private:

	BSPTree* 	pBSPTree_;

public:
	// Constructor. Loads visual file.
	ServerVisual( const BW::string& visualBaseName, BoundingBox& boundingBox) :
			pBSPTree_( NULL )
	{
		Moo::VisualLoader< ServerVisual > loader( visualBaseName );
		if (!loader.getRootDataSection())
			return;

		loader.setBoundingBox( boundingBox );

		Moo::BSPMaterialIDs bspMaterialIDs;
		bool shouldRemapBSPFlags =
				loader.loadBSPTree( pBSPTree_, bspMaterialIDs );
		if (pBSPTree_ && shouldRemapBSPFlags)
		{
			Moo::BSPMaterialIDs 	missingBspMaterialIDs;
			Materials materials;
			loader.collectMaterials( materials );
			loader.remapBSPMaterialFlags( *pBSPTree_, materials,
					bspMaterialIDs, missingBspMaterialIDs );
		}
	}

	// This method will be used to load an old style _bsp.visual file.
	static BSPTree* loadBSPVisual( const BW::string& visualResID )
	{
		if ( BWResource::openSection( visualResID ) )
		{
			BoundingBox		dummyBoundingBox;
			ServerVisual 	dummyVisual( visualResID, dummyBoundingBox );
			return dummyVisual.getBSPTree();
		}
		else
		{
			return NULL;
		}
	}

	BSPTree* 		getBSPTree()		{ return pBSPTree_; }
}; // end ServerVisual class


// -----------------------------------------------------------------------------
// Section: ServerModel
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
ServerModel::ServerModel( BSPTree * pTree, BoundingBox & bb ) :
	pTree_( pTree ),
	bb_( bb )
{
}


/**
 *	Constructor.
 */
ServerModel::ServerModel( const BW::string & type, DataSectionPtr pFile ) :
	pTree_( NULL )
{
	BW::string visual = pFile->readString( type );

	// Dummy ServerVisual to load the visual file.
	ServerVisual serverVisual( visual, bb_ );
	pTree_ = serverVisual.getBSPTree();

#ifdef COLLISION_DEBUG
	// __kyl__ (12/10/2006) This is no longer the name of the actual BSP
	// file being used... mainly due to difficulties in obtaining that
	// name because BSP loading is quite complicated and we didn't update
	// the functions the pass the name around.
	if (pTree_)
		pTree_->name( visual );
#endif
}


// -----------------------------------------------------------------------------
// Section: NodefullModel
// -----------------------------------------------------------------------------

/**
 *	This class is used by the server to represent a nodeless server model.
 */
class NodefullModel : public ServerModel
{
public:
	NodefullModel( const BW::string & resourceID, DataSectionPtr pFile,
		const BW::string & vname ) :
			ServerModel( vname, pFile )
	{
	}
};


// -----------------------------------------------------------------------------
// Section: NodelessModel
// -----------------------------------------------------------------------------

/**
 *	This class is used by the server to represent a nodeless server model.
 */
class NodelessModel : public ServerModel
{
public:
	NodelessModel( const BW::string & resourceID, DataSectionPtr pFile,
		const BW::string & vname ) :
			ServerModel( vname, pFile )
	{
	}
};


// -----------------------------------------------------------------------------
// Section: Model
// -----------------------------------------------------------------------------

namespace
{
typedef StringHashMap< ModelPtr > ModelMap;
ModelMap s_pModelMap;
SimpleMutex s_pModelMapLock;
}

/**
 *	This static method gets the model with the given resource ID.
 */
ModelPtr Model::get( const BW::string & resourceID )
{
	{ // s_pModelMapLock
		SimpleMutexHolder smh( s_pModelMapLock );
		ModelMap::iterator found = s_pModelMap.find( resourceID );

		// Iterator is only valid inside the mutex
		if (found != s_pModelMap.end())
		{
			return found->second;
		}
	} // !s_pModelMapLock


	DataSectionPtr pFile = BWResource::openSection( resourceID );
	if (!pFile)
	{
		ERROR_MSG( "Model::get: Could not open model resource %s\n",
				resourceID.c_str() );
		return NULL;
	}

	ModelPtr pModel = Model::load( resourceID, pFile );

	if (pModel)
	{ // s_pModelMapLock
		SimpleMutexHolder smh( s_pModelMapLock );
		s_pModelMap[ resourceID ] = pModel;
	} // !s_pModelMapLock

	return pModel;
}

/**
 * @internal
 * TODO: to be documented.
 */
// This factory class is passed to Moo::createModelFromFile()
class ModelFactory
{
	const BW::string& 	resourceID_;
	DataSectionPtr& 	pFile_;
public:
	typedef Model ModelBase;

	ModelFactory( const BW::string& resourceID, DataSectionPtr& pFile ) :
		resourceID_( resourceID ), pFile_( pFile )
	{}

	NodefullModel* newNodefullModel()
	{
		return new NodefullModel( resourceID_, pFile_, "nodefullVisual" );
	}

	NodelessModel* newNodelessModel()
	{
		return new NodelessModel( resourceID_, pFile_, "nodelessVisual" );
	}

	Model* newBillboardModel()
	{
		INFO_MSG( "Model::load: "
				"Billboard models are not handled in the server\n" );
		return NULL;
	}
};

/**
 *	This static method loads a model.
 */
Model* Model::load( const BW::string & resourceID,
		DataSectionPtr pFile )
{
	ModelFactory factory( resourceID, pFile );
	Model * pModel = Moo::createModelFromFile( pFile, factory );

	if (!pModel)
	{
		ERROR_MSG( "Model::load: "
				"Could not determine type of model in resource %s\n",
			resourceID.c_str() );
	}
	else if (!pModel->valid())
	{
		delete pModel;
		pModel = NULL;
	}

	return pModel;
}


// -----------------------------------------------------------------------------
// Section: SuperModel
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
SuperModel::SuperModel( const BW::vector< BW::string > & modelIDs )
{
	BW::vector< BW::string >::const_iterator iter = modelIDs.begin();

	while (iter != modelIDs.end())
	{
		ModelPtr pModel = Model::get( *iter );

		if (pModel)
		{
			models_.push_back( pModel );
		}
		else
		{
			// It's actually OK for skinned objects not to have a BSP, which
			// results in it not being loaded in the server since it's not
			// useful.
//			ERROR_MSG( "SuperModel::SuperModel: Could not get %s\n",
//					iter->c_str() );
		}

		iter++;
	}
}

void SuperModel::boundingBox( BoundingBox& bbRet ) const
{
	if (!models_.empty())
	{
		bbRet = models_[0]->boundingBox();

		for (unsigned int i = 1; i <models_.size(); i++)
		{
			bbRet.addBounds( models_[i]->boundingBox() );
		}
	}
	else
	{
		bbRet = BoundingBox( Vector3(0,0,0), Vector3(0,0,0) );
	}
}

BW_END_NAMESPACE

// server_super_model.cpp
