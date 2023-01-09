#include "pch.hpp"
#include "chunk_deferred_decal.hpp"
#include "moo/texture_manager.hpp"
#include "geometry_mapping.hpp"
#include "chunk_manager.hpp"

#if UMBRA_ENABLE
#include "chunk_umbra.hpp"
#include "umbra_chunk_item.hpp"
#include "ob/umbraObject.hpp"
#endif

BW_BEGIN_NAMESPACE

//-- Note: Another example of the BigWorld's macro magic.
#undef IMPLEMENT_CHUNK_ITEM_ARGS
#define IMPLEMENT_CHUNK_ITEM_ARGS (pSection, pChunk)
IMPLEMENT_CHUNK_ITEM(StaticChunkDecal, staticDecal, 0)

//-- Note: To explain to the very smart linker that we want to keep this code in the final exe.
int ChunkDeferredDecal_token = 0;

//--------------------------------------------------------------------------------------------------
StaticChunkDecal::StaticChunkDecal()
	:	ChunkItem(WANTS_DRAW), m_decal(-1)
{

}

//--------------------------------------------------------------------------------------------------
StaticChunkDecal::~StaticChunkDecal()
{
	if (m_decal != -1)
	{
		DecalsManager::instance().removeStaticDecal(m_decal);
		m_decal = -1;
	}
}

//--------------------------------------------------------------------------------------------------
bool StaticChunkDecal::load(DataSectionPtr iData)
{
	BW_GUARD;	

	//-- shortcut.
	typedef DecalsManager::Decal Decals;
	
	Matrix defaultMatrix;
	defaultMatrix.setRotateX( MATH_PI / 2.f );

	//-- read properties from the decal section and fill decal Decal::Desc structure.
	m_decalDesc.m_transform		= iData->readMatrix34("transform", defaultMatrix);
	m_decalDesc.m_map1			= iData->readString("diffTex", "");
	m_decalDesc.m_map2			= iData->readString("bumpTex", "");
	m_decalDesc.m_priority		= iData->readUInt("priority", 0);
	m_decalDesc.m_influenceType = iData->readUInt("influence", Decals::APPLY_TO_TERRAIN);

	unsigned int defaultMaterialType = m_decalDesc.m_map2 == "" ?
		Decals::MATERIAL_DIFFUSE : Decals::MATERIAL_BUMP;
	m_decalDesc.m_materialType	=
		Decals::EMaterialType( iData->readUInt( "type", defaultMaterialType ) );

	//-- try to create.
	m_decal = DecalsManager::instance().createStaticDecal(m_decalDesc);
	if (m_decal == -1)
	{
		return false;
	}

	return true;
}

//--------------------------------------------------------------------------------------------------
bool StaticChunkDecal::load(DataSectionPtr iData, Chunk* /*pChunk*/)
{
	BW_GUARD;	
	
	return load(iData);
}

//-- add ourselves to or remove ourselves from the given chunk.
//--------------------------------------------------------------------------------------------------
void StaticChunkDecal::toss(Chunk* newChunk)
{
	BW_GUARD;
	ChunkItem::toss(newChunk);

	if (pChunk_ != NULL)
	{
		//-- update decal.
		update(m_decalDesc.m_transform);
	}
}

//--------------------------------------------------------------------------------------------------
void StaticChunkDecal::update(const Matrix& mat)
{
	BW_GUARD;
	
	//-- save new local for current chunk transform.
	m_decalDesc.m_transform = mat;

	//-- reconstruct world transform.
	Matrix transform = mat;
	transform.postMultiply(pChunk_->transform());

	//-- calculate world bounds.
	{
		m_worldBounds.setBounds(Vector3(-0.5f, -0.5f, -0.5f), Vector3(+0.5f, +0.5f, +0.5f));
		m_worldBounds.transformBy(transform);
	}

	//-- notify decals manager.
	if (isLoaded())
	{
		DecalsManager::instance().updateStaticDecal(m_decal, transform);
	}
}

//--------------------------------------------------------------------------------------------------
void StaticChunkDecal::syncInit()
{
	BW_GUARD;

	//-- ToDo (b_sviglo): Overhead of an additional UMBRA occlusion culling may be very big. Reconsider.
#if UMBRA_ENABLE
	delete pUmbraDrawItem_;
	BoundingBox bb(Vector3(-0.5f, -0.5f, -0.5f), Vector3(+0.5f, +0.5f, +0.5f));

	// Set up object transforms
	Matrix transform = m_decalDesc.m_transform;
	transform.postMultiply(pChunk_->transform());

	UmbraChunkItem* pUmbraChunkItem = new UmbraChunkItem;
	pUmbraChunkItem->init(this, bb, transform, pChunk_->getUmbraCell());
	pUmbraDrawItem_ = pUmbraChunkItem;
	Umbra::OB::Object& uObj = *pUmbraDrawItem_->pUmbraObject()->object();
	uObj.setRenderCost(Umbra::OB::Object::CHEAP);
	//-- ToDo (b_sviglo): reconsider distance.
	uObj.setCullDistance(0, 400.0f);
	updateUmbraLenders();
#endif
}

//--------------------------------------------------------------------------------------------------
void StaticChunkDecal::draw( Moo::DrawContext& )
{
	BW_GUARD;

	//-- add to manager for drawing only visible decals.
	//-- Note: When umbra enabled we don't need to to this camera vs. bb test because umbra does it
	//--       for us in the very effective way.
#if UMBRA_ENABLE
	if (ChunkManager::instance().umbra()->umbraEnabled())
	{
		DecalsManager::instance().markVisible(m_decal);
	}
	else
#endif //-- UMBRA_ENABLE
	{
		m_worldBounds.calculateOutcode(Moo::rc().viewProjection());
		if (!m_worldBounds.combinedOutcode())
		{
			DecalsManager::instance().markVisible(m_decal);
		}
	}
}

BW_END_NAMESPACE
