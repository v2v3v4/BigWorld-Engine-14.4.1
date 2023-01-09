#include "pch.hpp"

#if ENABLE_CONSOLES

#include "console.hpp"

#include "moo/gpu_profiler.hpp"
#include "romp/console_manager.hpp"
#include "romp/histogram_provider.hpp"

#include "cstdmf/config.hpp"
#include "cstdmf/dogwatch.hpp"
#include "cstdmf/locale.hpp"
#include "cstdmf/watcher.hpp"
#include "cstdmf/string_utils.hpp"

#include "math/colour.hpp"

#include "pyscript/python_input_substituter.hpp"
#include "pyscript/script.hpp"

#include "script/script_output_hook.hpp"

DECLARE_DEBUG_COMPONENT2( "Romp", 0 )


BW_BEGIN_NAMESPACE

#ifndef CODE_INLINE
#include "console.ipp"
#endif

namespace { // anonymous

// Named constants
const int			AUTO_INDEX_RESET	= 0x7FFFFFFF;
const BW::wstring	CONSOLE_SEPARATORS	= L" ~!%^&*+-={}\\|;:'\",/<>?";

// Helper types
typedef BW::vector<BW::wstring> StringVector;
typedef std::pair< BW::wstring, BW::wstring > StringPair;

// Helper functions
StringPair splitHeadAndExpression( const BW::wstring & line );
StringPair splitObjAndAttr( const BW::wstring & expression );
StringVector getAttributes( const BW::wstring & object );

StringVector filterAttributes(
	const StringVector & attributes,
	const BW::wstring  & startsWith );

BW::wstring buildCompletedLine(
	const BW::wstring & head,
	const BW::wstring & object,
	const BW::wstring & attribute );

BW::wstring getCommonPrefix(
	const StringVector & filtered,
	const BW::wstring  & knownStart );

void printAttributes(
	XConsole           & console,
	const StringVector & attributes );

} // namespace anonymous


// -----------------------------------------------------------------------------
// Section: StatisticsConsole
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
StatisticsConsole::StatisticsConsole( StatisticsConsole::Handler * pHandler ) :
	profileMode_(),
	profileModeInclusive_(),
	profileModeFreeze_(),
	pHandler_( pHandler )
{
	BW_GUARD;
}


/**
 *	This method initialises this object.
 */
bool StatisticsConsole::init()
{
	BW_GUARD;
	bool succeeded = XConsole::init();

	if (succeeded)
	{
		this->setConsoleColour( 0xFFFFFF00 );
		this->setCursor( 0, 0 );
		this->print( L"Statistics Console\n" );
	}

	profileMode_ = 0;
	profileModeInclusive_ = false;
	profileModeFreeze_ = false;
	return succeeded;
}


/**
 *	This method overrides the XConsole method. It is called to update this
 *	console.
 */
void StatisticsConsole::update()
{
	BW_GUARD;
	this->clear();

	DogWatchManager::iterator	selection = this->followPath();
	selection.flags( selection.flags() | 4 );

	pHandler_->displayStatistics( *this );

	selection.flags( selection.flags() & ~4 );
}


/**
 *	This method overrides the InputHandler method.
 */
bool StatisticsConsole::handleKeyEvent( const KeyEvent & event )
{
	BW_GUARD;
	if (!event.isKeyDown())
	{
		// required behaviour for consoles
		return false;
	}
	bool handled = true;

	DogWatchManager::iterator iter = this->followPath();

	switch (event.key())
	{
	case KeyCode::KEY_JOY3:
	case KeyCode::KEY_RETURN:
		if (!profileMode_)
		{
			if (event.isAltDown())
			{
				handled = false;
				break;
			}

			if (iter.begin() != iter.end())
			{
				iter.flags( iter.flags() ^ 1 );
			}
		}
#if ENABLE_PROFILER
		if(profileMode_ == Profiler::HIERARCHICAL)
		{
			g_profiler.toggleHierChildren();
		}
#endif	
		break;

	case KeyCode::KEY_G:
	case KeyCode::KEY_JOY2:
		if (!profileMode_)
		{
			iter.flags( iter.flags() ^ 2 );
		}
#if ENABLE_PROFILER
		// hold down shift to toggle all the children of this entry (including this entry)
		if(profileMode_ == Profiler::HIERARCHICAL)
		{
			g_profiler.toggleGraph(event.isShiftDown());
		}
#endif
		break;

	case KeyCode::KEY_RBRACKET:
	case KeyCode::KEY_PGDN:
	case KeyCode::KEY_JOY1:
	{
		if (!profileMode_)
		{
			// ok, we want to move down.
			size_t psize = path_.size();

			// are we visible and we have children?
			if ((iter.flags() & 1) && iter.begin() != iter.end())
			{
				path_.push_back( 0 );
				break;
			}

			// ok, add one to the last path element until we succeed then
			bool movedDown = false;

			while (!movedDown && psize > 0)
			{
				path_.back()++;
				iter = this->followPath();
				movedDown = path_.size() == psize;
				psize = path_.size();
			}
		}
#if ENABLE_PROFILER
		if(profileMode_ == Profiler::HIERARCHICAL)
		{
			g_profiler.nextHierEntry();
		}
#endif
		break;
	}

	case KeyCode::KEY_LBRACKET:
	case KeyCode::KEY_PGUP:
	case KeyCode::KEY_JOY0:
	{
		if (!profileMode_)
		{
			// we're moving up in the world...
			if (path_.size() > 0)
			{
				// are we at the start of our group?
				if (path_.back() == 0)
				{		// yep, go up a level, and that's it
					path_.erase( path_.end() - 1 );
					break;
				}

				// nope, so take a step back
				path_.back()--;
				iter = this->followPath();
			}

			// and follow down the chain of last children
			while ((iter.flags() & 1) && iter.begin() != iter.end())
			{
				int	idx = 0;

				// find the last child
				DogWatchManager::iterator post = iter.begin();
				DogWatchManager::iterator end = iter.end();
				do
				{
					iter = post;
					++post;
					idx++;
				}
				while (post != end);

				// and repeat!
				path_.push_back( idx - 1 );
				// (iter already set)
			}
		}
#if ENABLE_PROFILER
		if(profileMode_ == Profiler::HIERARCHICAL)
		{
			g_profiler.prevHierEntry();
		}
#endif
		break;
	}

	case KeyCode::KEY_NUMPADMINUS:
		this->scrollUp();
		break;

	case KeyCode::KEY_ADD:
		this->scrollDown();
		break;

	case KeyCode::KEY_HOME:
		this->scrollOffset( 0 );
#if ENABLE_PROFILER
		if(profileMode_ == Profiler::HIERARCHICAL)
		{
			// move to the first entry
			while( g_profiler.prevHierEntry() )
				;
		}
#endif

		break;

	case KeyCode::KEY_B:
#if ENABLE_NVIDIA_PERFKIT
		if (!profileMode_)
		{
			Moo::GPUProfiler::instance().perfkitPerformExperiment();
		}
#endif
#if ENABLE_PROFILER
		if(profileMode_ == Profiler::SORT_BY_TIME ||
			profileMode_ == Profiler::SORT_BY_NUMCALLS ||
#if ENABLE_PER_CORE_PROFILER
			profileMode_ == Profiler::CORES ||
#endif
			profileMode_ == Profiler::CPU_GPU )
		{
			g_profiler.setProfileBarVisibility(!g_profiler.getProfileBarVisibility());
		}
		break;


	case KeyCode::KEY_P:
		if (profileMode_ == Profiler::SORT_BY_TIME)
		{
			profileMode_ = Profiler::SORT_BY_NUMCALLS;
		}
		else if (profileMode_ == Profiler::SORT_BY_NUMCALLS)
		{
			profileMode_ = Profiler::CPU_GPU;
		}
#if ENABLE_PER_CORE_PROFILER
		else if (profileMode_ == Profiler::CPU_GPU)
		{
			profileMode_ = Profiler::CORES;
		}
#endif
		else
		{
			profileMode_ = Profiler::SORT_BY_TIME;
		}

		break;
	case KeyCode::KEY_H:
		profileMode_ = Profiler::HIERARCHICAL;
		profileModeInclusive_ = true; // Start out inclusive
		break;
	case KeyCode::KEY_T:
		profileMode_ = Profiler::GRAPHS;
		break;
	case KeyCode::KEY_I:
		profileModeInclusive_ = !profileModeInclusive_;
		break;
	case KeyCode::KEY_M:
		profileMode_ = 0;
		break;
	case KeyCode::KEY_E:
		{
			bool enableFPSFreeze = g_profiler.allowHitchFreezing();
			g_profiler.allowHitchFreezing( !enableFPSFreeze );
		}
		break;
	case KeyCode::KEY_F:
		if (g_profiler.hitchFreeze() == false)
		{
			profileModeFreeze_ = !profileModeFreeze_;
		}
		else
		{
			g_profiler.hitchFreeze( false );
		}
		break;
	case KeyCode::KEY_COMMA:
		{
			g_profiler.prevCategory();
		}
		break;
	case KeyCode::KEY_PERIOD:
		{
			g_profiler.nextCategory();
		}
		break;
	case KeyCode::KEY_1:
	case KeyCode::KEY_2:
	case KeyCode::KEY_3:
	case KeyCode::KEY_4:
	case KeyCode::KEY_5:
	case KeyCode::KEY_6:
	case KeyCode::KEY_7:
	case KeyCode::KEY_8:
	case KeyCode::KEY_9:
	case KeyCode::KEY_0:
		{
			int threadId = int(event.key()) - 1;
			if (threadId == 10)
			{
				threadId = 0;
			}
			g_profiler.setThreadVisibility(threadId, !g_profiler.getThreadVisibility(threadId));
		}
		break;
#endif
	default:
		handled = false;
		break;
	}

	return handled;
}


