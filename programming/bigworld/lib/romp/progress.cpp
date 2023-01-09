
#include "pch.hpp"

#include "progress.hpp"
#include "resmgr/bwresource.hpp"
#include "romp/font_manager.hpp"
#include "romp/font_metrics.hpp"
#include "cstdmf/debug.hpp"
#include "cstdmf/timestamp.hpp"
#include "moo/render_context.hpp"
#include "moo/material.hpp"

DECLARE_DEBUG_COMPONENT2( "Romp", 0 )


BW_BEGIN_NAMESPACE

/// Constructor. Note it is safe to provide a NULL owner
ProgressTask::ProgressTask( ProgressDisplay *pOwner, const BW::string & name,
		float length ) :
	pOwner_( pOwner ),
	done_( 0.f ),
	length_( length ),
	isCancelled_( false )
{
	BW_GUARD;
	if (pOwner_ != NULL) 
	{
		pOwner_->add( *this, name );
	}
}

ProgressTask::~ProgressTask()
{
	BW_GUARD;
	if (pOwner_ != NULL) 
	{
		pOwner_->del( *this );
	}
}


bool ProgressTask::step( float progress )
{
	BW_GUARD;
	done_ += progress;

	if (pOwner_ != NULL) 
	{
		return pOwner_->onProgress();
	}
	return true;
}


bool ProgressTask::set( float done )
{
	BW_GUARD;
	done_ = done;

	if (pOwner_ != NULL) 
	{
		return pOwner_->onProgress();
	}
	return true;
}

bool ProgressTask::isCancelled()
{
	return isCancelled_;
}

void ProgressTask::registerCallback( ProgressTaskCB* pCallback )
{
	BW_GUARD;
	bool found = false;
	MF_ASSERT( pCallback != NULL );
	for (uint i = 0; i < callbacks_.size(); i++)
	{
		if(callbacks_[i] == pCallback)
		{
			found = true;
		}
	}
	MF_ASSERT( found == false );
	callbacks_.push_back( pCallback );
}

void ProgressTask::onCancel()
{
	BW_GUARD;
	isCancelled_ = true;
	for (uint i = 0; i < callbacks_.size(); i++)
	{
		callbacks_[i]->onCancelProgress();
	}
}



ProgressDisplay::ProgressDisplay( FontPtr pFont, ProgressCallback pCallback, uint32 colour ):
	pFont_( pFont ),
	pCallback_( pCallback ),
	colour_( colour ),
	deepestNode_( -1 ),
	lastDrawn_( 0 ),
	minRedrawTime_( stampsPerSecond()/20 ),
	rowHeight_( 0.05f )
{
	BW_GUARD;
	const Vector2 screenDim(
		float( Moo::rc().screenWidth() ), float( Moo::rc().screenHeight() ) );

	if ( !pFont_ )
	{
		float	fsize = screenDim[0] / 92.f;

		pFont_ = FontManager::instance().get( "default_medium.font" );
	}

	if ( pFont_ )
	{
		pFont_->colour( colour_ );
		rowHeight_ = pFont_->metrics().clipHeight();
	}

	rowSep_ = rowHeight_ / 5;
}


ProgressDisplay::~ProgressDisplay()
{
	BW_GUARD;
	for (uint i=0; i < tasks_.size(); i++)
	{
		if (tasks_[i].task != NULL)
		{
			tasks_[i].task->detach();
			tasks_[i].task = NULL;
		}
	}
}


/**
 *	Method to add a static message string
 */
void ProgressDisplay::add( const BW::string & str )
{
	BW_GUARD;
	messages_.push_back( str );
}


/**
 *	Appends string to the last line in the progress display
 */
void ProgressDisplay::append( const BW::string & str )
{
	BW_GUARD;
	if( !messages_.empty() )
		messages_.back().append( str );
	else
		messages_.push_back( str );
}


/**
 *	Method to display ourselves
 */
