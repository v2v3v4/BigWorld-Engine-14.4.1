#include "pch.hpp"
#include "ecotype_generators.hpp"
#include "flora.hpp"
#include "flora_renderer.hpp"
#include "flora_texture.hpp"
#include "cstdmf/debug.hpp"
#include "moo/vertex_format_cache.hpp"
#include "moo/visual_manager.hpp"
#include "moo/visual_common.hpp"
#include "flora_constants.hpp"

DECLARE_DEBUG_COMPONENT2( "Flora", 0 )


BW_BEGIN_NAMESPACE

//class factory methods
typedef EcotypeGenerator* (*EcotypeGeneratorCreator)();
typedef BW::map< BW::string, EcotypeGeneratorCreator > CreatorFns;
CreatorFns g_creatorFns;

#define ECOTYPE_FACTORY_DECLARE( b ) EcotypeGenerator* b##Generator() \
	{															 \
		return new b##Ecotype;									 \
	}

#define REGISTER_ECOTYPE_FACTORY( a, b ) g_creatorFns[a] = b##Generator;


typedef ChooseMaxEcotype::Function* (*FunctionCreator)();
typedef BW::map< BW::string, FunctionCreator > FunctionCreators;
FunctionCreators g_fnCreators;
#define FUNCTION_FACTORY_DECLARE( b ) ChooseMaxEcotype::Function* b##Creator() \
	{															 \
		return new b##Function;									 \
	}

#define REGISTER_FUNCTION_FACTORY( a, b ) g_fnCreators[a] = b##Creator;



#if ENABLE_RELOAD_MODEL
/*static*/ BW::list<VisualsEcotype*> FloraVisualManager::visualEcotypes_;
/*static*/ FloraVisualManager::StringRefVisualEcotypesMap FloraVisualManager::floraTextureDependencies_;
/*static*/ SimpleMutex FloraVisualManager::mutex_;
#endif//RELOAD_MODEL_SUPPORT

/**
 *	This method creates and initialises an EcotypeGenerator given
 *	a data section.  The legacy xml grammar of a list of &lt;visual>
 *	sections is maintained; also the new grammar of a &lt;generator>
 *	section is introduced, using the g_creatorFns factory map.
 *
 *	@param pSection	Section containing generator data.
 *	@param target	Ecotype to store the texture resource used by
 *					the generator, or empty string ("").
 */
EcotypeGenerator* EcotypeGenerator::create( DataSectionPtr pSection,
	Ecotype& target )
{	
	BW_GUARD;
	EcotypeGenerator* eg = NULL;
	DataSectionPtr pInitSection = NULL;

	if (pSection)
	{
		//legacy support - visuals directly in the section
		if ( pSection->findChild("visual") )
		{			
			eg = new VisualsEcotype();
			pInitSection = pSection;			
		}
		else if ( pSection->findChild("generator") )
		{			
			DataSectionPtr pGen = pSection->findChild("generator");
			const BW::string& name = pGen->asString();
			if (name != "")
			{
				CreatorFns::iterator it = g_creatorFns.find(name);
				if ( it != g_creatorFns.end() )
				{
					EcotypeGeneratorCreator egc = it->second;
					eg = egc();
					pInitSection = pGen;
				}
				else
				{
					ERROR_MSG( "Unknown ecotype generator %s\n", name.c_str() );
				}
			}
		}

		if (eg)
		{
			if (eg->load(pInitSection,target) )
			{
				return eg;
			}
			else
			{
				bw_safe_delete(eg);
			}
		}
	}

    target.textureResource("");
	target.pTexture(NULL);
	return new EmptyEcotype();
}


/**
 *	This method implements the transformIntoVB interface for
 *	EmptyEcotype.  It simply NULLs out the vertices.
 */
uint32 EmptyEcotype::generate(		
		const Vector2& uvOffset,
		FloraVertexContainer* pVerts,
		uint32 idx,
		uint32 maxVerts,
		const Matrix& objectToWorld,
		const Matrix& objectToChunk,
		BoundingBox& bb )
{
	BW_GUARD;
	uint32 nVerts = min(maxVerts, (uint32)12);
	if (pVerts)
		pVerts->clear( nVerts );
	return nVerts;
}

#if ENABLE_RELOAD_MODEL

/**
 *	This method  tell the ecotype to re-swap texture.
*/
void VisualsEcotype::VisualCopy::refreshTexture()
{
	BW_GUARD;
	pTarget_->refreshTexture();
}


/**
 *	This method update the visuals from the given pVisual for 
 *	the matched visuals.
 *	@param	visualResourceID the resource name to be matched.
 *	@param	pVisual the Visual object that we pulll info from.
 *	@return true on success.
 */
