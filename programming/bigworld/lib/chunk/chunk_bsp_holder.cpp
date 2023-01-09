#include "pch.hpp"
#include "chunk/chunk_bsp_holder.hpp"

#if ENABLE_BSP_MODEL_RENDERING

#ifdef EDITOR_ENABLED
#include "appmgr/options.hpp"
#include "moo/render_context.hpp"
#endif

#include "cstdmf/bw_hash.hpp"
#include "cstdmf/bwrandom.hpp"
#include "cstdmf/guard.hpp"

#include <limits>

BW_BEGIN_NAMESPACE

/*static*/ const size_t ChunkBspHolder::NO_BSP_MODEL =
	std::numeric_limits< size_t >::max();

ChunkBspHolder::ChunkBspHolder()
	: colour_(),
	bspModelIndex_( ChunkBspHolder::NO_BSP_MODEL )
{
	this->getRandomBspColour( colour_ );

	BspStatics & statics = BspStatics::instance();
	if (BW_ATOMIC32_INC_AND_FETCH( &statics.chunkBspHolderCount_ ) == 1)
	{
		MF_WATCH_REF( "Render/BSP/Main Switch",
			*this,
			&ChunkBspHolder::getDrawBspAsInt,
			&ChunkBspHolder::setDrawBspAsInt,
			"Draw the collision scene."
			"0 = none, 1 = selection" );

		MF_WATCH_REF( "Render/BSP/SpeedTrees",
			*this,
			&ChunkBspHolder::getDrawBspSpeedTreesAsInt,
			&ChunkBspHolder::setDrawBspSpeedTreesAsInt,
			"Draw the collision scene."
			"0 = off, 1 = on" );

		MF_WATCH_REF( "Render/BSP/Other Models",
			*this,
			&ChunkBspHolder::getDrawBspOtherModelsAsInt,
			&ChunkBspHolder::setDrawBspOtherModelsAsInt,
			"Draw the collision scene."
			"0 = off, 1 = on" );

		MF_WATCH( "Render/BSP/type count",
			statics.bspCount_, Watcher::WT_READ_ONLY,
			"The number of different types of BSPs" );
	}
}


ChunkBspHolder::~ChunkBspHolder()
{
	BspStatics & statics = BspStatics::instance();
	if (BW_ATOMIC32_DEC_AND_FETCH( &statics.chunkBspHolderCount_ ) == 0)
	{
#if ENABLE_WATCHERS
		if (Watcher::hasRootWatcher())
		{
			Watcher::rootWatcher().removeChild( "Render/BSP/Main Switch" );
			Watcher::rootWatcher().removeChild( "Render/BSP/SpeedTrees" );
			Watcher::rootWatcher().removeChild( "Render/BSP/Other Models" );
			Watcher::rootWatcher().removeChild( "Render/BSP/type count" );
		}
#endif // ENABLE_WATCHERS
		statics.infos_.clear();
	}
}


/**
 *	Check if a BSP has been created for this item.
 *	@return true if a BSP has been created for this object.
 */
bool ChunkBspHolder::isBspCreated() const
{
	BW_GUARD;
	if (bspModelIndex_ != NO_BSP_MODEL)
	{
		return true;
	}
	return false;
}


/**
 *	Create a BSP model using the given vertices and add it to the list of BSP
 *	models.
 *	
 *	@param verts the vertices that comprise the BSP.
 */
void ChunkBspHolder::addBsp( const BW::vector< Moo::VertexXYZL >& verts,
	const BW::string& name )
{
	BW_GUARD;

	if (verts.empty())
	{
		return;
	}
	MF_ASSERT( (verts.size() % 3) == 0 );

	bspModelIndex_ = this->getBspIndex( name );
	MF_ASSERT( bspModelIndex_ != NO_BSP_MODEL );

	BspStatics & statics = BspStatics::instance();
	SimpleMutexHolder smh( statics.infoMutex_ );

	// Check if there already is a BSP model for this type of object
	if (statics.infos_.find( bspModelIndex_ ) != statics.infos_.end())
	{
		return;
	}

	// Otherwise, make a new one
	const size_t bufferSize = verts.size() * sizeof ( Moo::VertexXYZL );
	const UINT uintBufferSize = static_cast< UINT >( bufferSize );

	Info info;

	info.primitiveCount_ = static_cast< UINT >( verts.size() / 3 );
	info.vb_.create( uintBufferSize,
		D3DUSAGE_WRITEONLY,
		Moo::VertexXYZL::fvf(),
		D3DPOOL_MANAGED );
	Moo::VertexLock< Moo::VertexXYZL >( info.vb_ ).fill( &verts[ 0 ],
		uintBufferSize );

	statics.infos_[ bspModelIndex_ ] = info;
	statics.bspCount_ = statics.infos_.size();
}