/**
 *	This method turns path_ into an iterator.
 */
DogWatchManager::iterator StatisticsConsole::followPath()
{
	BW_GUARD;
	DogWatchManager::iterator	iter = DogWatchManager::instance().begin();

	for (uint i = 0; i < path_.size(); i++)
	{
		DogWatchManager::iterator sub = iter.begin();

		// iterate until we get to path_[i] down
		for (int tidx = path_[i]; tidx > 0 && sub != iter.end(); tidx--)
		{
			++sub;
		}

		// did we run out of elements (not sure if this can happen)
		if (sub == iter.end())
		{
			path_.erase( path_.begin() + i, path_.end() );
			return iter;
		}

		// ok, this one looks good then
		iter = sub;
	}

	return iter;
}


// -----------------------------------------------------------------------------
// Section: ResourceUsageConsole
// -----------------------------------------------------------------------------
/**
 *	Constructor.
 */
ResourceUsageConsole::ResourceUsageConsole( Handler * pHandler ) :
	pHandler_( pHandler )
{
	BW_GUARD;
}


/**
 *	This method initialises this object.
 */
bool ResourceUsageConsole::init()
{
	BW_GUARD;
	bool succeeded = XConsole::init();

	if (succeeded)
	{
		this->setConsoleColour( 0xFFFFFF00 );
		this->setCursor( 0, 0 );
		this->print( L"Resource Usage Console\n" );
	}

	return succeeded;
}


/**
 *	This method overrides the XConsole method. It is called to update this
 *	console.
 */
void ResourceUsageConsole::update()
{
	BW_GUARD;
	this->clear();

	pHandler_->displayResourceStatistics( *this );
}


/**
 *	This method overrides the InputHandler method.
 */
bool ResourceUsageConsole::handleKeyEvent( const KeyEvent & event )
{
	BW_GUARD;
	if (!event.isKeyDown())
	{
		// required behaviour for consoles
		return false;
	}

	bool handled = true;

	switch (event.key())
	{
	// Cycle through the memory type groupings
	case KeyCode::KEY_SPACE:
	case KeyCode::KEY_JOY2:
		pHandler_->cycleGranularity();
		break;

	// Scroll up
	case KeyCode::KEY_NUMPADMINUS:
	case KeyCode::KEY_PGUP:
		this->scrollUp();
		break;

	// Scroll down
	case KeyCode::KEY_ADD:
	case KeyCode::KEY_PGDN:
		this->scrollDown();
		break;

	// Jump to top of page
	case KeyCode::KEY_HOME:
		this->scrollOffset( 0 );
		break;

	// Save to CSV
	case KeyCode::KEY_C:
		pHandler_->dumpToCSV(*this);
		break;

	// Key not handled
	default:
		handled = false;
		break;
	}

	return handled;
}


// -----------------------------------------------------------------------------
// Section: HistogramConsole
// -----------------------------------------------------------------------------

bool HistogramConsole::allowDarkBackground() const
{
	return !Luminance_ && !R_ && !G_ && !B_;
}

/**
 *	This method initialises this object.
 */
bool HistogramConsole::init()
{
	BW_GUARD;
	Luminance_ = true;
	R_ = true;
	G_ = true;
	B_ = true;
	Background_ = false;
	topRatio_ = 1;

	bool succeeded = XConsole::init();

	if (succeeded)
	{
		this->setConsoleColour( 0xFFFFFF00 );
		this->setCursor( 0, 0 );
		this->print( L"Histogram Console\n" );
	}

	return succeeded;
}

void HistogramConsole::activate( bool isReactivate )
{
	BW_GUARD;
	InputConsole::activate( isReactivate );
	if( !isReactivate )
		HistogramProvider::instance().addUser();
}

void HistogramConsole::deactivate()
{
	BW_GUARD;
	HistogramProvider::instance().removeUser();
}

class ConsoleGraphTrack
{
	float left_;// all dimensions are from 0.0 to 1.0
	float top_;
	float width_;
	float height_;
public:
	enum DUMMY
	{
		ENTRY_SIZE = 256,
		MARGIN_SIZE = 1
	};
	ConsoleGraphTrack( float left, float top, float width, float height )
		: left_( left ), top_( top ), width_( width ), height_( height )
	{}