bool VisualsEcotype::updateVisual( const BW::StringRef& visualResourceID,
						Moo::Visual* pVisual )
{
	bool succeed = true;

	VisualCopies::iterator itv = visuals_.begin();
	for ( ; itv != visuals_.end(); ++itv )
	{
		if ((*itv).visual_ == visualResourceID)
		{
			if ((*itv).pullInfoFromVisual( pVisual ))
			{
				(*itv).refreshTexture();
			}
			else
			{
				succeed = false;
			}
		}
	}
	return succeed;
}


/**
 *	This method  check if there are loaded flora visa matched visualResourceID
 *	@param	visualResourceID the resource name to be matched.
 *	@return true if there is.
*/
bool VisualsEcotype::hasVisual( const BW::StringRef& visualResourceID )
{
	BW_GUARD;
	VisualCopies::iterator itv = visuals_.begin();
	for (; itv != visuals_.end(); ++itv)
	{
		if ((*itv).visual_ == visualResourceID)
		{
			return true;
		}
	}
	return false;
}

void FloraVisualManager::init()
{
	BW_GUARD;
	BWResource::instance().addModificationListener( this );
}

void FloraVisualManager::fini()
{
	BW_GUARD;
	// Check if BWResource singleton exists as this singleton could
	// be destroyed afterwards
	if (BWResource::pInstance() != NULL)
	{
		BWResource::instance().removeModificationListener( this );
	}
}

class FloraVisualManager::DependencyReloadContext
{
public:
	void reload( ResourceModificationListener::ReloadTask& task )
	{
		SimpleMutexHolder mutexHolder( mutex_ );

		BW::StringRef baseName = BWResource::removeExtension( task.resourceID() );
		StringRefVisualEcotypesMap::const_iterator findIt = 
			floraTextureDependencies_.find( baseName );
		// check if modified file is one of active flora textures
		if (findIt != floraTextureDependencies_.end())
		{
			BWResource::WatchAccessFromCallingThreadHolder accessWatcher( false );

			// collect list of flora textures which needs to be rebuilt
			// multiple visual ecotypes could use the same flora
			// we want to avoid re-creating the same texture multiple times
			DynamicEmbeddedArray<FloraTexture*, 16> texturesToRebuild;
			const VisualEcotypes& ecs = (*findIt).second;
			for (VisualEcotypes::const_iterator it = ecs.begin(); 
				it != ecs.end(); ++it)
			{
				texturesToRebuild.add_unique( (*it)->floraTexture() );
			}
			// rebuild flora textures which depend on changed texture
			for (uint i = 0; i < texturesToRebuild.size(); ++i)
			{
				texturesToRebuild[i]->createUnmanagedObjects();
			}
		}
	}
};

class FloraVisualManager::VisualReloadContext
{
public:
	void reload( ResourceModificationListener::ReloadTask& task )
	{
		BWResource::instance().purge( task.resourceID(), true );

		bool succeed = FloraVisualManager::updateVisual( task.resourceID() );	

		TRACE_MSG( "FloraVisualManager: modified '%s', reload %s\n",
			task.resourceID().c_str(), succeed? "succeed.":"failed." );
	}
};

/**
 *	Notification from the file system that something we care about has been
 *	modified at run time. Let us reload visuals.
 */
void FloraVisualManager::onResourceModified( 
	const BW::StringRef& basePath, const BW::StringRef& resourceID,
	ResourceModificationListener::Action action )
{
	BW_GUARD;

#if ENABLE_RELOAD_MODEL
	if (!Reloader::enable())
		return;

	BW::StringRef baseName = BWResource::removeExtension( resourceID );
	BW::StringRef ext = BWResource::getExtension( resourceID );
	{
		SimpleMutexHolder mutexHolder( mutex_ );

		StringRefVisualEcotypesMap::const_iterator findIt = 
								floraTextureDependencies_.find( baseName );
		// check if modified file is one of active flora textures
		if (findIt != floraTextureDependencies_.end())
		{
			// if this is not a dds we need to trigger conversion first
			if (ext != "dds")
			{
				// trigger source texture->dds conversion
				Moo::TextureManager::instance()->prepareResource( resourceID );
			}
			else
			{
				queueReloadTask( DependencyReloadContext(), basePath, 
					resourceID );
			}

		}
	}
	if ( ext != "visual" )
		return;

	if (FloraVisualManager::hasVisual( resourceID ))
	{
		queueReloadTask( VisualReloadContext(), basePath, 
			resourceID );
	}
#endif//RELOAD_MODEL_SUPPORT

}



/**
 *	This method adds a VisualsEcotype to the list
 */
/*static*/void FloraVisualManager::addVisualsEcotype( VisualsEcotype* pVisualEcotype )
{
	BW_GUARD;
	SimpleMutexHolder mutexHolder( mutex_ );
	visualEcotypes_.push_back( pVisualEcotype );
}