bool ProgressDisplay::draw( bool force )
{
	BW_GUARD;
#ifndef EDITOR_ENABLED
	// make sure we're not drawing again too soon
	if (!force && (timestamp() - lastDrawn_ < minRedrawTime_))
	{
		return true;
	}
#endif
	lastDrawn_ = timestamp();

	// run the callback
	bool ret = true;
	if (pCallback_)
		ret = (*pCallback_)();

	if ( !ret || !Moo::rc().checkDevice())
		return false;

	// just draw the scene
	Moo::rc().device()->Clear( 0, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER, 0x00000000, 1, 0 );
	Moo::rc().beginScene();
	if (Moo::rc().mixedVertexProcessing())
		Moo::rc().device()->SetSoftwareVertexProcessing( TRUE );
	Moo::rc().nextFrame();
	if (Moo::rc().mixedVertexProcessing())
		Moo::rc().device()->SetSoftwareVertexProcessing(TRUE);

	int row = 0;
	int col = 0;

	if ( pFont_ && FontManager::instance().begin(*pFont_) )
	{
		// print static messages
		for (uint i=0; i<messages_.size(); i++)
		{
			pFont_->drawConsoleString( messages_[i], col, row );
			row++;
		}
		FontManager::instance().end();
	}

	// now traverse the tree
	BW::vector<int>	ixStack = roots_;

	while( ixStack.size() > 0)
	{
		int i = ixStack.back();
		ixStack.pop_back();

		ProgressNode & node = tasks_[i];

		// draw the name
		if ( pFont_ && FontManager::instance().begin(*pFont_) )
		{
			pFont_->drawConsoleString(node.name, col + node.level, row );
			FontManager::instance().end();
		}

		// draw the bar
		if (node.task != NULL)
		{
			float usedone = node.task->done_;

			if (usedone < 0.f) usedone = 0.f;

			float uselen = node.task->length_;

			if ( uselen == 0.f ) uselen = 1000000.f;
			if ( uselen > 0.f && uselen < usedone ) uselen = usedone;

			this->drawBar( row, usedone / uselen );
		}
		else
		{
			this->drawBar( row, 2.f );
		}

		// now push on all our children in reverse order
		const int numChildren = static_cast<int>( node.children.size() );
		for (int j = numChildren-1; j >= 0; j--)
		{
			ixStack.push_back( node.children[j] );
		}

		row++;
	}	

	Moo::rc().endScene();
	Moo::rc().present();

	return true;
}


/**
 *	Private method to draw a progress bar
 */
void ProgressDisplay::drawBar( int row, float fraction )
{
	BW_GUARD;
	const float halfWidth = Moo::rc().halfScreenWidth();
	const float halfHeight = Moo::rc().halfScreenHeight();

	static Moo::Material mat;
	static bool firstTime = true;

	if (firstTime)
	{
		Moo::TextureStage	ts;
		ts.colourOperation( Moo::TextureStage::SELECTARG2,
			Moo::TextureStage::TEXTURE,	Moo::TextureStage::DIFFUSE );
		ts.alphaOperation( Moo::TextureStage::DISABLE,
			Moo::TextureStage::TEXTURE, Moo::TextureStage::DIFFUSE );
		mat.addTextureStage( ts );

		ts.colourOperation( Moo::TextureStage::DISABLE,
			Moo::TextureStage::TEXTURE, Moo::TextureStage::DIFFUSE );
		mat.addTextureStage( ts );
		mat.srcBlend( Moo::Material::SRC_ALPHA );
		mat.destBlend( Moo::Material::INV_SRC_ALPHA );
		mat.alphaBlended( true );
		mat.zBufferRead( false );
		mat.zBufferWrite( false );
		mat.fogged( false );

		firstTime = false;
	}

	typedef Moo::VertexTL VertexType;

	uint16 polys[ 6+8 ];
	VertexType verts[ 8 ];


	uint32	fillAlpha = 0x88000000;
	uint32	frameAlpha = 0xFF000000;

	if (fraction < 0.f)
	{
		uint8	alpha = uint8( (-fraction)*32.f );
		if (alpha > 127) alpha = -alpha;

		fillAlpha = uint32(alpha) << 24;
		fraction = 1.f;
	}
	else if (fraction > 1.5f)
	{
		fillAlpha = fillAlpha/4*3;
		frameAlpha = frameAlpha/4*3;
		fraction = 1.f;
	}

	for (uint i=0; i < 8; i++)
	{
		verts[i].pos_.z = 0;
		verts[i].pos_.w = 1;
		verts[i].colour_ = ((i<4) ? fillAlpha : frameAlpha) | ( colour_ & 0x00ffffff );
	}

	for (uint i=0; i < 8; i+=4)
	{
		verts[ 0+i ].pos_.x = -0.01f * halfWidth + halfWidth;
		verts[ 0+i ].pos_.y = - (1.f - rowHeight_*row) * halfHeight + halfHeight;

		verts[ 1+i ].pos_.x = (1.f*(i?1:fraction) - 0.01f) * halfWidth + halfWidth;
		verts[ 1+i ].pos_.y = -(1.f - rowHeight_*row) * halfHeight + halfHeight;

		verts[ 2+i ].pos_.x = -0.01f * halfWidth + halfWidth;
		verts[ 2+i ].pos_.y = -(1.f - rowHeight_*(row+1) + rowSep_) * halfHeight + halfHeight;

		verts[ 3+i ].pos_.x = (1.f*(i?1:fraction) - 0.01f) * halfWidth + halfWidth;
		verts[ 3+i ].pos_.y = -(1.f - rowHeight_*(row+1) + rowSep_) * halfHeight + halfHeight;
	}

	//setMaterial( &mat );
	Moo::rc().setRenderState( D3DRS_ALPHATESTENABLE, FALSE );
	Moo::rc().setRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
	Moo::rc().setRenderState( D3DRS_SRCBLEND, D3DBLEND_SRCALPHA );
	Moo::rc().setRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );

	Moo::rc().setTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_SELECTARG2 );
	Moo::rc().setTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
	Moo::rc().setTextureStageState( 0, D3DTSS_ALPHAOP, D3DTOP_DISABLE );
	Moo::rc().setTextureStageState( 1, D3DTSS_ALPHAOP, D3DTOP_DISABLE );
	Moo::rc().setTextureStageState( 1, D3DTSS_COLOROP, D3DTOP_DISABLE );

	Moo::rc().setVertexShader( NULL );
	Moo::rc().setFVF( VertexType::fvf() );


	// triangles
	polys[ 0 ] = 0;
	polys[ 1 ] = 1;
	polys[ 2 ] = 2;
	polys[ 3 ] = 1;
	polys[ 4 ] = 3;
	polys[ 5 ] = 2;

	// wires
	polys[ 6 ] = 0;
	polys[ 7 ] = 1;
	polys[ 8 ] = 1;
	polys[ 9 ] = 3;
	polys[ 10 ] = 3;
	polys[ 11 ] = 2;
	polys[ 12 ] = 2;
	polys[ 13 ] = 0;


	//DynamicVertexBuffer	
	int vertexSize = sizeof(VertexType);
	Moo::DynamicVertexBufferBase2& vb = 
		Moo::DynamicVertexBufferBase2::instance( vertexSize );
	uint32 lockIndex = 0;
	if ( vb.lockAndLoad( &verts[0], 8, lockIndex ) &&
		 SUCCEEDED(vb.set( 0 )) )
	{
		Moo::rc().drawPrimitive( D3DPT_TRIANGLESTRIP, lockIndex, 2 );

		//TODO: this looks pretty static....
		//DynamicIndexBuffer
		Moo::DynamicIndexBufferBase& dynamicIndexBuffer = Moo::rc().dynamicIndexBufferInterface().get( D3DFMT_INDEX16 );
		Moo::IndicesReference ind = dynamicIndexBuffer.lock( 14 );
		if ( ind.valid() )
		{
			ind.fill( &polys[0], 14 );
			dynamicIndexBuffer.unlock();
			if ( SUCCEEDED(dynamicIndexBuffer.indexBuffer().set()) )
			{
				uint32 startIndex = dynamicIndexBuffer.lockIndex();
				Moo::rc().drawIndexedPrimitive( D3DPT_LINELIST, lockIndex + 4, 0, 4, startIndex + 6, 4 );
			}
		}
	}
}