	void clear()
	{
		BW_GUARD;
		static BW::vector< Moo::VertexTL > tlvv;

		// draw the background
		tlvv.erase( tlvv.begin(), tlvv.end() );

		float left = left_ * Moo::rc().screenWidth();
		float right = ( left_ + width_ ) * Moo::rc().screenWidth();
		float top = top_ * Moo::rc().screenHeight();
		float bottom = ( top_ + height_ ) * Moo::rc().screenHeight();

		Moo::VertexTL tlv;
		tlv.pos_.z = 0;
		tlv.pos_.w = 1;
		tlv.colour_ = 0x00000000;

		tlv.pos_.x = left;				tlv.pos_.y = top;			tlvv.push_back( tlv );
		tlv.pos_.x = right;				tlv.pos_.y = top;			tlvv.push_back( tlv );
		tlv.pos_.x = right;				tlv.pos_.y = bottom;		tlvv.push_back( tlv );

		tlv.pos_.x = left;				tlv.pos_.y = top;			tlvv.push_back( tlv );
		tlv.pos_.x = right;				tlv.pos_.y = bottom;		tlvv.push_back( tlv );
		tlv.pos_.x = left;				tlv.pos_.y = bottom;		tlvv.push_back( tlv );

		Moo::Material::setVertexColour();

		Moo::rc().setRenderState( D3DRS_ALPHABLENDENABLE, FALSE );
		Moo::rc().setRenderState( D3DRS_SRCBLEND, D3DBLEND_ONE );
		Moo::rc().setRenderState( D3DRS_DESTBLEND, D3DBLEND_ONE );
		Moo::rc().setRenderState( D3DRS_BLENDOP, D3DBLENDOP_ADD );
		Moo::rc().setRenderState( D3DRS_ZENABLE, TRUE );
		Moo::rc().setRenderState( D3DRS_ZFUNC, D3DCMP_ALWAYS );

		Moo::rc().setVertexShader( NULL );
		Moo::rc().setFVF( D3DFVF_XYZRHW | D3DFVF_DIFFUSE );

		Moo::rc().drawPrimitiveUP( D3DPT_TRIANGLELIST, (uint)tlvv.size() / 3,
			&tlvv.front(), sizeof( Moo::VertexTL ) );
	}
	void draw( uint32 color, const unsigned int* histogram, unsigned int topRatio ) const
	{
		BW_GUARD;
		static BW::vector< Moo::VertexTL > tlvv;

		unsigned int max = 0;
		for( int i = 0; i < ENTRY_SIZE; ++i )
			if( histogram[ i ] > max )
				max = histogram[ i ];

		max /= topRatio;

		tlvv.erase( tlvv.begin(), tlvv.end() );

		// draw the border
		{
			float margin = MARGIN_SIZE;//float( MARGIN_SIZE ) / ( ENTRY_SIZE + MARGIN_SIZE * 2 ) * Moo::rc().screenWidth();
			float left = left_ * Moo::rc().screenWidth();
			float right = ( left_ + width_ ) * Moo::rc().screenWidth();
			float top = top_ * Moo::rc().screenHeight();
			float bottom = ( top_ + height_ ) * Moo::rc().screenHeight();

			Moo::VertexTL tlv;
			tlv.pos_.z = 0;
			tlv.pos_.w = 1;
			tlv.colour_ = 0xffffffff;

			tlv.pos_.x = left;				tlv.pos_.y = top;				tlvv.push_back( tlv );
			tlv.pos_.x = right;				tlv.pos_.y = top;				tlvv.push_back( tlv );
			tlv.pos_.x = right;				tlv.pos_.y = top + margin;		tlvv.push_back( tlv );

			tlv.pos_.x = left;				tlv.pos_.y = top;				tlvv.push_back( tlv );
			tlv.pos_.x = right;				tlv.pos_.y = top + margin;		tlvv.push_back( tlv );
			tlv.pos_.x = left;				tlv.pos_.y = top + margin;		tlvv.push_back( tlv );

			tlv.pos_.x = right;				tlv.pos_.y = top;				tlvv.push_back( tlv );
			tlv.pos_.x = right;				tlv.pos_.y = bottom;			tlvv.push_back( tlv );
			tlv.pos_.x = right - margin;	tlv.pos_.y = bottom;			tlvv.push_back( tlv );

			tlv.pos_.x = right;				tlv.pos_.y = top;				tlvv.push_back( tlv );
			tlv.pos_.x = right - margin;	tlv.pos_.y = bottom;			tlvv.push_back( tlv );
			tlv.pos_.x = right - margin;	tlv.pos_.y = top;				tlvv.push_back( tlv );

			tlv.pos_.x = left;				tlv.pos_.y = top;				tlvv.push_back( tlv );
			tlv.pos_.x = left + margin;		tlv.pos_.y = top;				tlvv.push_back( tlv );
			tlv.pos_.x = left + margin;		tlv.pos_.y = bottom;			tlvv.push_back( tlv );

			tlv.pos_.x = left;				tlv.pos_.y = top;				tlvv.push_back( tlv );
			tlv.pos_.x = left + margin;		tlv.pos_.y = bottom;			tlvv.push_back( tlv );
			tlv.pos_.x = left;				tlv.pos_.y = bottom;			tlvv.push_back( tlv );

			tlv.pos_.x = left;				tlv.pos_.y = bottom - margin;	tlvv.push_back( tlv );
			tlv.pos_.x = right;				tlv.pos_.y = bottom - margin;	tlvv.push_back( tlv );
			tlv.pos_.x = right;				tlv.pos_.y = bottom;			tlvv.push_back( tlv );

			tlv.pos_.x = left;				tlv.pos_.y = bottom - margin;	tlvv.push_back( tlv );
			tlv.pos_.x = right;				tlv.pos_.y = bottom;			tlvv.push_back( tlv );
			tlv.pos_.x = left;				tlv.pos_.y = bottom;			tlvv.push_back( tlv );
		}

		Moo::VertexTL tlv;
		tlv.pos_.z = 0;
		tlv.pos_.w = 1;
		tlv.colour_ = color;

		float lastX = 0, lastY = height_;
		float currentX, currentY;

		for( int i = 0; i < ENTRY_SIZE; ++i )
		{
			currentX = float( i + MARGIN_SIZE ) / ( ENTRY_SIZE + MARGIN_SIZE * 2 ) * width_;
			currentY = height_ - ( histogram[ i ] <= max ? histogram[ i ] : max ) / float( max ) * height_;

			tlv.pos_.x = ( lastX + left_ ) * Moo::rc().screenWidth();
			tlv.pos_.y = ( height_ + top_ ) * Moo::rc().screenHeight();
			tlvv.push_back( tlv );

			tlv.pos_.x = ( lastX + left_ ) * Moo::rc().screenWidth();
			tlv.pos_.y = ( lastY + top_ ) * Moo::rc().screenHeight();
			tlvv.push_back( tlv );

			tlv.pos_.x = ( currentX + left_ ) * Moo::rc().screenWidth();
			tlv.pos_.y = ( currentY + top_ ) * Moo::rc().screenHeight();
			tlvv.push_back( tlv );

			tlv.pos_.x = ( lastX + left_ ) * Moo::rc().screenWidth();
			tlv.pos_.y = ( height_ + top_ ) * Moo::rc().screenHeight();
			tlvv.push_back( tlv );

			tlv.pos_.x = ( currentX + left_ ) * Moo::rc().screenWidth();
			tlv.pos_.y = ( currentY + top_ ) * Moo::rc().screenHeight();
			tlvv.push_back( tlv );

			tlv.pos_.y = ( height_ + top_ ) * Moo::rc().screenHeight();
			tlvv.push_back( tlv );

			lastX = currentX;
			lastY = currentY;
		}

		if( tlvv.size() )
		{
			Moo::Material::setVertexColour();

			Moo::rc().setRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
			Moo::rc().setRenderState( D3DRS_SRCBLEND, D3DBLEND_ONE );
			Moo::rc().setRenderState( D3DRS_DESTBLEND, D3DBLEND_ONE );
			Moo::rc().setRenderState( D3DRS_BLENDOP, D3DBLENDOP_ADD );
			Moo::rc().setRenderState( D3DRS_ZENABLE, TRUE );
			Moo::rc().setRenderState( D3DRS_ZFUNC, D3DCMP_ALWAYS );

			Moo::rc().setVertexShader( NULL );
			Moo::rc().setFVF( D3DFVF_XYZRHW | D3DFVF_DIFFUSE );

			Moo::rc().drawPrimitiveUP( D3DPT_TRIANGLELIST, (uint)tlvv.size() / 3,
				&tlvv.front(), sizeof( Moo::VertexTL ) );
		}

	}
};

/**
 *	This method overrides the XConsole method. It is called to update this
 *	console.
 */