/**
 *	Delete a BSP from the list.
 */
void ChunkBspHolder::delBsp()
{
	BW_GUARD;

	if (bspModelIndex_ == NO_BSP_MODEL)
	{
		return;
	}

	BspStatics & statics = BspStatics::instance();
	SimpleMutexHolder smh( statics.infoMutex_ );
	Infos::iterator it = statics.infos_.find( bspModelIndex_ );
	if ( it != statics.infos_.end() )
	{
		statics.infos_.erase( it );
		statics.bspCount_ = statics.infos_.size();
	}

	bspModelIndex_ = NO_BSP_MODEL;
}


/**
 *	Setup the render states for drawing.
 *	@param rc the current render context.
 *	@param renderSelection render item as selected.
 */
void ChunkBspHolder::setRenderStates( Moo::RenderContext& rc,
	bool renderSelection )
{
	BW_GUARD;

	rc.device()->SetTransform( D3DTS_VIEW, &rc.view() );
	rc.device()->SetTransform( D3DTS_PROJECTION, &rc.projection() );

	rc.setPixelShader( NULL );
	rc.setVertexShader( NULL );
	rc.setFVF( Moo::VertexXYZL::fvf() );
	rc.setRenderState( D3DRS_ALPHATESTENABLE, FALSE );
	rc.setRenderState( D3DRS_ALPHABLENDENABLE, FALSE );
	rc.setRenderState( D3DRS_LIGHTING, FALSE );
	rc.setRenderState( D3DRS_ZWRITEENABLE, TRUE );
	rc.setRenderState( D3DRS_ZENABLE, D3DZB_TRUE );
	rc.setRenderState( D3DRS_ZFUNC, D3DCMP_LESSEQUAL );

	if (renderSelection)
	{
		rc.setTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_SELECTARG1 );
		rc.setTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TFACTOR );
		rc.setTextureStageState( 0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1 );
		rc.setTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_TFACTOR );
	}
	else
	{
		rc.setTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_SELECTARG1 );
		rc.setTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_DIFFUSE );
		rc.setTextureStageState( 0, D3DTSS_ALPHAOP, D3DTOP_DISABLE );
	}

	rc.setTextureStageState( 1, D3DTSS_ALPHAOP, D3DTOP_DISABLE );
	rc.setTextureStageState( 1, D3DTSS_COLOROP, D3DTOP_DISABLE );
}


/**
 *	Draw the BSP for a given item using a default colour.
 *	@param rc the current render context.
 *	@param world the world transform of the object.
 *	@param renderSelection render item as selected.
 *	@param setStates setup the render states first.
 */
void ChunkBspHolder::drawBsp( Moo::RenderContext& rc,
	const Matrix& world,
	bool renderSelection,
	bool setStates ) const
{
	BW_GUARD;
	ChunkBspHolder::drawColourBsp(
		rc, bspModelIndex_, world, colour_, renderSelection, setStates );
}


/**
 *	Put the given BSP into a list to be drawn later.
 *	Can only be done on the main thread.
 *	@param world the world transform of the object.
 */
void ChunkBspHolder::batchBsp( const Matrix& world ) const
{
	BW_GUARD;
	MF_ASSERT( bspModelIndex_ != NO_BSP_MODEL );
	MF_ASSERT( MainThreadTracker::isCurrentThreadMain() );

	BspPrimitive bp = { bspModelIndex_, world, colour_ };
	BspStatics::instance().bspPrimitives_.push_back( bp );
}


