#include "pch.hpp"
#include "closed_captions.hpp"
#include "ashes/simple_gui.hpp"
#include "ashes/text_gui_component.hpp"
#include "romp/font_manager.hpp"
#include <strstream>

#ifndef CODE_INLINE
#include "closed_captions.ipp"
#endif

BW_BEGIN_NAMESPACE

namespace
{
	SimpleMutex s_pendingMessagesMutex;


	class ScopedReentryCounter
	{
	public:
		ScopedReentryCounter()
		{
			this->incrementCounter();
		}

		~ScopedReentryCounter()
		{
			this->decrementCounter();
		}

		static void incrementCounter() { ++s_reentryCounter_; }
		static void decrementCounter() { --s_reentryCounter_; }

		static bool checkReentryCounter( const BW::wstring & msg )
		{
			if (s_reentryCounter_ != 0)
			{
				OutputDebugString( L"ERROR: ClosedCaptions reentry with msg: '" );
				OutputDebugString( msg.c_str() );
				OutputDebugString( L"'\n" );
				return false;
			}

			return true;
		}

	private:
		static THREADLOCAL( int ) s_reentryCounter_;
	};

	/*static*/ THREADLOCAL( int ) ScopedReentryCounter::s_reentryCounter_ = 0;

} // anonymous namespace



// -----------------------------------------------------------------------------
// Section: Construction/Destruction
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
ClosedCaptions::ClosedCaptions( int bufferSize, PyTypeObject * pType ) :
	PyObjectPlus( pType ),
	head_( 0 ),
	timeout_( 6.f ),
	fadeout_( 2.f ),
	clipHeight_( 0.05f )
{
	msgs_.resize( bufferSize );

	margin_.x = 0.05f;
	margin_.y = 0.05f;

	root_ = new SimpleGUIComponent( "" );
	root_->visible( false );
	root_->width( 0.f );
	root_->height( 0.f );
	SimpleGUI::instance().addSimpleComponent( *root_ );
	font_ = FontManager::instance().getCachedFont( "default_small.font" );

	for ( int i = 0; i < bufferSize; i++ )
	{
		msgs_[i].age_ = timeout_;
		msgs_[i].component_ = new TextGUIComponent( FontManager::instance().getCachedFont( "default_small.font" ) );
		msgs_[i].backing_ = new SimpleGUIComponent( "resources/maps/gui/background.dds" );

		//set up backing component properties
		SimpleGUIComponent * b = msgs_[i].backing_;
		b->position( Vector3( -1.f + margin_.x, -1.f + margin_.y, 1.f ) );
		b->widthMode( SimpleGUIComponent::SIZE_MODE_CLIP ); 
		b->heightMode( SimpleGUIComponent::SIZE_MODE_CLIP ); 
		b->width( 2.f - 2*margin_.x );
		b->height( clipHeight_ );
		b->anchor( SimpleGUIComponent::ANCHOR_H_LEFT,
				SimpleGUIComponent::ANCHOR_V_BOTTOM );
		b->colour( 0x40000020 );
		b->materialFX( SimpleGUIComponent::FX_BLEND );
		b->visible( false );
		b->tiled( true );
		b->tileWidth( 16 );
		b->tileHeight( 16 );

		BW::ostringstream childNameStream;
		childNameStream << "CaptionBackground_" << i;
		root_->addChild( childNameStream.str(), b );

		//set up text component properties
		TextGUIComponent * c = msgs_[i].component_;

		c->filterType( SimpleGUIComponent::FT_LINEAR );
		c->slimLabel( "***" );
		c->visible( false );
		c->colour( 0xffffffff );
		c->anchor( SimpleGUIComponent::ANCHOR_H_LEFT,
				SimpleGUIComponent::ANCHOR_V_BOTTOM );

		BW::ostringstream captionNameStream;
		captionNameStream << "CaptionText_" << i;
		b->addChild( captionNameStream.str(), c );
	}
}