void HistogramConsole::update()
{
	BW_GUARD;
	this->clear();
	this->setCursor( 0, 0 );
	BW::wstring title = L"Histogram : ";
	if( Luminance_ )
		title += L"Luminance";
	if( R_ )
		title += *title.rbegin() == L' ' ? L"Red" : L" + Red";
	if( G_ )
		title += *title.rbegin() == L' ' ? L"Green" : L" + Green";
	if( B_ )
		title += *title.rbegin() == L' ' ? L"Blue" : L" + Blue";
	if( *title.rbegin() == ' ' )
	{
		title += L"None\n\n";
		title += L"Press Numpad 0 to toggle Luminance\n";
		title += L"Press Numpad 1 to toggle Red\n";
		title += L"Press Numpad 2 to toggle Green\n";
		title += L"Press Numpad 3 to toggle Blue\n";
		title += L"Press Numpad 4 to toggle Background\n";
		title += L"Press Numpad 5 to increase range\n";
		title += L"Press Numpad 6 to decrease range\n";
	}
	this->print( title.c_str() );

	if( Luminance_ )
	{
		ConsoleGraphTrack graphTrack( 0.0f, 0.025f, 0.25f, 0.25f );
		if( Background_ )
			graphTrack.clear();
		graphTrack.draw( 0xffffffff, HistogramProvider::instance().get( HistogramProvider::HT_LUMINANCE ).value_, topRatio_ );
	}
	if( R_ || G_ || B_ )
	{
		ConsoleGraphTrack graphTrack( !Luminance_ ? 0.0f : 0.27f, 0.025f, 0.25f, 0.25f );
		if( Background_ )
			graphTrack.clear();

		if( R_ )
			graphTrack.draw( 0xffff0000, HistogramProvider::instance().get( HistogramProvider::HT_R ).value_, topRatio_ );
		if( G_ )
			graphTrack.draw( 0xff00ff00, HistogramProvider::instance().get( HistogramProvider::HT_G ).value_, topRatio_ );
		if( B_ )
			graphTrack.draw( 0xff0000ff, HistogramProvider::instance().get( HistogramProvider::HT_B ).value_, topRatio_ );
	}
}

/**
 *	This method overrides the InputHandler method.
 */
bool HistogramConsole::handleKeyEvent( const KeyEvent & event )
{
	BW_GUARD;
	if (!event.isKeyDown())
	{
		// required behaviour for consoles
		return false;
	}

	bool handled = true;

	switch (event.key())
	{
	case KeyCode::KEY_NUMPAD0:
		Luminance_ = !Luminance_;
		break;
	case KeyCode::KEY_NUMPAD1:
		R_ = !R_;
		break;
	case KeyCode::KEY_NUMPAD2:
		G_ = !G_;
		break;
	case KeyCode::KEY_NUMPAD3:
		B_ = !B_;
		break;
	case KeyCode::KEY_NUMPAD4:
		Background_ = !Background_;
		break;
	case KeyCode::KEY_NUMPAD5:
		if( topRatio_ > 1 )
			--topRatio_;
		break;
	case KeyCode::KEY_NUMPAD6:
		++topRatio_;
		break;
	default:
		handled = false;
		break;
	}

	return handled;
}


// -----------------------------------------------------------------------------
// Section: EditConsole
// -----------------------------------------------------------------------------

// 'this' : used in base member initialiser list
#pragma warning(push)
#pragma warning(disable: 4355)

/**
 *	Constructor.
 */
EditConsole::EditConsole() :
	lineEditor_(this),
	editLine_( 0 ),
	editCol_( 0 ),
	editColour_( 0xFFFFFFFF )
{
	BW_GUARD;
	this->editLine_ = this->visibleHeight()-1;
}

#pragma warning(pop)

/**
 *	This method overrides the InputHandler method to handle key presses.
 */
bool EditConsole::handleKeyEvent( const KeyEvent & event )
{
	BW_GUARD;
	bool handled = false;

	if (this->isCursorShowing())
	{
		BW::wstring resultLine;

		LineEditor::ProcessState state =
			lineEditor_.processKeyEvent( event, resultLine );

		switch (state)
		{
			case LineEditor::NOT_HANDLED:
				handled = false;
				break;

			case LineEditor::PROCESSED:
				this->update();
				handled = true;
				break;

			case LineEditor::RESULT_SET:
				this->processLine( resultLine );
				handled = true;
				break;

			default:
				MF_ASSERT( 0 );
				break;
		}
	}

	return handled;
}


/**
 *	This method overrides the InputHandler method to handle axis positions.
 */
bool EditConsole::handleAxisEvent( const AxisEvent & event )
{
	BW_GUARD;
	bool handled = false;

	if (this->isCursorShowing())
	{
		// we consume all axis events
		handled = true;
		// but we only use the associated key down events
	}

	return handled;
}


/**
 *	This method overrides the XConsole method. It is called to update this
 *	console.
 */
void EditConsole::update()
{
	BW_GUARD;
	this->XConsole::update();

	if (this->isCursorShowing())
	{
		// now do normal update stuff
		this->lineColourOverride( editLine_, editColour_ );

		this->setCursor( editCol_, editLine_ );
		this->print( this->prompt() );
		this->setCursor( editCol_ + (int)this->prompt().size(), editLine_ );
		this->print( lineEditor_.editString() );

		this->setCursor(
			editCol_ + (int)this->prompt().size() + lineEditor_.cursorPosition(), editLine_ );
	}
}


/// Deactivates this console
void EditConsole::deactivate()
{
	BW_GUARD;
	this->lineEditor_.deactivate();
}


/// Draw this console
void EditConsole::draw( float dTime )
{
	BW_GUARD;
	this->lineEditor_.tick( dTime );
	this->XConsole::draw( dTime );
}


/**
 * This method reset editLine_ according to visibleHeight()
 */
void EditConsole::visibleAreaChanged( int oldVisibleWidth, int oldVisibleHeight )
{
	BW_GUARD;
	this->editLine_ = this->visibleHeight() - 1;
}


/**
 *	This method hides the cursor.
 */
void EditConsole::hideCursor()
{
	BW_GUARD;
	XConsole::hideCursor();
	this->lineEditor_.deactivate();
	this->setCursor( editCol_, editLine_ );
	this->print( L" " );
}

void EditConsole::createUnmanagedObjects()
{
	BW_GUARD;
	XConsole::createUnmanagedObjects();
}


#if ENABLE_WATCHERS
// -----------------------------------------------------------------------------
// Section: DebugConsole
// -----------------------------------------------------------------------------

int DebugConsole::highlightItem_ = 0;

DebugConsole::~DebugConsole()
{
}
/**
 *	This method initialises this object.
 */
bool DebugConsole::init()
{
	BW_GUARD;
	bool succeeded = EditConsole::init();

	this->hideCursor();

	if (succeeded)
	{
		this->setConsoleColour( 0xFFFFFF00 );
		this->setCursor( 0, 0 );
		this->print( L"Debug Console\n" );
	}

	return succeeded;
}

/**
 *	This method is called when the debug console is activated
 *
 */
void DebugConsole::activate( bool isReactivate )
{
	BW_GUARD;
	InputConsole::activate( isReactivate );
}

void DebugConsole::deactivate()
{
	BW_GUARD;
	this->lineEditor_.deactivate();
}

/**
 * TODO: to be documented.
 */
class DebugConsoleWatcherVisitor : public WatcherVisitor
{
public:
	DebugConsoleWatcherVisitor( DebugConsole *pDebugConsole ) :
		pDebugConsole_( pDebugConsole ),
		count_( 0 )
	{
		BW_GUARD;
	}