/**
 *	Draw the given BSP.
 *	@param rc the current render context.
 *	@param world the world transform of the object.
 *	@param colour the colour of the BSP.
 *	@param renderSelection render item as selected.
 *	@param setStates setup the render states first.
 */
void ChunkBspHolder::drawColourBsp( Moo::RenderContext& rc,
	const size_t& index,
	const Matrix& world,
	const Moo::Colour& colour,
	bool renderSelection,
	bool setStates )
{
	BW_GUARD;
	MF_ASSERT( index != NO_BSP_MODEL );

	static DogWatch dw( "ChunkBspHolder_drawColourBsp" );
	ScopedDogWatch sdw( dw );

	if (setStates)
	{
		ChunkBspHolder::setRenderStates( rc, renderSelection );
	}

	BspStatics & statics = BspStatics::instance();
	SimpleMutexHolder smh( statics.infoMutex_ );
	Infos::const_iterator iter = statics.infos_.find( index );
	MF_ASSERT( iter != statics.infos_.end() );

	rc.device()->SetTransform( D3DTS_WORLD, &world );
	iter->second.vb_.set( 0, 0, sizeof(Moo::VertexXYZL) );

	rc.drawPrimitive( D3DPT_TRIANGLELIST, 0,
		iter->second.primitiveCount_ );
}


/**
 *	Draw all of the BSPs that have been batched.
 *	@param rc the current render context.
 */
void ChunkBspHolder::drawBsps( Moo::RenderContext& rc )
{
	BW_GUARD;
	static DogWatch dw( "ChunkBspHolder_drawBsps" );
	ScopedDogWatch sdw( dw );

	ChunkBspHolder::setRenderStates( rc, false );

	BspStatics & statics = BspStatics::instance();
	for (BspPrimitives::const_iterator bp = statics.bspPrimitives_.begin();
		bp != statics.bspPrimitives_.end();
		++bp)
	{
		ChunkBspHolder::drawColourBsp(
			rc, bp->index_, bp->world_, bp->colour_, false, false );
	}

	statics.bspPrimitives_.clear();
}


/**
 *	Check if we are drawing BSPs.
 *	@return if we are drawing BSPs, false = none, true = selection.
 */
/*static*/const bool ChunkBspHolder::getDrawBsp()
{
	BW_GUARD;
	BspStatics & statics = BspStatics::instance();
#ifdef EDITOR_ENABLED
	readSettings( statics );
#endif
	return statics.drawBsp_;
}


/**
 *	Gets the the value of the "drawBSP" option as an integer.
 *	@return if we are drawing BSPs, 0 = none, 1 = selection.
 */
const int& ChunkBspHolder::getDrawBspAsInt() const
{
	BW_GUARD;
	BspStatics & statics = BspStatics::instance();
	statics.drawBspInt_ = ChunkBspHolder::getDrawBsp();
	return statics.drawBspInt_;
}


/**
 *	Sets the the value of the "drawBSP" option as an integer.
 *	@param value if we are drawing BSPs, 0 = none, 1 = some, 2 = all.
 */
void ChunkBspHolder::setDrawBspAsInt( const int& value )
{
	BW_GUARD;
	BspStatics & statics = BspStatics::instance();
#ifdef EDITOR_ENABLED
	Options::setOptionInt( "render/drawBSP", value );
#endif
	statics.drawBspInt_ = value;
	statics.drawBsp_ = value != 0;
}


/**
 *	Check if we are drawing BSPs for SpeedTree models.
 *	@return if we are drawing BSPs.
 */
/*static*/const bool ChunkBspHolder::getDrawBspSpeedTrees()
{
	BW_GUARD;
	BspStatics & statics = BspStatics::instance();
#ifdef EDITOR_ENABLED
	readSettings( statics );
#endif
	return statics.drawBspSpeedTrees_;
}


/**
 *	Gets the the value of the "drawBSPSpeedTrees" option as an integer.
 *	@return if we are drawing BSPs, 0 = no, 1 = yes.
 */