/**
 *	Destructor.
 */
ClosedCaptions::~ClosedCaptions()
{
	for ( uint i = 0; i < msgs_.size(); i++ )
	{
		TextGUIComponent * c = msgs_[i].component_;
		root_->removeChild( c );
		Py_DECREF( c );
		msgs_[i].component_ = NULL;

		SimpleGUIComponent * b = msgs_[i].backing_;
		root_->removeChild( b );
		Py_DECREF( b );
		msgs_[i].backing_ = NULL;
	}

	SimpleGUI::instance().removeSimpleComponent( *root_ );

	Py_DecRef(root_);
}


/**
 *	This method is called by the global commentary system, when
 *	a commentary msg was added.
 *
 *	Note that we push the message onto a pending message queue.
 *	This is because we may have messages added when the windowing
 *	system doesn't know how large an area it has to draw in.
 *	If this is the case, then we can't successfully add a message
 *	because we don't know whether or not to clip it.
 *
 *
 *	@param msg	the new commentary message
 *	@param id	the id of the type of commentary message
 */
void ClosedCaptions::onAddMsg( const BW::wstring & msg, int id )
{
	if (!ScopedReentryCounter::checkReentryCounter( msg )) return;
	ScopedReentryCounter reentryCounter;

	SimpleMutexHolder mutex( s_pendingMessagesMutex );

	pendingMessages_.push_back( PendingMessage( msg, id ) );
}


/**
 *	This method adds all pending messages to the gui system.
 */
void ClosedCaptions::parseEventQueue()
{
	PendingMessages pm;

	{
		SimpleMutexHolder mutex( s_pendingMessagesMutex );
		pm.swap( pendingMessages_ );
	}

	PendingMessages::iterator it = pm.begin();
	PendingMessages::iterator end = pm.end();

	while ( it != end )
	{
		PendingMessage& pm = *it++;

		addMsg( pm.msg_, pm.id_ );
	}
}


/**
 *	This method adds a message to the gui system.
 */
void ClosedCaptions::addMsg( const BW::wstring & msg, int id )
{
	if (!ScopedReentryCounter::checkReentryCounter( msg )) return;
	ScopedReentryCounter reentryCounter;

	if ( msgs_.empty() )
		return;

	TextGUIComponent * c = msgs_[head_].component_;
	SimpleGUIComponent * b = msgs_[head_].backing_;

	float w,h;
	SimpleGUI::instance().clipRangesToPixel( b->width(), b->height(), &w, &h );

	if ( !font_ )
		return;

	if ( msg.length() == 0 )
		return;

	uint stringWidth = font_->metrics().stringWidth( msg );

	if ( font_->metrics().maxWidth() <= 0 )
		return;	

	if ( stringWidth > w )
	{
		BW::vector<BW::wstring> lines;
		int sw = (int)w;
		int sh = (int)h;
 		lines = font_->metrics().breakString( msg, sw, sh );
		if ( lines.size() > 1 )
		{
			for ( size_t i=0; i<lines.size(); i++ )
			{
				ScopedReentryCounter::decrementCounter();
				this->addMsg( lines[i], id );
				ScopedReentryCounter::incrementCounter();
			}
			return;
		}
	}

	c->visible( true );
	b->visible( true );
	c->label( msg );
	msgs_[head_].age_ = timeout_;

	switch ( id )
	{
	case Commentary::COMMENT:
		c->colour( 0xffffffff );
		break;
	case Commentary::CRITICAL:
		c->colour( 0xffff0000 );
		break;
	case Commentary::ERROR_LEVEL:
		c->colour( 0xffff0000 );
		break;
	case Commentary::WARNING:
		c->colour( 0xffff8000 );
		break;
	case Commentary::HACK:
		c->colour( 0xff4040ff );
		break;
	default:
		c->colour( 0xffffffff );
	}

	head_++;
	head_ %= msgs_.size();	
}