/**
 *	This method deletes a VisualsEcotype from the list and
 *  Updates flora texture dependency list
 */
/*static*/ void FloraVisualManager::delVisualsEcotype( VisualsEcotype* pVisualEcotype )
{
	BW_GUARD;
	SimpleMutexHolder mutexHolder( mutex_ );

	BW::list<VisualsEcotype*>::iterator found =
		std::find( visualEcotypes_.begin(), visualEcotypes_.end(), pVisualEcotype );
	if (found != visualEcotypes_.end())
	{
		visualEcotypes_.erase( found );
	}
	// search for all textures which this ecotypes depends on
	StringRefVisualEcotypesMap::iterator texIt = floraTextureDependencies_.begin();
	while (texIt != floraTextureDependencies_.end())
	{
		VisualEcotypes& ecotypes = (*texIt).second;
		VisualEcotypes::iterator ecoIt = ecotypes.begin();
		// remove given ecotype pointer from the array
		while (ecoIt != ecotypes.end())
		{
			if ((*ecoIt) == pVisualEcotype)
			{
				ecoIt = ecotypes.erase( ecoIt );
			}
			else
			{
				++ecoIt;
			}
		}
		// remove map entry if there are no dependencies left
		if (ecotypes.empty())
		{
			floraTextureDependencies_.erase( texIt++ );
		}
		else
		{
			++texIt;
		}
	}
}


/**
 *	This method update all the flora visuals whose resource is visualResourceID
 *	@param	visualResourceID the resource name to be matched.
 *	@return true on success.
 */
bool FloraVisualManager::updateVisual( const BW::StringRef& visualResourceID )
{
	BW_GUARD;

	Moo::Visual visual( visualResourceID, true, Moo::Visual::LOAD_GEOMETRY_ONLY );
	visual.isInVisualManager( false );
	if (!visual.isOK())
		return false;

	bool succeed = true;

	{
		SimpleMutexHolder mutexHolder( mutex_ );
		BW::list<VisualsEcotype*>::iterator ite = visualEcotypes_.begin();
		for (; ite != visualEcotypes_.end(); ++ite)
		{
			succeed &= (*ite)->updateVisual( visualResourceID, &visual );
		}

	}

	return succeed;
}

/**
 *	This method registers a flora texture dependency for given visual id
 */
void FloraVisualManager::addFloraTextureDependency( const BW::string& textureName,
												VisualsEcotype* visualEcotype )
{
	BW_GUARD;
	SimpleMutexHolder mutexHolder( mutex_ );

	BW::StringRef baseTextureName = BWResource::removeExtension( textureName );
	StringRefVisualEcotypesMap::iterator findIt = 
				floraTextureDependencies_.find( baseTextureName );

	if (findIt == floraTextureDependencies_.end())
	{
		floraTextureDependencies_[baseTextureName] = VisualEcotypes( 1, visualEcotype );
	}
	else
	{
		// add ecotype if we don't have it in the list already
		VisualEcotypes& ecs = (*findIt).second;
		if (std::find(ecs.begin(), ecs.end(), visualEcotype ) == ecs.end())
		{
			ecs.push_back( visualEcotype );
		}
	}
}


/**
 *	This method  check if there are loaded flora visa matched visualResourceID
 *	@param	visualResourceID the resource name to be matched.
 *	@return true if there is.
*/
bool FloraVisualManager::hasVisual( const BW::StringRef& visualResourceID )
{
	BW_GUARD;
	SimpleMutexHolder mutexHolder( mutex_ );

	BW::list<VisualsEcotype*>::iterator ite = visualEcotypes_.begin();
	for (; ite!=visualEcotypes_.end(); ++ite)
	{
		if ((*ite)->hasVisual( visualResourceID ))
		{
			return true;
		}
	}
	return false;
}



#endif//RELOAD_MODEL_SUPPORT


VisualsEcotype::VisualsEcotype():
	density_( 1.f )
{	
	BW_GUARD;

#if ENABLE_RELOAD_MODEL
	FloraVisualManager::addVisualsEcotype( this );
#endif//RELOAD_MODEL_SUPPORT
}

VisualsEcotype::~VisualsEcotype()
{
	BW_GUARD;

#if ENABLE_RELOAD_MODEL
	FloraVisualManager::delVisualsEcotype( this );
#endif//RELOAD_MODEL_SUPPORT
}


/**
 *	This method implements the EcotypeGenerator::load interface
 *	for the VisualsEcotype class.
 */