	~DebugConsoleWatcherVisitor()
	{
		BW_GUARD;
		if (DebugConsole::highlightItem_ < 0)
		{
			DebugConsole::highlightItem_ = count_ - 1;
		}
		if (DebugConsole::highlightItem_ >= count_)
		{
			DebugConsole::highlightItem_ = 0;
		}
	}

protected:
	virtual bool visit( Watcher::Mode mode,
		const BW::string & label,
		const BW::string & desc,
		const BW::string & value )
	{
		BW_GUARD;
		char buf[256];

		if (mode == Watcher::WT_DIRECTORY)
		{
			bw_snprintf( buf, sizeof(buf), "%2d [%s]\n",
				count_, label.c_str() );
		}
		else
		{
			bw_snprintf( buf, sizeof(buf), "%2d %-14s = %8s    \n",
				count_, label.c_str(), value.c_str() );
		}

		pDebugConsole_->lineColourOverride( pDebugConsole_->cursorY(),
			(count_ == DebugConsole::highlightItem_)	? 0xffff0000 :
			(mode == Watcher::WT_READ_WRITE)			? 0xffffffff :
			(mode == Watcher::WT_DIRECTORY)				? 0xffffff20 :
														  0xff9f9f9f );

		if (count_ == DebugConsole::highlightItem_)
		{
			int8 x, y;
			x = pDebugConsole_->cursorX();
			y = pDebugConsole_->cursorY();

			pDebugConsole_->lineColourOverride( pDebugConsole_->visibleHeight() - 4, 0xffffffff );
			pDebugConsole_->lineColourOverride( pDebugConsole_->visibleHeight() - 3, 0xffffffff );
			pDebugConsole_->lineColourOverride( pDebugConsole_->visibleHeight() - 2, 0xffffffff );

			uint iChar = 0;

			// print desription with word wrap at the bottom of the console
			pDebugConsole_->setCursor( 0, pDebugConsole_->visibleHeight() - 4 );

			BW::vector< BW::string > tokens;
			bw_tokenise< BW::string >( desc, " \n", tokens );
			for ( BW::vector< BW::string >::iterator it = tokens.begin() ;
				it != tokens.end() ; ++it )
			{
				const BW::string & token = *it;
				if( static_cast< int >( token.length() ) > 
					( pDebugConsole_->visibleWidth() - pDebugConsole_->cursorX() ) )
				{
					pDebugConsole_->print( "\n" );
				}

				if( pDebugConsole_->cursorX() != 0 )
					pDebugConsole_->print( " " );
				pDebugConsole_->print( token );
			}
			pDebugConsole_->setCursor( x, y );
		}

		pDebugConsole_->print( buf );

		count_++;

		return true;
	}
private:
	DebugConsole	*pDebugConsole_;
	int				count_;
};


/**
 *	This method overrides the XConsole method. It is called to update this
 *	console.
 */
void DebugConsole::update()
{
	BW_GUARD;
	this->clear();

	this->setCursor( 0, 0 );
	this->print( L"Debug Console\n" );
	this->print( path_.c_str() );
	this->print( L"\n\n" );
	int headerLines = this->cursorY();

	DebugConsoleWatcherVisitor	dcwv( this );

#if ENABLE_WATCHERS
	Watcher::rootWatcher().visitChildren( NULL, path_.c_str(), dcwv );
#endif

	EditConsole::update();

	const int scro = this->scrollOffset();
	if (highlightItem_ <= scro && scro != 0)
		this->scrollUp();
	if (highlightItem_ >= scro + (this->visibleHeight() - headerLines - 1))
		this->scrollDown();
}


/**
 *	This method processes the current line.
 *
 *	@param line		The current line to process.
 */
void DebugConsole::processLine( const BW::wstring & wline )
{
	BW_GUARD;
	BW::string line = bw_wtoutf8( wline );

	if (line.substr( 0, 2 ) == "cd")
	{
		if (line.length() == 2)
		{
			path_ = "";
		}
		else
		{
			path_ += line.substr( 3, line.length() - 3 ) + "/";
		}

		highlightItem_ = 0;
	}
	else if (line.substr( 0, 1 ) == ":")
	{
		int gotoItem = 0;
		if (watcherStringToValue(line.c_str() + 1, gotoItem ))
		{
			highlightItem_ = gotoItem;
		}
	}
	else
	{
		BW::string::size_type assignmentLocation = line.find_first_of( "=" );

		if (assignmentLocation != BW::string::npos) 
		{
			// Strip
			BW::string::size_type startPos = line.find_first_not_of( " " );
			BW::string::size_type endPos = line.find_last_not_of( " ", assignmentLocation - 1 );
			BW::string id	 = line.substr( startPos, endPos - startPos + 1 );

			BW::string value = line.substr( assignmentLocation + 1, line.size() );

			Watcher::rootWatcher().setFromString( NULL, (path_ + id).c_str(), value.c_str() );
		}
	}

	this->hideCursor();
}


/**
 *	This method overrides the EditConsole method to process key events.
 *
 *	@param event	An object representing the key event.
 *
 *	@return			True if it was handled, false otherwise.
 */
bool DebugConsole::handleKeyEvent( const KeyEvent& event )
{
	BW_GUARD;
	bool handled = false;

	// Do some checking before we send it to the EditConsole.

	if (this->isCursorShowing() && event.isKeyDown())
	{
		handled = true;

		switch (event.key())
		{
			case KeyCode::KEY_PGDN:
			case KeyCode::KEY_PGUP:
			case KeyCode::KEY_JOY0:
			case KeyCode::KEY_JOY1:
			{
				printf("Page up or down\n");

				float increment =
					event.isAltDown()	? 1000.f :
					event.isCtrlDown()	?  100.f :
					event.isShiftDown()	?   10.f : 1.f;

				if (event.key() == KeyCode::KEY_PGDN ||
					event.key() == KeyCode::KEY_JOY1)
				{
					increment = -increment;
				}

				this->incrementLine( increment );
			}
			break;

			case KeyCode::KEY_ESCAPE:
			case KeyCode::KEY_JOY9:
//			case KeyCode::KEY_JOY2:
				lineEditor_.editString( L"" );
				lineEditor_.cursorPosition( 0 );
				this->hideCursor();
				//watcherDesc_->visible( false );
			break;

			default:
				handled = false;
			break;
		}
	}

	// Give the EditConsole a go.

	if (!handled)
	{
		handled = EditConsole::handleKeyEvent( event );
	}

	// Do some processing if it wasn't handled by the EditConsole.

	if (!handled && event.isKeyDown())
	{
		handled = true;

		switch (event.key())
		{
			case KeyCode::KEY_0:
			case KeyCode::KEY_1:
			case KeyCode::KEY_2:
			case KeyCode::KEY_3:
			case KeyCode::KEY_4:
			case KeyCode::KEY_5:
			case KeyCode::KEY_6:
			case KeyCode::KEY_7:
			case KeyCode::KEY_8:
			case KeyCode::KEY_9:
			{
				int itemNum = 0;

				if (event.key() == KeyCode::KEY_0)
				{
					itemNum = 0;
				}
				else
				{
					// HACK: Assumes KEY_1 to KEY_9 are sequential.
					itemNum = event.key() - KeyCode::KEY_1 + 1;
				}

				itemNum +=
					(event.isShiftDown()? 10 : 0) +
					(event.isCtrlDown()	? 20 : 0) +
					(event.isAltDown()	? 40 : 0);

				this->selectItem( itemNum );
				break;
			}

			case KeyCode::KEY_NUMPADENTER:
			case KeyCode::KEY_RETURN:
//			case KeyCode::KEY_JOY3:
			case KeyCode::KEY_JOY8:
				if (event.isAltDown())
				{
					handled = false;
					break;
				}

				if (event.isShiftDown())
				{
					this->showCursor();
				}
				else
				{
					this->selectItem( highlightItem_ );
				}
				break;

			case KeyCode::KEY_HOME:
				path_ = "";
				highlightItem_ = 0;
				break;

			case KeyCode::KEY_BACKSPACE:
//			case KeyCode::KEY_JOY2:
			case KeyCode::KEY_JOY9:
			{
				if (path_.size() > 0)
				{
					BW::string::size_type pos = path_.find_last_of( "/", path_.size() - 2 );

					if (pos != BW::string::npos)
					{
						path_ = path_.substr( 0, pos + 1 );
					}
					else
					{
						path_ = "";
					}

					highlightItem_ = 0;
				}
				break;
			}
			case KeyCode::KEY_SPACE:
			{
				if (event.isAltDown())
				{
					handled = false;
					break;
				}
				if (!event.isShiftDown())
				{
					this->processSpaceDown( highlightItem_ );
				}

				break;
			}
			case KeyCode::KEY_PGUP:
			case KeyCode::KEY_JOY0:
				highlightItem_ -= event.isShiftDown() ? 10 : 1;
				break;

			case KeyCode::KEY_PGDN:
			case KeyCode::KEY_JOY1:
				highlightItem_ += event.isShiftDown() ? 10 : 1;
				break;
			case KeyCode::KEY_SEMICOLON: // KEY_COLON doesn't seem to work!
				// mimic KEY_COLON with SEMICOLON and check for 'shift' key down
			{
				if ( event.isShiftDown() )
				{
					lineEditor_.editString( L":" );
					lineEditor_.cursorPosition( 1 );
					this->showCursor();
				}
				break;
			}
			case KeyCode::KEY_ADD:
				this->scrollDown();
				break;

			case KeyCode::KEY_NUMPADMINUS:
				this->scrollUp();
				break;

			default:
				handled = false;
				break;
		}
	}

	return handled;
}