/**
 *	This method updates our messages.
 *
 *	@param dTime the change in time since the last frame.
 */
void ClosedCaptions::update( float dTime )
{
	if (font_)
	{
		if ( !SimpleGUI::instance().hasResolutionChanged() )
		{
			this->parseEventQueue();
		}
		else
		{
			clipHeight_ = font_->metrics().clipHeight();
		}

		size_t idx = head_;

		//update all ages
		for ( uint i = 0; i < msgs_.size(); i++ )
		{
			Caption & c = msgs_[ idx ];

			if ( c.age_ > 0.f )
			{
				c.age_ -= dTime;

				if ( c.age_ <= 0.f )
				{
					c.component_->visible( false );
					c.backing_->visible( false );
					c.age_ = timeout_;
				}
				else 
				{
					unsigned int alpha = 255;

					if ( c.age_ < fadeout_ )
					{
						alpha = (unsigned int)( ( c.age_ / fadeout_ ) * 255.f );
					}

					alpha <<= 24;

					unsigned int col = c.component_->colour() & 0x00ffffff;
					c.component_->colour( col | alpha );

					alpha >>= 24;
					alpha /= 4;
					alpha <<= 24;
					unsigned int bcol = c.backing_->colour() & 0x00ffffff;
					c.backing_->colour( bcol | alpha );
				}
			}

			idx++;
			idx %= msgs_.size();
		}

		//Now go through and lay them out
		idx = head_;

		for ( uint i = 0; i < msgs_.size(); i++ )
		{
			idx--;
			if ( idx > msgs_.size() - 1 )
				idx = msgs_.size() - 1;

			TextGUIComponent & c = *msgs_[ idx ].component_;
			c.position( Vector3( -1.f + margin_.x, -1.f + margin_.y + ( (float)i * clipHeight_ ), 0.9f ) );

			SimpleGUIComponent & b = *msgs_[ idx ].backing_;
			b.height( clipHeight_ );
			b.position( Vector3( -1.f + margin_.x, -1.f + margin_.y + ( (float)i * clipHeight_ ), 1.f ) );
		}
	}
}

// -----------------------------------------------------------------------------
// Section: Python stuff
// -----------------------------------------------------------------------------

PY_TYPEOBJECT( ClosedCaptions )

PY_BEGIN_METHODS( ClosedCaptions )
	PY_METHOD( update )
	PY_METHOD( addAsView )
	PY_METHOD( delAsView )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( ClosedCaptions )
	PY_ATTRIBUTE( visible )
PY_END_ATTRIBUTES()

PY_FACTORY( ClosedCaptions, GUI )

/**
 *	This method gets the captions to update
 */
PyObject * ClosedCaptions::py_update( PyObject * args )
{
	float dtime;
	if (!PyArg_ParseTuple( args, "f", &dtime ))
	{
		PyErr_SetString( PyExc_TypeError, "ClosedCaptions.update: "
			"Argument parsing error: Expected float dtime" );
		return NULL;
	}

	this->update( dtime );

	Py_RETURN_NONE;
}

/**
 *	Put this one in the central mi...er.. commentary position
 */
PyObject * ClosedCaptions::py_addAsView( PyObject * args )
{
	Commentary::instance().addView( this );
	Py_RETURN_NONE;
}

/**
 *	Now it's walking a view fine line
 */
PyObject * ClosedCaptions::py_delAsView( PyObject * args )
{
	Commentary::instance().delView( this );
	Py_RETURN_NONE;
}


/**
 *	Factory method
 */
PyObject * ClosedCaptions::pyNew( PyObject * args )
{
	int nlines = 5;
	if (!PyArg_ParseTuple( args, "|i", &nlines ))
	{
		PyErr_SetString( PyExc_TypeError, "ClosedCaptions() "
			"expects an optional integer number of lines to display" );
		return NULL;
	}

	return new ClosedCaptions( nlines );
}

BW_END_NAMESPACE

// closed_captions.cpp