bool VisualsEcotype::load( DataSectionPtr pSection, Ecotype& target)
{
	BW_GUARD;
	flora_ = target.flora();

	density_ = pSection->readFloat( "density", 1.f );

	target.textureResource("");
	target.pTexture(NULL);
	BW::vector<DataSectionPtr>	pSections;
	pSection->openSections( "visual", pSections );

	for ( uint i=0; i<pSections.size(); i++ )
	{
		DataSectionPtr curr = pSections[i];

		BW::string visualResID = curr->asString();
		Moo::VisualPtr visual;

		if (Moo::VisualLoader<Moo::Visual>::visualFileExists( visualResID ))
		{
			visual = new Moo::Visual( visualResID, true, Moo::Visual::LOAD_GEOMETRY_ONLY );
			visual->isInVisualManager( false );
		}

		if ( visual )
		{
			float flex = curr->readFloat( "flex", 1.f );
			float scaleVariation = curr->readFloat( "scaleVariation", 0.f );
			float tintFactor = curr->readFloat("tintFactor", 1.0f);

			VisualCopy visCopy;
#ifdef EDITOR_ENABLED
			visCopy.highLight_ = curr->readBool( "highlight", false );
#endif
			bool ok = visCopy.set( visual, flex, scaleVariation, tintFactor, flora_, &target );
			if (ok)
			{
				visuals_.push_back( visCopy );
				// add texture dependencies
				#if ENABLE_RELOAD_MODEL
				Ecotype* ecotype = visCopy.pTarget_;
				if (ecotype)
				{
					// add texture dependency for this ecotype
					const BW::string& texName = ecotype->textureResource();
					if (!texName.empty())
					{
						FloraVisualManager::addFloraTextureDependency( texName, this );
					}
				}
				#endif//ENABLE_RELOAD_MODEL
			}
			else
			{
				DEBUG_MSG( "VisualsEcotype - Removing %s because VisualCopy::set failed\n", 
					pSections[i]->asString().c_str() );
			}
		}
		else
		{
			DEBUG_MSG( "VisualsEcotype - %s does not exist\n", 
					pSections[i]->asString().c_str() );
		}
	}

	return (visuals_.size() != 0);
}


/**
 *	This method takes a visual, and creates a triangle list of FloraVertices.
 */
bool VisualsEcotype::VisualCopy::set( Moo::VisualPtr pVisual,
							float flex, float scaleVariation,
							float tintFactor,
							Flora* flora, Ecotype* pTarget )
{
	BW_GUARD;

	scaleVariation_ = scaleVariation;
	flex_ = flex;
	flora_ = flora;
	visual_ = pVisual->resourceID();
	pTarget_ = pTarget;

	return this->pullInfoFromVisual( pVisual.get() );
}



/**
 *	This method creates a triangle list of FloraVertices.
 *	@param pVisual	the visual that we pull info from
 *	@param forceSwapTexture	if we want to force the picture of pVisual to be swapped in
 */
bool VisualsEcotype::VisualCopy::pullInfoFromVisual( Moo::Visual* pVisual )
{

	if (!pVisual)
		return false;

	const Moo::VertexFormat* pFormat = 
		Moo::VertexFormatCache::get< Moo::VertexXYZNUV >();
	void * verts;
	uint32 nVerts;
	Moo::IndicesHolder	 indices;
	uint32 nIndices;
	Moo::EffectMaterialPtr  material;

	vertices_.clear();
	if ( pVisual->createCopy( *pFormat, verts, indices, nVerts, nIndices, material ) )
	{
		Moo::VertexXYZNUV * vertices = static_cast< Moo::VertexXYZNUV * >( verts );
		vertices_.resize( nIndices );

		//preprocess uvs
		BoundingBox bb;
		bb.setBounds(	Vector3(0.f,0.f,0.f),
						Vector3(0.f,0.f,0.f) );

		float numBlocksWide = float(flora_->floraTexture()->blocksWide());
		float numBlocksHigh = float(flora_->floraTexture()->blocksHigh());
		for (uint32 i = 0; i < nVerts; ++i)
		{
			Moo::VertexXYZNUV& vert = vertices[i];

			vert.uv_.x /= numBlocksWide;
			vert.uv_.y /= numBlocksHigh;

			bb.addBounds(vert.pos_);
		}

		float totalHeight = bb.maxBounds().y - bb.minBounds().y;

		//-- copy the mesh
		for (uint32 i = 0; i < nIndices; ++i)
		{
			const Moo::VertexXYZNUV& iVert = vertices[indices[i]];
			FloraVertex&			 oVert = vertices_[i];

			oVert.set(iVert);
			
			//-- calculate vertex's flex parameter.
			float relativeHeight = (iVert.pos_.y - bb.minBounds().y) / 2.0f;
			{
				if (relativeHeight>1.f)
					relativeHeight=1.f;

				oVert.flex(relativeHeight > 0.1f ? flex_ * relativeHeight : 0.f);
			}

			//-- find correct tint position.
			//--	 This relatively expensive task is made only once and may be even precomputed to
			//--	 save loading time.
			{
				Vector3 tintPos = iVert.pos_;
				if (almostZero(1.0f - iVert.normal_.dotProduct(Vector3(0,1,0))))
				{
					tintPos.y = 0.f;
				}
				else
				{
					//-- build TBN basis matrix.
					Matrix m;
					m.lookAt(Vector3(0,0,0), iVert.normal_, Vector3(0,1,0));
					m.invert();

					//-- extract and normalize up vector.
					Vector3 up = m.applyToUnitAxisVector(1);
					up.normalise();

					//-- find offset along already calculated up vector.
					float cosA   = up.dotProduct(Vector3(0,1,0));
					float offset = iVert.pos_.y / cosA;

					//-- find tint position.
					tintPos -= up * offset;
				}

				//-- for flora coloring we need real relativeHeight.
				relativeHeight = (iVert.pos_.y - bb.minBounds().y) / totalHeight;
				
				//-- ... and write in to the output vertex.
				oVert.tint_ = Vector4(tintPos, relativeHeight);
			}
		}

		bw_safe_delete_array<uint8*>(*(uint8**)&verts);

		return VisualsEcotype::findTextureResource( visual_, *pTarget_ );
	}

	return false;
}