/**
 *	This method increments the current value on the edit line.
 *
 *	@param increment		The amount to increment by.
 */
void DebugConsole::incrementLine( float increment )
{
	BW_GUARD;
	BW::string line = bw_wtoutf8( lineEditor_.editString() );

	BW::string::size_type assignmentLocation = line.find_first_of( "=" );

	if (assignmentLocation != BW::string::npos)
	{
		BW::string valueStr = line.substr( assignmentLocation + 1, line.size() );
		BW::string::size_type startPos	= line.find_first_not_of( " " );
		BW::string::size_type endPos		= line.find_last_not_of( " ", assignmentLocation - 1 );
		BW::string label	= line.substr( startPos, endPos - startPos + 1 );

		BW::string path		= path_ + label;

		float oldValue = 0.f;

		BW::istringstream istr( valueStr );
		istr.imbue( Locale::standardC() );
		istr >> oldValue;

		if (!istr.fail())
		{
			float newValue = oldValue + increment;

			BW::ostringstream ostr;
			ostr.imbue( Locale::standardC() );
			ostr << newValue;

			Watcher::rootWatcher().setFromString( NULL, path.c_str(), ostr.str().c_str() );

			BW::string	result;
			BW::string	desc;
			Watcher::Mode mode;

			if (Watcher::rootWatcher().getAsString( NULL, path.c_str(), result, desc, mode ))
			{
				this->edit( label, result );
			}
		}
	}
}

/**
 * This is a simple helper class used by DebugConsole::selectItem
 */
class SelectWatcherVisitor : public WatcherVisitor
{
public:
	SelectWatcherVisitor( int itemNum ) : found_( false ),
		count_( 0 ),
		itemNum_( itemNum )
	{}

	virtual bool visit( Watcher::Mode mode,
		const BW::string & label,
		const BW::string & desc,
		const BW::string & value )
	{
		BW_GUARD;
		if (count_ == itemNum_)
		{
			mode_ = mode;
			label_ = label;
			value_ = value;
			desc_ = desc;
			found_ = true;
		}

		count_++;

		return true;
	}

	bool found_;
	Watcher::Mode mode_;
	BW::string label_;
	BW::string value_;
	BW::string desc_;

	int count_;
	int itemNum_;
};


/**
 *	This method is used to select an item from the current list.
 *
 *	@param itemNum		The item number starting from 0.
 */
void DebugConsole::selectItem( int itemNum )
{
	BW_GUARD;
	if (this->isCursorShowing())
	{
		return;
	}

	SelectWatcherVisitor visitor( itemNum );
	Watcher::rootWatcher().visitChildren( NULL,path_.c_str(), visitor );

	if (visitor.found_)
	{
		switch (visitor.mode_)
		{
			case Watcher::WT_DIRECTORY:
				path_ += visitor.label_;
				path_ += "/";
				highlightItem_ = 0;
				break;

			case Watcher::WT_READ_WRITE:
				this->edit( visitor.label_, visitor.value_ );
				break;

			case Watcher::WT_READ_ONLY:
				// No action.
				break;

			default:
				WARNING_MSG( "DebugConsole::selectItem: Unhandled case\n" );
				break;
		}
	}
}


/**
 *	This method starts editing a value.
 */
void DebugConsole::edit( const BW::string & label, const BW::string & value )
{
	BW_GUARD;
	BW::string line = label + "=" + value;

	lineEditor_.editString( bw_utf8tow( line ).c_str() );
	lineEditor_.cursorPosition( (int)lineEditor_.editString().size() );

	this->showCursor();
}

/**
 *	Switches watcher value to opposite
 */
void DebugConsole::processSpaceDown( int itemNum ) 
{
	BW_GUARD;
	if (this->isCursorShowing())
	{
		return;
	}

	SelectWatcherVisitor visitor( itemNum );
	Watcher::rootWatcher().visitChildren( NULL, path_.c_str(), visitor );

	if (visitor.found_ && visitor.mode_ == Watcher::WT_READ_WRITE)
	{
		// switch bool watcher to opposite value
		if (visitor.value_ == "true")
		{
			this->edit( visitor.label_, "false" );
		}
		else if (visitor.value_ == "false")
		{
			this->edit( visitor.label_, "true" );
		}
		EditConsole::handleKeyEvent(
			KeyEvent::make( KeyCode::KEY_RETURN, true, 0, Vector2::zero()) );
	}
}
#endif /* ENABLE_WATCHERS */


// -----------------------------------------------------------------------------
// Section: PythonConsole
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
PythonConsole::PythonConsole() :
	isMultiline_( false ),
	autoCompIndex_( 0 ),
	pHookedWriter_( NULL )
{
	BW_GUARD;
}

PythonConsole::~PythonConsole()
{
	BW_GUARD;
	if (pHookedWriter_ != NULL)
	{
		ScriptOutputHook::unhookScriptOutput( pHookedWriter_, this );
	}
}


/// Initialise this console
bool PythonConsole::init()
{
	BW_GUARD;
	bool succeeded = EditConsole::init();

	if (succeeded)
	{
		this->setConsoleColour( 0xFFFFFFFF );
		this->lineEditor_.advancedEditing( true );
		this->setScrolling( true );

		this->prompt( L">>> " );

		char	initString[512];
		bw_snprintf( initString, sizeof(initString), "Python %s on %s\n",
			Py_GetVersion(), Py_GetPlatform() );
		this->setCursor( 0, 0 );
		this->print( initString );
	}

	pHookedWriter_ = ScriptOutputHook::hookScriptOutput( this );
	MF_ASSERT( pHookedWriter_ != NULL );

	return succeeded;
}

/// Update this console
void PythonConsole::update()
{
	BW_GUARD;
	this->editLine( this->cursorY() );
	EditConsole::update();
}

/// Draw this console
void PythonConsole::draw( float dTime )
{
	BW_GUARD;
	this->showCursor();

	this->EditConsole::draw( dTime );
}

/**
 *	Retrieves the current command history list.
 */
const LineEditor::StringVector PythonConsole::history() const
{
	BW_GUARD;
	return this->EditConsole::lineEditor_.history();
}

/**
 *	Replaces the command history list.
 */
void PythonConsole::setHistory(const LineEditor::StringVector & history)
{
	BW_GUARD;
	this->EditConsole::lineEditor_.setHistory(history);
}