const int& ChunkBspHolder::getDrawBspSpeedTreesAsInt() const
{
	BW_GUARD;
	BspStatics & statics = BspStatics::instance();
	statics.drawBspSpeedTreesInt_ = ChunkBspHolder::getDrawBspSpeedTrees();
	return statics.drawBspSpeedTreesInt_;
}


/**
 *	Sets the the value of the "drawBSPSpeedTrees" option as an integer.
 *	@param value if we are drawing BSPs, 0 = no, 1 = yes.
 */
void ChunkBspHolder::setDrawBspSpeedTreesAsInt( const int& value )
{
	BW_GUARD;
	BspStatics & statics = BspStatics::instance();
#ifdef EDITOR_ENABLED
	Options::setOptionInt( "render/drawBSP/SpeedTrees", value );
#endif
	statics.drawBspSpeedTreesInt_ = value;
	statics.drawBspSpeedTrees_ = value != 0;
}


/**
 *	Check if we are drawing BSPs for other models.
 *	@return if we are drawing BSPs, 0 = none, 1 = selection.
 */
/*static*/const bool ChunkBspHolder::getDrawBspOtherModels()
{
	BW_GUARD;
	BspStatics & statics = BspStatics::instance();
#ifdef EDITOR_ENABLED
	readSettings( statics );
#endif
	return statics.drawBspOtherModels_;
}


/**
 *	Gets the the value of the "drawBSPOtherModels" option as an integer.
 *	@return if we are drawing BSPs, 0 = no, 1 = yes.
 */
const int& ChunkBspHolder::getDrawBspOtherModelsAsInt() const
{
	BW_GUARD;
	BspStatics & statics = BspStatics::instance();
	statics.drawBspOtherModelsInt_ = ChunkBspHolder::getDrawBspOtherModels();
	return statics.drawBspOtherModelsInt_;
}


/**
 *	Sets the the value of the "drawBSPOtherModels" option as an integer.
 *	@param value if we are drawing BSPs, 0 = no, 1 = yes.
 */
void ChunkBspHolder::setDrawBspOtherModelsAsInt( const int& value )
{
	BW_GUARD;
	BspStatics & statics = BspStatics::instance();
#ifdef EDITOR_ENABLED
	Options::setOptionInt( "render/drawBSP/OtherModels", value );
#endif
	statics.drawBspOtherModelsInt_ = value;
	statics.drawBspOtherModels_ = value != 0;
}


/**
 *	Get the index for a given model's BSP.
 *	
 */
size_t ChunkBspHolder::getBspIndex( const BW::string& name )
{
	BW_GUARD;
	MF_ASSERT( !name.empty() );
	return BW::hash_string( name.c_str() );
}


/**
 *	Get a random colour for colouring BSPs.
 *	@param colour set this colour.
 */
void ChunkBspHolder::getRandomBspColour( Moo::Colour& colour ) const
{
	BW_GUARD;
	BWRandom& random = BWRandom::instance();
	colour = Moo::Colour( random( 0.0f, 1.0f ),
		random( 0.0f, 1.0f ),
		random( 0.0f, 1.0f ),
		1.0f );
}


#ifdef EDITOR_ENABLED
/**
 *	Change the colour of cloned objects' BSP models.
 */
void ChunkBspHolder::postClone()
{
	BW_GUARD;

	this->getRandomBspColour( colour_ );
}


/**
 *	Updates the Draw BSP flags from the Options.
 */
/*static*/void ChunkBspHolder::readSettings( BspStatics& statics )
{
	Moo::RenderContext& rc = Moo::rc();

	if (rc.frameTimestamp() != statics.settingsMark_)
	{
		statics.drawBsp_ =
			Options::getOptionInt( "render/drawBSP", 0 ) != 0;
		statics.drawBspSpeedTrees_ =
			Options::getOptionInt( "render/drawBSP/SpeedTrees", 0 ) != 0;
		statics.drawBspOtherModels_ =
			Options::getOptionInt( "render/drawBSP/OtherModels", 0 ) != 0;
		statics.settingsMark_ = rc.frameTimestamp();
	}
}
#endif // EDITOR_ENABLED

BW_END_NAMESPACE

#endif // ENABLE_BSP_MODEL_RENDERING