/**
 *	This method extracts the first texture property from the given visual.
 */
bool VisualsEcotype::findTextureResource(const BW::StringRef& resourceID,Ecotype& target)
{
	BW_GUARD;
	BW::string texName;
	bool found = VisualsEcotype::findTextureName( resourceID, texName );
	if (found && texName != target.textureResource())
	{
		target.textureResource( texName );
		Moo::BaseTexturePtr pMooTexture =
			Moo::TextureManager::instance()->getSystemMemoryTexture( texName );
		target.pTexture( pMooTexture );
	}
	return found;
}


/**
 *	This method extracts the first texture property from the given visual.
 *	and return the texture name into texName if succeed.
 */
/*static*/ bool VisualsEcotype::findTextureName( const BW::StringRef& resourceID, BW::string& texName )
{
	BW_GUARD;
	DataSectionPtr pMaterial = NULL;
	DataSectionPtr pSection =
		Moo::VisualLoader<Moo::Visual>::loadVisual( resourceID );

	if ( pSection )
	{
		DataSectionPtr pRenderset = pSection->openSection("renderSet");
		if (pRenderset)
		{
			DataSectionPtr pGeometry = pRenderset->openSection("geometry");
			if (pGeometry)
			{
				DataSectionPtr pPrimitiveGroup = pGeometry->openSection("primitiveGroup");
				if (pPrimitiveGroup)
				{
					pMaterial = pPrimitiveGroup->openSection("material");
				}
			}
		}
	}

	if (!pMaterial)
	{
		ASSET_MSG( "No material in flora visual %s\n", resourceID.to_string().c_str() );
		return false;
	}

	BW::string mfmName = pMaterial->readString( "mfm" );
	if ( mfmName != "" )
	{
		pMaterial = BWResource::openSection( mfmName );
		if ( !pMaterial )
		{
			ASSET_MSG( "MFM (%s) referred to in visual (%s) does not exist\n", mfmName.c_str(), resourceID.to_string().c_str() );
			return false;
		}
	}

	//First see if we've got a material defined in the visual file itself
	BW::vector<DataSectionPtr> pProperties;
	pMaterial->openSections("property",pProperties);
	BW::vector<DataSectionPtr>::iterator it = pProperties.begin();
	BW::vector<DataSectionPtr>::iterator end = pProperties.end();
	while (it != end)
	{
		DataSectionPtr pProperty = *it++;
		DataSectionPtr pTexture = pProperty->findChild( "Texture" );
		if (pTexture)
		{
			texName = pProperty->readString("Texture");
			return true;
		}
	}

	ASSET_MSG( "Could not find a texture property in the flora visual %s\n", resourceID.to_string().c_str() );
	return false;
}



#ifdef EDITOR_ENABLED
/**
 *	This method high light the tree branch according to
 *	the indexPath list generated from flora setting tree view
 */