/// Handle a key event
bool PythonConsole::handleKeyEvent( const KeyEvent& event )
{
	BW_GUARD;
	bool handled = false;

	switch (event.key())
	{
	// deactivate console
	case KeyCode::KEY_Z:
		if (event.isCtrlDown() && event.isKeyDown())
		{
			ConsoleManager::instance().deactivate();
			handled = true;
		}
		break;

	// try auto completion
	case KeyCode::KEY_TAB:
		if (event.isKeyDown() && !event.isAltDown())
		{
			if (this->autoCompIndex_ == AUTO_INDEX_RESET)
			{
				this->completeLine( AUTO_INDEX_RESET );
				this->autoCompIndex_ = 0;
			}
			else
			{
				// cycle auto completion index
				this->autoCompIndex_ += event.isShiftDown() ? -1 : +1;
				this->completeLine( this->autoCompIndex_ );
			}
			handled = true;
		}
		break;
	case KeyCode::KEY_V:
		if (event.isCtrlDown() && event.isKeyDown())
		{
			BW::wstring fromClipboard;
			if ( OpenClipboard(NULL) ) 
			{
				HANDLE hData = GetClipboardData( CF_UNICODETEXT );
				if (hData != NULL)
				{
					wchar_t * buffer = (wchar_t*)GlobalLock( hData );
					fromClipboard = buffer;
					GlobalUnlock( hData );
				}
				CloseClipboard();
			}
			for (size_t i = 0; i  < fromClipboard.size(); i++)
			{
				if (fromClipboard[i] == L'\r' && fromClipboard[i+1] == L'\n')
				{
					fromClipboard[i] = L';';
					fromClipboard[i+1] = L' ';
				}
				else if (fromClipboard[i] == L'\n')
				{
					fromClipboard[i] = L';';
				}
			}
			lineEditor_.insertInEditString( fromClipboard );
			handled = true;
		}
		break;
	default:
		break;
	}

	if (!handled)
	{
		handled = this->EditConsole::handleKeyEvent( event );
		if (handled)
		{
			this->autoCompIndex_ = AUTO_INDEX_RESET;
		}
	}

	return handled;
}


/// Process a line
void PythonConsole::processLine( const BW::wstring & line )
{
	BW_GUARD;
	if (line.length() == 0)
	{
		if (!isMultiline_)
		{
			ConsoleManager::instance().deactivate();
			return;
		}

		isMultiline_ = false;	// no more multiline => run existing string
		this->prompt( L">>> " );
	}
	else
	{
		this->print( "\n" );

		BW::wstring subbed = bw_utf8tow( PythonInputSubstituter::substitute(
			bw_wtoutf8( line ) ) );
		if (subbed.size() == 0)
			return;

		if (subbed.length() != 0 && subbed[ subbed.length() - 1] == ':')
		{
			isMultiline_ = true;
			this->prompt( L"... " );
		}

		multiline_ += subbed + L"\n";
	}

	if (!isMultiline_)                                                  
	{
		// don't print warnings during script execution.
		BWResource::WatchAccessFromCallingThreadHolder fileAccessHolder( false );

		PyErr_Clear();
		PyObject * res = Script::runString( const_cast<char *>( bw_wtoutf8( multiline_ ).c_str() ), true );

		if (PyErr_Occurred())
		{
			PyErr_PrintEx(0);
			PyErr_Clear();
		}

		if (Py_FlushLine())
			PyErr_Clear();

		multiline_ = L"";
	}
}


/// Try to complete current line
void PythonConsole::completeLine( int compIndex )
{
	BW_GUARD;
	int useIndex;
	if (compIndex == AUTO_INDEX_RESET)
	{
		// get fresh input from lineEditor
		BW::wstring fullLine = lineEditor_.editString();
		int cPos = lineEditor_.cursorPosition();
		this->lastLineStart_  = fullLine.substr( 0, cPos );
		this->lastLineExcess_ = fullLine.substr( cPos, fullLine.length() - cPos );
		useIndex = 0;
	}
	else
	{
		// use saved state
		useIndex = compIndex;
	}

	BW::wstring subbedLine = bw_utf8tow( PythonInputSubstituter::substitute(
		bw_wtoutf8( this->lastLineStart_ ) ) );
	if (subbedLine.size() == 0)
		return;

	StringPair headAndExp   = splitHeadAndExpression( subbedLine );
	StringPair objAndAttr   = splitObjAndAttr( headAndExp.second );
	StringVector attributes = getAttributes( objAndAttr.first.c_str() );

	BW::wstring partialAttribute = objAndAttr.second.c_str();
	StringVector filtered = filterAttributes( attributes, partialAttribute );
	std::sort( filtered.begin(), filtered.end() );

	BW::wstring completed;

	// ... single match. Return completed line
	if (filtered.size() == 1)
	{
		completed = buildCompletedLine(
				headAndExp.first, objAndAttr.first, filtered[0] );
	}
	// multiple match. Cycle
	else if (filtered.size() > 1)
	{
		// uncomment to print all candidates to console and
		// complete up to a common prefix (bash like behaviour)
		/*
		this->print( "\n" );
		printAttributes( *this, filtered );
		BW::string commonStart = getCommonPrefix( filtered, partialAttribute );
		*/

		// make sure useIndex is never negative (the
		// mod trick does't work for negative values)
		size_t index = ( useIndex % filtered.size() + filtered.size() ) % filtered.size();
		completed = buildCompletedLine(
				headAndExp.first, objAndAttr.first,
				filtered[ index ] );
		++useIndex;
	}
	// no match
	else
	{
		completed = this->lastLineStart_;
	}

	// update lineEditor
	lineEditor_.editString( completed + this->lastLineExcess_ );
	lineEditor_.cursorPosition( static_cast<int>(completed.length()) );
}


/**
 *	This method receives stdout or stderr output from the scripting system
 *
 *	@param output	The BW::string that was output.
 *	@param isStderr	True if the output was to stderr, false otherwise.
 */
void PythonConsole::onScriptOutput( const BW::string & output,
	bool /* isStderr */ )
{
	BW_GUARD;

	this->print( output );
}


/**
 *	This method is notified when the ScriptOutputWriter we're hooked to is
 *	going to be destroyed.
 *
 *	After this call returns, pOwner is no longer a valid pointer and should
 *	not be dereferenced.
 *
 *	@param pOwner	The ScriptOutputWriter that is to be destroyed.
 */
void PythonConsole::onOutputWriterDestroyed( ScriptOutputWriter * owner )
{
	BW_GUARD;

	MF_ASSERT( owner == pHookedWriter_ );
	pHookedWriter_ = NULL;
}


// -----------------------------------------------------------------------------
// Section: TextConsole
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
TextConsole::TextConsole( DataSectionPtr pSection ) :
	pSection_( pSection ),
	pageNum_( 0 )
{
}


/**
 *	This method is called when this console is activated.
 */
void TextConsole::activate( bool isReactivate )
{
	BW_GUARD;
	if (isReactivate)
	{
		pageNum_++;
	}
	else
	{
		pageNum_ = 0;
	}

	if (pageNum_ < this->numPages())
	{
		this->refresh();
	}
	else
	{
		ConsoleManager::instance().deactivate();
	}
}


/**
 *	This method returns the number of pages associated with this console.
 */
int TextConsole::numPages() const
{
	BW_GUARD;
	return pSection_ ? pSection_->countChildren() : 0;
}


/**
 *	This method refreshes the information displayed by this console.
 */
void TextConsole::refresh()
{
	BW_GUARD;
	this->clear();

	DataSectionPtr pPage = pSection_->openChild( pageNum_ );

	// Read the colour associated with the page.
	this->setConsoleColour(
		Colour::getUint32( pPage->readVector3( "colour",
				Vector3( 0xff, 0xff, 0xff ) ) ) | 0xff000000 );

	DataSection::iterator iter = pPage->begin();

	while (iter != pPage->end())
	{
		if ((*iter)->sectionName() == "line")
		{
			// Read the colour associated with a line.
			DataSectionPtr pColour = (*iter)->openSection( "colour" );

			if (pColour)
			{
				this->lineColourOverride( this->cursorY(),
					Colour::getUint32( pColour->asVector3() ) | 0xff000000 );
			}
			else
			{
				this->lineColourOverride( this->cursorY() );
			}

			// Read the line's string.
			this->print( (*iter)->asWideString() );
			this->print( L"\n" );
		}

		++iter;
	}
}


