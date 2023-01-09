#ifndef _WORLDEDITOR_WORLD_ITEMS_EDITORCHUNKSUBSTANCE_IPP_
#define _WORLDEDITOR_WORLD_ITEMS_EDITORCHUNKSUBSTANCE_IPP_

// editor_chunk_substance.ipp

#include "worldeditor/world/editor_chunk_cache.hpp"

BW_BEGIN_NAMESPACE

/**
 *	@file This file contains the implementations of the template methods
 *	in EditorChunkSubstance
 */
template <class CI>
EditorChunkSubstance<CI>::EditorChunkSubstance() :
	pReprSuperModel_( NULL )
{

}


template <class CI>
EditorChunkSubstance<CI>::~EditorChunkSubstance()
{
	bw_safe_delete( pReprSuperModel_ );
}


/**
 *	Load method
 */
template <class CI> bool EditorChunkSubstance<CI>::load( DataSectionPtr pSect )
{
	BW_GUARD;

	pOwnSect_ = pSect;
	edCommonLoad( pSect );
	const bool success = this->CI::load( pSect );

	this->updateReprModel();

	return success;
}


/**
 *	Load method for items whose load method has a chunk pointer
 */
template <class CI> bool EditorChunkSubstance<CI>::load(
	DataSectionPtr pSect, Chunk * pChunk )
{
	BW_GUARD;

	pOwnSect_ = pSect;
	edCommonLoad( pSect );
	const bool success = this->CI::load( pSect, pChunk );

	this->updateReprModel();

	return success;
}


template <class CI>
void EditorChunkSubstance<CI>::updateReprModel()
{
	BW_GUARD;

	bw_safe_delete( pReprSuperModel_ );
	ModelPtr model = this->reprModel();
	if (model)
	{
		BW::vector< BW::string > modelIDs;
		modelIDs.push_back( model->resourceID() );
		pReprSuperModel_ = new SuperModel( modelIDs );
	}

	return;
}


/**
 *	This method does extra stuff when this item is tossed between chunks.
 *
 *	It updates its datasection in that chunk.
 */
template <class CI> void EditorChunkSubstance<CI>::toss( Chunk * pChunk )
{
	BW_GUARD;

	if (pChunk_ != NULL)
	{
		ChunkModelObstacle::instance( *pChunk_ ).delObstacles( this );

		if (pOwnSect_)
		{
			EditorChunkCache::instance( *pChunk_ ).
				pChunkSection()->delChild( pOwnSect_ );
			pOwnSect_ = NULL;
		}
	}

	this->CI::toss( pChunk );

	if (pChunk_ != NULL)
	{
		if (!pOwnSect_)
		{
			pOwnSect_ = EditorChunkCache::instance( *pChunk_ ).
				pChunkSection()->newSection( this->sectName() );
			this->edSave( pOwnSect_ );
		}

		this->addAsObstacle();
	}

	if (this->autoAddToSceneBrowser())
	{
		// Update the database
		ItemInfoDB::instance().toss( this, pChunk != NULL );
	}
}

/**
 * If we want to draw or not
 */
template <class CI> bool EditorChunkSubstance<CI>::edShouldDraw()
{
	BW_GUARD;

	if( !CI::edShouldDraw() )
		return false;

	return this->isDrawFlagVisible();
}


/**
 *	Check if this substance needs its animations updated this frame.
 *	@return true if this substance needs its animations updated,
 *		false if it does not.
 */
template <class CI>
void EditorChunkSubstance<CI>::updateAnimations()
{
	BW_GUARD;

	this->CI::updateAnimations();

	if (pReprSuperModel_ != NULL)
	{
		Matrix world( CI::chunk()->transform() );
		world.preMultiply( this->edTransform() );

		pReprSuperModel_->updateAnimations( world,
			NULL, // pPreFashions
			NULL, // pPostFashions
			Model::LOD_AUTO_CALCULATE );
	}
}


/**
 *	Draw method
 */
template <class CI> void EditorChunkSubstance<CI>::draw( Moo::DrawContext& drawContext )
{
	BW_GUARD;

	this->CI::draw( drawContext );
	
	if (!edShouldDraw())
		return;

	Moo::rc().push();
	Moo::rc().preMultiply( this->edTransform() );

	Model::incrementBlendCookie();

	if (WorldManager::instance().drawSelection())
	{
		WorldManager::instance().registerDrawSelectionItem(
													(ChunkItem*) this );
	}

	bool drawFrozen = this->edFrozen() && OptionsMisc::frozenVisible();
	
	if (!drawFrozen || !WorldManager::instance().drawSelection())
	{
		bool projectModule = ProjectModule::currentInstance() == ModuleManager::instance().currentModule();
		if( !projectModule )
		{
			ModelPtr pModel = reprModel();
			if (pModel.exists())
			{
				BoundingBox bbox;
				Matrix boxTransform;
				if (drawFrozen)
				{
					// display substance as frozen
					bbox = pModel->visibilityBox();
					boxTransform = pChunk_ ? pChunk_->transform() : Matrix::identity;
					boxTransform.preMultiply( this->edTransform() );
					WorldManager::instance().showFrozenRegion( bbox, boxTransform );
				}
			}
		}

		if (pReprSuperModel_ != NULL)
		{
			pReprSuperModel_->draw( drawContext, Moo::rc().world(), NULL );
		}
		else
		{
			// If any reprModel exists, then it should be in
			// the repr super model
			MF_ASSERT( !this->reprModel() );
		}

	}

	Moo::rc().pop();
}


/**
 *	Get the bounding box for this substance
 */
template <class LT> void EditorChunkSubstance<LT>::edBounds(
	BoundingBox& bbRet ) const
{
	BW_GUARD;

	ModelPtr model = this->reprModel();
	// Get the size of the representative model
	if (model)
		bbRet = model->boundingBox();
	// If there is not representative model then we return an arbitrary box
	else
		bbRet = BoundingBox(Vector3(0.0f, 0.0f, 0.0f), Vector3(0.1f, 0.1f, 0.1f));
}


/**
 *	Add this itme's representation model as an obstacle. Normally won't
 *	need to be overridden, but some items have special requirements.
 */
template <class LT> void EditorChunkSubstance<LT>::addAsObstacle()
{
	BW_GUARD;

	Matrix world( pChunk_->transform() );
	world.preMultiply( this->edTransform() );

	ModelPtr model = this->reprModel();
	if (model)
	{
		ChunkModelObstacle::instance( *pChunk_ ).addModel(
			model, world, this );
	}
}


/**
 *	Reload this item and report on success
 */
template <class LT> bool EditorChunkSubstance<LT>::reload()
{
	BW_GUARD;

	return this->load( pOwnSect_ );
}


template <class LT> bool EditorChunkSubstance<LT>::addYBounds( BoundingBox& bb ) const
{
	BW_GUARD;

	// sorry for the conversion
	bb.addYBounds( ( ( EditorChunkSubstance<LT>* ) this)->edTransform().applyToOrigin().y );
	return true;
}

BW_END_NAMESPACE

// editor_chunk_substance.ipp

#endif // _WORLDEDITOR_WORLD_ITEMS_EDITORCHUNKSUBSTANCE_IPP_