bool VisualsEcotype::highLight( FloraVertexContainer* pVerts, BW::list<int>& indexPath, bool highLight  )
{
	BW_GUARD;

	int visualIndex = -1;
	if (!indexPath.empty())
	{
		visualIndex = indexPath.front();
		indexPath.pop_front();
	}
	Vector2 uvOffsetOverride = (highLight)? 
		flora_->floraTexture()->highLightTextureUV():
		pVerts->pCurEcotype_->uvOffset();
	pVerts->uvOffset( uvOffsetOverride.x, uvOffsetOverride.y );


	BW::vector< uint32 > offsets;
	if (visualIndex != -1)
	{
		if (visualIndex < static_cast<int>(visuals_.size()))
		{
			VisualCopy& visualCopy = visuals_[visualIndex];
			visualCopy.highLight_ = highLight;

			for (size_t i = 0; i < BLOCK_STRIDE; i++)
			{
				for (size_t j = 0; j< BLOCK_STRIDE; j++)
				{
					offsets.clear();
					pVerts->blocks_[i * BLOCK_STRIDE + j].findVisualCopyVerticeOffset( (int*)&visualCopy, offsets );

					BW::vector< uint32 >::iterator ito = offsets.begin();
					for (; ito != offsets.end(); ++ito)
					{
						FloraVertex* pCurVert = pVerts->pVerts( *ito ) + pVerts->blocks_[i * BLOCK_STRIDE + j].offset();
						BW::vector< FloraVertex >::iterator itV = visualCopy.vertices_.begin();
						for (; itV != visualCopy.vertices_.end(); ++itV)
						{
							pCurVert->uv_[0] = (*itV).uv_[0];
							pCurVert->uv_[1] = (*itV).uv_[1];
							pVerts->offsetUV( pCurVert );
							pCurVert++;
						}
					}
				}
			}
			return true;
		}
	}
	else
	{
		VisualCopies::iterator it = visuals_.begin();
		VisualCopies::iterator end = visuals_.end();
		while (it != end)
		{
			VisualCopy& visualCopy = *it;
			visualCopy.highLight_ = highLight;
			for (size_t i = 0; i < BLOCK_STRIDE; i++)
			{
				for (size_t j = 0; j < BLOCK_STRIDE; j++)
				{
					offsets.clear();
					pVerts->blocks_[i * BLOCK_STRIDE + j].findVisualCopyVerticeOffset( (int*)&visualCopy, offsets );

					BW::vector< uint32 >::iterator ito = offsets.begin();
					for (; ito != offsets.end(); ++ito)
					{
						FloraVertex* pCurVert = pVerts->pVerts( *ito ) + pVerts->blocks_[i * BLOCK_STRIDE + j].offset();
						BW::vector< FloraVertex >::iterator itV = visualCopy.vertices_.begin();
						for (; itV != visualCopy.vertices_.end(); ++itV)
						{
							pCurVert->uv_[0] = (*itV).uv_[0];
							pCurVert->uv_[1] = (*itV).uv_[1];
							pVerts->offsetUV( pCurVert );
							pCurVert++;
						}
					}
				}
			}
			it++;
		}
	}
	return true;
}


/**
 *	This method high light the tree branch according to
 *	the indexPath list generated from flora setting tree view
 */
bool ChooseMaxEcotype::highLight( FloraVertexContainer* pVerts, BW::list<int>& indexPath, bool highLight  )
{
	BW_GUARD;
	bool ret = true;
	int functionIndex = -1;
	if (!indexPath.empty())
	{
		functionIndex = indexPath.front();
		indexPath.pop_front();
	}

	if (functionIndex != -1)
	{
		int i = 0;
		SubTypes::iterator it = subTypes_.begin();
		SubTypes::iterator end = subTypes_.end();
		while (it != end)
		{
			if (i++ == functionIndex)
			{
				EcotypeGenerator& gen = *it->second;
				return gen.highLight( pVerts, indexPath, highLight );
			}
			it++;
		}
	}
	else
	{
		SubTypes::iterator it = subTypes_.begin();
		SubTypes::iterator end = subTypes_.end();
		while (it != end)
		{
			EcotypeGenerator& gen = *it->second;
			ret &= gen.highLight( pVerts, indexPath, highLight );
			it++;
		}
	}
	return ret;
}

#endif


/**
 *	This method fills the vertex buffer with a single object, and returns the
 *	number of vertices used.
 */