/**
 *	This method handles keyboard events for this console.
 */
bool TextConsole::handleKeyEvent( const KeyEvent & event )
{
	BW_GUARD;
	bool handled = false;

	if (event.isKeyDown())
	{
		handled = true;

		switch( event.key() )
		{
		case KeyCode::KEY_ADD:
			this->scrollDown();
			break;

		case KeyCode::KEY_NUMPADMINUS:
			this->scrollUp();
			break;

		case KeyCode::KEY_HOME:
			this->scrollOffset( 0 );
			break;

		default:
			handled = false;
			break;
		}
	}

	return handled;
}


/**
 *	This method handles the scroll event that comes from the XConsole.
 */
void TextConsole::onScroll()
{
	BW_GUARD;
	this->refresh();
}


// Helper functions
namespace { // anonymous

/**
 *	Splits given console line into two parts: head and expression.
 *	The expression is the substring that currently matters in the
 *	context of code completion. The head is anything preceeding it.
 *	Return result as a std::pair of strings.
 *
 *	For example, in the line:
 *		"if BigWorld.entities[ BigWorld.player().targetI"
 *	Head would be:
 *		"if BigWorld.entities[ "
 *	And expression:
 *		"BigWorld.player().targetI"
 */
StringPair splitHeadAndExpression( const BW::wstring & line )
{
	BW_GUARD;
	BW::wstring::const_iterator sepBegin = CONSOLE_SEPARATORS.begin();
	BW::wstring::const_iterator sepEnd   = CONSOLE_SEPARATORS.end();

	int parenthesisCount = 0;
	int bracketsCount    = 0;
	BW::wstring::const_reverse_iterator lineIt  = line.rbegin();
	BW::wstring::const_reverse_iterator lineEnd = line.rend();
	while (lineIt != lineEnd)
	{
		if (std::find( sepBegin, sepEnd, *lineIt ) != sepEnd)
		{
			break;
		}
		else if (*lineIt == L'(')
		{
			if (parenthesisCount == 0)
			{
				break;
			}
			else
			{
				--parenthesisCount;
			}
		}
		else if (*lineIt == L')')
		{
			++parenthesisCount;
		}
		else if (*lineIt == L'[')
		{
			if (bracketsCount == 0)
			{
				break;
			}
			else
			{
				--bracketsCount;
			}
		}
		else if (*lineIt == L']')
		{
			++bracketsCount;
		}
		++lineIt;
	}

	size_t pos = std::distance( lineIt, line.rend() );
	return std::make_pair(
			line.substr( 0, pos ),
			line.substr( pos, line.length() - pos ) );
}


/**
 *	Splits given expression line into two parts: object and attribute.
 *	The object is the substring where potential matching attributes are
 *	extracted from. The attribute is anything succeding the last dot in
 *	the expression. Return result as a std::pair of strings.
 *
 *	For example, in the expresttion:
 *		"BigWorld.player().targetI"
 *	Expression would be:
 *		"BigWorld.player()"
 *	And attribute:
 *		"targetI"
 */
StringPair splitObjAndAttr( const BW::wstring & expression )
{
	BW_GUARD;
	// find lastest occurent of a point
	BW::wstring::size_type lastDotPos = expression.find_last_of( L'.' );

	// return result
	if (lastDotPos != -1)
	{
		// context + partial attribute
		return std::make_pair(
			expression.substr( 0, lastDotPos ),
			expression.substr(
					lastDotPos + 1,
					expression.length() - lastDotPos - 1 ) );
	}
	else
	{
		// partial attribute only
		return std::make_pair(
				L"", expression.substr( 0, expression.length() ) );
	}
}

/**
 *	Extracts Python attributes from given object. Attributes are
 *	assumed to be the list of keys in object's dir(). If object
 *	is empty, return globals and all Python 2.4 keywords.
 */
StringVector getAttributes( const BW::wstring & object )
{
	BW_GUARD;
	BW::wstring out;
	if (object != L"")
	{
		// get attributes from object's dictionary
		out = L"dir( "+object+L" )";
	}
	else
	{
		// empty object get globals + keywords.
		// See keyword.py in Python's distribution
		out = L"globals().keys() + ";
		out += L"['and', 'assert', 'break', 'class', 'continue', 'def', ";
		out += L" 'del', 'elif', 'else', 'except', 'exec', 'finally', 'for', ";
		out += L" 'from', 'global', 'if', 'import', 'in', 'is', 'lambda', ";
		out += L" 'not', 'or', 'pass', 'print', 'raise', 'return', 'try', ";
		out += L" 'while', 'yield', 'None', 'as' ] ";
	}

	PyObject * list = Script::runString( bw_wtoutf8( out ).c_str(), false );
	if (PyErr_Occurred())
	{
		PyErr_Clear();
	}

	StringVector attributes;
	if (list != NULL)
	{
		Py_ssize_t n = PyList_Size( list );
		if (n > 0) {
			for (int i = 0; i < n; i++) {
				PyObject * entry = PyList_GetItem( list, i );
				attributes.push_back( bw_utf8tow( PyString_AS_STRING( entry ) ) );
			}
		}
	}
	return attributes;
}


/**
 *	Given a list of attribute names, return the
 *	ones that starts with the given string.
 */
StringVector filterAttributes(
		const StringVector &attributes,
		const BW::wstring &startsWith )
{
	BW_GUARD;
	StringVector result;

	StringVector::const_iterator attIt  = attributes.begin();
	StringVector::const_iterator attEnd = attributes.end();
	while (attIt != attEnd)
	{
		if (attIt->find( startsWith ) == 0)
		{
			result.push_back( *attIt );
		}
		++attIt;
	}
	return result;
}


/**
 *	Builds the completed line.
 */
BW::wstring buildCompletedLine(
	const BW::wstring &head,
	const BW::wstring &object,
	const BW::wstring &attribute )
{
	BW_GUARD;
	return object != L""
		? head + object + L"." + attribute
		: head + attribute;
}


/**
 *	Extracts a prefix common to all attributes
 *	name in the given list, if any.
 */
BW::wstring getCommonPrefix(
		const StringVector & filtered,
		const BW::wstring  & knownStart )
{
	BW_GUARD;
	MF_ASSERT( filtered.size() > 1 )

	bool failed = false;
	BW::wstring result = knownStart;
	for (size_t i=knownStart.size(); i < filtered[0].length(); ++i)
	{
		const wchar_t & testChar = filtered[0][i];
		StringVector::const_iterator filterIt  = filtered.begin();
		StringVector::const_iterator filterEnd = filtered.end();
		while (filterIt != filterEnd)
		{
			if ((*filterIt)[i] != testChar)
			{
				failed = true;
				break;
			}
			++filterIt;
		}
		if (failed)
		{
			break;
		}
		else
		{
			result += testChar;
		}
	}
	return result;
}


/**
 *	Prints the list of attribute names into the given console.
 */
void printAttributes( XConsole &console, const StringVector &attributes )
{
	BW_GUARD;
	StringVector::const_iterator filterIt  = attributes.begin();
	StringVector::const_iterator filterEnd = attributes.end();
	BW::wstring attribListLine;
	while (filterIt != filterEnd)
	{
		if (attribListLine.length() + filterIt->length() > 80)
		{
			console.print( attribListLine + L"\n" );
			attribListLine = L"";
		}
		attribListLine += L"  " + *filterIt;
		++filterIt;
	}
	if (attribListLine.length() > 0)
	{
		console.print( attribListLine + L"\n" );
	}
}

} // namespace anonymous

BW_END_NAMESPACE

// console.cpp
#endif // ENABLE_CONSOLES