/**
 *	Private method to add a progress task to our list
 */
void ProgressDisplay::add( ProgressTask & task, const BW::string & name )
{
	BW_GUARD;
	ProgressNode	newNode;
	newNode.task = &task;
	newNode.name = name;

	int	newIdx = static_cast<int>( tasks_.size() );

	if (deepestNode_ != -1)
	{
		newNode.level = tasks_[deepestNode_].level + 1;
		tasks_[deepestNode_].children.push_back( newIdx );
	}
	else
	{
		newNode.level = 0;
		roots_.insert( roots_.begin(), newIdx );
	}

	tasks_.push_back( newNode );
	deepestNode_ = newIdx;

	this->draw( true );
}


/**
 *	Private method to remove a progress task from our list
 */
void ProgressDisplay::del( ProgressTask & task )
{
	BW_GUARD;
	for (uint i=0; i<tasks_.size(); i++)
	{
		if (tasks_[i].task == &task )
		{
			tasks_[i].task = NULL;

			// now correct deepestNode
			deepestNode_ = -1;
			int	depth = -1;
			for (uint j=0; j<tasks_.size(); j++)
			{
				if (tasks_[j].task != NULL &&
					tasks_[j].level >= depth )
				{
					deepestNode_ = j;
					depth = tasks_[j].level;
				}
			}
			break;
		}
	}
}

void ProgressDisplay::onCancel()
{
	BW_GUARD;
	for (uint i=0; i < tasks_.size(); i++)
	{
		if (tasks_[i].task != NULL)
		{
			tasks_[i].task->onCancel();
		}
	}
}

bool ProgressDisplay::onProgress()
{
	bool ret = this->draw( );
	this->checkCancel();
	return ret;
}

BW_END_NAMESPACE

/*progress.cpp*/