uint32 VisualsEcotype::generate(	
	const Vector2& uvOffset,
	FloraVertexContainer* pVerts,
	uint32 idx,
	uint32 maxVerts,
	const Matrix& objectToWorld,
	const Matrix& objectToChunk,
	BoundingBox& bb )
{
	BW_GUARD;
	if ( visuals_.size() )
	{			
		Vector2 uvOffsetOverride = uvOffset;

		VisualCopy& visual = visuals_[idx%visuals_.size()];

		uint32 nVerts = static_cast<uint32>( visual.vertices_.size() );

		if ( nVerts > maxVerts)
			return 0;

		//nextRandomFloat are here to keep determinism.
		float rDensity = flora_->nextRandomFloat();
		float rScale = flora_->nextRandomFloat();

		if ( rDensity < density_ )
		{
			if (pVerts)
			{
				Matrix scaledObjectToChunk;
				Matrix scaledObjectToWorld;

				float scale = fabsf(rScale) * visual.scaleVariation_ + 1.f;
				scaledObjectToChunk.setScale( scale, scale, scale );
				scaledObjectToChunk.postMultiply( objectToChunk );
				scaledObjectToWorld.setScale( scale, scale, scale );
				scaledObjectToWorld.postMultiply( objectToWorld );

				//calculate animation block number for this object
				int blockX = abs((int)floorf(objectToWorld.applyToOrigin().x / BLOCK_WIDTH));
				int blockZ = abs((int)floorf(objectToWorld.applyToOrigin().y / BLOCK_WIDTH));
				int blockNum = (blockX%7 + (blockZ%7) * 7);

#ifdef EDITOR_ENABLED
				if (visual.highLight_)
				{
					uvOffsetOverride = flora_->floraTexture()->highLightTextureUV();
				}
				if (!pVerts->pCurBlock_->recordAVisualCopy( (int*)&visual, pVerts->curOffset() ))
				{
					ERROR_MSG( "VisualsEcotype::generate, recorded too many Visal copy in the block!"
						"Some visuals might not be able to be high lighten when using flora editor.\n" );
				}
#endif
				pVerts->uvOffset( uvOffsetOverride.x, uvOffsetOverride.y );
				pVerts->blockNum( blockNum );
				pVerts->addVertices( &*visual.vertices_.begin(), nVerts, &scaledObjectToWorld );			

				for ( uint32 i=0; i<nVerts; i++ )
				{				
					bb.addBounds( scaledObjectToWorld.applyPoint( visual.vertices_[i].pos_ ) );
				}
			}

			return nVerts;			
		}
		else
		{
			if (pVerts)
			{
				pVerts->clear( nVerts );
			}
			return nVerts;
		}
	}

	return 0;
}


/**
 *	This method returns true if the ecotype is empty.  Being empty means
 *	it will not draw any detail objects, and neighbouring ecotypes will not
 *	encroach.
 */
bool VisualsEcotype::isEmpty() const
{
	return (visuals_.size() == 0);
}

#if ENABLE_RELOAD_MODEL
/**
 *	This method returns flora texture used by this ecotype
 *  This function is only used for runtime reload purposes
 */
FloraTexture* VisualsEcotype::floraTexture() const
{
	return flora_->floraTexture();
}
#endif // ENABLE_RELOAD_MODEL


/**
 *	This class implements the ChooseMaxEcotype::Function interface,
 *	and provides values at any geographical location based on a
 *	2-dimensions perlin noise function.  The frequency of the noise
 *	is specified in the xml file and set during the load() method.
 */
class NoiseFunction : public ChooseMaxEcotype::Function
{
public:
	void load( DataSectionPtr pSection )
	{
		frequency_ = pSection->readFloat( "frequency", 10.f );
	}

	float operator() (const Vector2& input)
	{
		float value = noise_.sumHarmonics2( input, 0.75f, frequency_, 3.f );		
		return value;
	}

private:
	PerlinNoise	noise_;
	float		frequency_;
};


/**
 *	This class implements the ChooseMaxEcotype::Function interface,
 *	and simply provides a random value for any given geographical
 *	location.
 */
class RandomFunction : public ChooseMaxEcotype::Function
{
public:
	void load( DataSectionPtr pSection )
	{		
	}

	float operator() (const Vector2& input)
	{
		//TODO : does this break the deterministic nature of flora?
		return float(rand()) / (float)RAND_MAX;
	}

private:		
};


/**
 *	This class implements the ChooseMaxEcotype::Function interface,
 *	and provides a fixed value at any geographical location.  The
 *	value is specified in the xml file, and set during the load() method.
 */
class FixedFunction : public ChooseMaxEcotype::Function
{
public:
	void load( DataSectionPtr pSection )
	{
		value_ = pSection->readFloat( "value", 0.5f );
	}

	float operator() (const Vector2& input)
	{
		return value_;
	}

private:
	float value_;
};


/**
 *	This is the ChooseMaxEcotype destructor.
 */
ChooseMaxEcotype::~ChooseMaxEcotype()
{
	for (SubTypes::iterator it = subTypes_.begin(); it != subTypes_.end(); ++it)
	{
		delete it->first;
		bw_safe_delete(it->second);
	}
	subTypes_.clear();
}


/**
 *	This static method creates a ChooseMaxEcotype::Function object based on the
 *	settings in the provided data section.  It sources creation functions from
 *	the g_fnCreators map that contains all noise generators registered at
 *	startup.
 */
ChooseMaxEcotype::Function* ChooseMaxEcotype::createFunction( DataSectionPtr pSection )
{
	BW_GUARD;
	const BW::string& name = pSection->sectionName();	
	FunctionCreators::iterator it = g_fnCreators.find(name);
	if ( it != g_fnCreators.end() )
	{
		FunctionCreator fc = it->second;
		ChooseMaxEcotype::Function*fn = fc();
		fn->load(pSection);
		return fn;
	}
	else
	{
		ERROR_MSG( "Unknown ChooseMaxEcotype::Function %s\n", name.c_str() );
	}
	return NULL;
}


/**
 *	This method initialises the ChooseMaxEcotype generator given the datasection
 *	that is passed in.  The texture resource used by the generator is returned.
 *
 *	@param pSection	section containing generator data
 *	@param target	Ecotype to contain the texture resource used by
 *	                the generator, or empty string ("").
 *
 *  @returns True on success, false on error.
 */
bool ChooseMaxEcotype::load( DataSectionPtr pSection, Ecotype& target )
{
	BW_GUARD;
	target.textureResource( "" );
	target.pTexture( NULL );

	DataSectionIterator it = pSection->begin();
	DataSectionIterator end = pSection->end();

	while (it != end)
	{
		DataSectionPtr pFunction = *it++;
		if (pFunction->sectionName() == "editorOnly")
		{
			continue;// we don't process editorOnly data
		}
		Function* fn = ChooseMaxEcotype::createFunction(pFunction);		

		if (fn)
		{			
			BW::string oldTextureResource = target.textureResource();
			Moo::BaseTexturePtr pTexture = target.pTexture();

			//NOTE we pass in the parent section ( pFunction ) because of legacy support
			//for a simple list of <visual> sections.  If not for legacy support, we'd
			//open up the 'generator' section and pass that into the ::create method.
			EcotypeGenerator* ecotype = EcotypeGenerator::create(pFunction,target);
			if (oldTextureResource != "")
			{
				if (target.textureResource() != "" && target.textureResource() != oldTextureResource)
				{
					ERROR_MSG( "All ecotypes within a choose_max section must reference the same texture\n" );					
				}
				target.textureResource( oldTextureResource );
				target.pTexture( pTexture );
			}			

			SubType subtype;
			subtype.first = fn;
			subtype.second = ecotype;
			subTypes_.push_back( subtype );			
		}
	}	

	return true;
}


/** 
 * This method implements the transformIntoVB interface for the
 *	ChooseMaxEcotype generator.  It chooses the best ecotype
 *	for the given geographical location, and delegates the vertex
 *	creation duties to the chosen generator.
 */
uint32 ChooseMaxEcotype::generate(
		const Vector2& uvOffset,
		class FloraVertexContainer* pVerts,
		uint32 idx,
		uint32 maxVerts,
		const Matrix& objectToWorld,
		const Matrix& objectToChunk,
		BoundingBox& bb )
{
	BW_GUARD;
	Vector2 pos2( objectToWorld.applyToOrigin().x, objectToWorld.applyToOrigin().z );
	EcotypeGenerator* eg = chooseGenerator(pos2);
	if (eg)	
		return eg->generate(uvOffset, pVerts, idx, maxVerts, objectToWorld, objectToChunk, bb);	
	return 0;
}


/** 
 *	This method chooses the best ecotype generator given the current
 *	position.  It does this by asking for the probability of all
 *	generators at the given position, and returning the maximum.
 *
 *	@param pos input position to seed the generators' probability functions.
 */
EcotypeGenerator* ChooseMaxEcotype::chooseGenerator(const Vector2& pos)
{
	BW_GUARD;
	EcotypeGenerator* chosen = NULL;
	float best = -0.1f;

	SubTypes::iterator it = subTypes_.begin();
	SubTypes::iterator end = subTypes_.end();

	while (it != end)
	{
		Function& fn = *it->first;		
		float curr = fn(pos);
		if (curr>best)
		{
			best = curr;
			chosen = it->second;
		}
		it++;
	}

	return chosen;
}

/**
 *	This method returns true if the ecotype is empty.  Being empty means
 *	it will not draw any detail objects, and neighbouring ecotypes will not
 *	encroach.
 */
bool ChooseMaxEcotype::isEmpty() const
{
	return false;
}

ECOTYPE_FACTORY_DECLARE( Empty );
ECOTYPE_FACTORY_DECLARE( Visuals );
ECOTYPE_FACTORY_DECLARE( ChooseMax );

FUNCTION_FACTORY_DECLARE( Noise );
FUNCTION_FACTORY_DECLARE( Random );
FUNCTION_FACTORY_DECLARE( Fixed );

bool registerFactories()
{
	BW_GUARD;
	REGISTER_ECOTYPE_FACTORY( "empty", Empty );
	REGISTER_ECOTYPE_FACTORY( "visual", Visuals );
	REGISTER_ECOTYPE_FACTORY( "chooseMax", ChooseMax );

	REGISTER_FUNCTION_FACTORY( "noise", Noise );
	REGISTER_FUNCTION_FACTORY( "random", Random );
	REGISTER_FUNCTION_FACTORY( "fixed", Fixed );
	return true;
};
bool alwaysSuccessful = registerFactories();

BW_END_NAMESPACE

// ecotype_generator.cpp
