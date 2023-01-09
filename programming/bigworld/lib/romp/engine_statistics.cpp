#include "pch.hpp"
#if ENABLE_CONSOLES
#include "engine_statistics.hpp"

#include "resource_manager_stats.hpp"

#include "font_manager.hpp"

#include "cstdmf/allocator.hpp"
#include "cstdmf/profile.hpp"

#include "moo/render_context.hpp"
#include "moo/texture_manager.hpp"
#include "moo/vertex_formats.hpp"
#include "moo/visual.hpp"
#include "moo/gpu_profiler.hpp"

#include "chunk/chunk.hpp"

#include "moo/geometrics.hpp"
#include "romp/profile_graph.hpp"

#include <strstream>
#include <fstream>

DECLARE_DEBUG_COMPONENT2( "Romp", 0 )


BW_BEGIN_NAMESPACE

#ifndef CODE_INLINE
#include "engine_statistics.ipp"
#endif

EngineStatistics EngineStatistics::instance_;


// -----------------------------------------------------------------------------
// Section: Construction/Destruction
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
EngineStatistics::EngineStatistics() : lastFrameTime_( 1.f ),
	timeToNextUpdate_( 0.f )
{}


/**
 *	Destructor.
 */
EngineStatistics::~EngineStatistics()
{}


// -----------------------------------------------------------------------------
// Section: General
// -----------------------------------------------------------------------------

/**
 *	Utility struct for keeping track of where we are in the DogWatch tree
 */
struct TreeTrack
{
	/// Constructor.
	TreeTrack(
		DogWatchManager::iterator & iA,
		DogWatchManager::iterator & endA,
		uint64 totA ) : i( iA ), end( endA ), tot( totA ), acc( 0 )
	{ }

	DogWatchManager::iterator	i;		///< @todo Comment
	DogWatchManager::iterator	end;	///< @todo Comment
	uint64						tot;	///< @todo Comment
	uint64						acc;	///< @todo Comment
};


/**
 *	Utility class for keeping track of a graph, and drawing it
 */
class EngineStatisticsGraphTrack
{
public:
	/**
	 *	Constructor.
	 *
	 *	@todo Comment.
	 */
	EngineStatisticsGraphTrack( DogWatchManager::iterator & stat,
			DogWatchManager::iterator & frame,
			int index ) :
		stat_( stat ), frame_( frame ), index_( index ) {}

	void draw();

	/**
	 *	This method returns the colour that the corresponding graph will be draw
	 *	in.
	 */
	uint32 colour() const
	{
		return s_colours[ index_ ];
	}

private:
	DogWatchManager::iterator	stat_;
	DogWatchManager::iterator	frame_;

	int		index_;

	const static struct Colours : public BW::vector<uint32>
	{
		Colours()
		{
			static uint32 ls_colours[] =
			{
				0xffff0000,
				0xff00ff00,
				0xff0000ff,
				0xff00ffff,
				0xffff00ff,
				0xff888800,
				0xffaaaaaa
			};

			this->assign( ls_colours,
				&ls_colours[sizeof(ls_colours)/sizeof(*ls_colours)] );
		}

		const uint32 & operator[]( int i ) const
		{
			return (*(BW::vector<uint32>*)this)[ i % this->size() ];
		}
	} s_colours;

};

const EngineStatisticsGraphTrack::Colours	EngineStatisticsGraphTrack::s_colours;

/**
 *	This method draws the graph of the given statistic.
 */
void EngineStatisticsGraphTrack::draw()
{
	BW_GUARD;
	static BW::vector< Moo::VertexTL > tlvv;
	//static BW::vector< uint16 > indices;

	tlvv.erase( tlvv.begin(), tlvv.end() );
	//indices.erase( indices.begin(), indices.end() );


	double period = stampsPerSecondD() * 2.0;
	double range = stampsPerSecondD() / 20.0;

	Moo::VertexTL tlv;
	tlv.pos_.z = 0;
	tlv.pos_.w = 1;
	tlv.colour_ = this->colour();

	uint64	doneTime = 0;
	uint64	uperiod = uint64(period);
	for (int i = 1; i < 120 && doneTime < uperiod; i++)
	{
		tlv.pos_.x = float( double(int64(doneTime)) / period );
		tlv.pos_.x *= Moo::rc().screenWidth();
		tlv.pos_.x = Moo::rc().screenWidth() - tlv.pos_.x;

		tlv.pos_.y = float( double(int64(stat_.value( i ))) / range );
		tlv.pos_.y *= -Moo::rc().screenHeight();
		tlv.pos_.y += Moo::rc().screenHeight();

		tlvv.push_back( tlv );

		doneTime += frame_.value( i );
	}

	if( tlvv.size() > 1 )
	{

		Moo::Material::setVertexColour();

		Moo::rc().setRenderState( D3DRS_ZENABLE, TRUE );
		Moo::rc().setRenderState( D3DRS_ZFUNC, D3DCMP_ALWAYS );

		Moo::rc().setVertexShader( NULL );
		Moo::rc().setFVF( D3DFVF_XYZRHW | D3DFVF_DIFFUSE );

		Moo::rc().drawPrimitiveUP( D3DPT_LINESTRIP, (uint)tlvv.size()-1,
			&tlvv.front(), sizeof( Moo::VertexTL ) );
	}

}

/**
 *	This is a static helper function for App::displayStatistics.
 */
static void displayOneStatistic( XConsole & console, const BW::string & label,
	int indent, uint64 value, uint64 overall )
{
	BW_GUARD;
	static char * spaceString = "                         ";

	std::strstream	stream;

	stream << label;
	size_t extraSpace = strlen(spaceString) - (label.length() + indent);
	if (extraSpace > 0)
	{
		stream << &spaceString[strlen(spaceString) - extraSpace];
	}

	char	percent[16];
	bw_snprintf( percent, sizeof(percent), "%4.1f%%", value * 100.f / overall );
	stream << percent << " (" << NiceTime( value ) << ")";
	stream << std::endl << std::ends;
	console.setCursor( indent, console.cursorY() );
	console.print( stream.str() );

	stream.freeze( 0 );
}


/**
 *	This method displays the statistics associated with the frame on the input
 *	console.
 *
 *	@param console	The console on which the statistics should be displayed.
 */
void EngineStatistics::displayStatistics( XConsole & console )
{	
	BW_GUARD;
//	PROFILER_SCOPED(displayStatistics);

	static const size_t STR_SIZE = 64;
	char statString[STR_SIZE];
	statString[STR_SIZE - 1] = 0;

	console.setCursor( 0, 0 );
	console.print( "Realtime Profiling Console\n" );

#if ENABLE_PROFILER
	// on error, overwrite the title line
	BW::string errorString = g_profiler.getErrorString();
	if (!errorString.empty())
	{
		console.setCursor( 0, 0 );
		console.lineColourOverride( console.cursorY(), 0xffff0000 );
		console.lineColourOverride( console.cursorY() + 1, 0xffff0000 );
		console.print( errorString.c_str() );
	}
#endif


	float framesPerSecond = lastFrameTime_ > 0.f ? 1.f/lastFrameTime_ : 0.f;

	// Frames Per Second
	_snprintf( statString, STR_SIZE - 1, "Fps      : %2.2f (%2.2fms per frame)\n", framesPerSecond, lastFrameTime_*1000.0f );
	console.print( statString );

#if ENABLE_PROFILER

	int pMode = ((StatisticsConsole*)&console)->profileMode_;
	bool inclusive = ((StatisticsConsole*)&console)->profileModeInclusive_;
	if (pMode==0)
	{
		g_profiler.setProfileMode( Profiler::PROFILE_OFF, inclusive );
#endif

		// Triangles

		const Moo::DrawcallProfilingData& pd = Moo::rc().lastFrameProfilingData();
		uint32 primsPerDrawcall = 0;
		if (pd.nDrawcalls_ != 0)
			primsPerDrawcall = pd.nPrimitives_ / pd.nDrawcalls_;

		_snprintf( statString, STR_SIZE - 1, "DrawCalls: %d Primitives: %d Primitives/DrawCall: %d\n",
			pd.nDrawcalls_, pd.nPrimitives_, primsPerDrawcall );

		console.print( statString );

		// Texture memory
		bw_snprintf( statString, STR_SIZE - 1, "Memory:\n"
			" Texture (Total):                   %6d KB\n",
			Moo::TextureManager::instance()->textureMemoryUsed() / 1024 );
		console.print( statString );
		_snprintf( statString, STR_SIZE - 1,   " Texture (Frame):                   %6d KB\n",
			Moo::ManagedTexture::totalFrameTexmem_ / 1024 );
		console.print( statString );

#if ENABLE_MEMORY_DEBUG
		// Heap memory allocations
		BW::Allocator::AllocationStats heapStats;
		BW::Allocator::readAllocationStats( heapStats );

		bw_snprintf( statString, STR_SIZE - 1, " Heap Memory/Allocations (Current): %6" PRIzu " KB /%6" PRIzu " allocs\n",
			heapStats.currentBytes_/1024, heapStats.currentAllocs_ );
		console.print( statString );
#endif // ENABLE_MEMORY_DEBUG

		bw_snprintf( statString, STR_SIZE - 1, "Chunks/Items:\n");
		console.print( statString );
		bw_snprintf( statString, STR_SIZE - 1, " Chunk Instances (Current): %d\n",
			Chunk::s_instanceCount_ );
		console.print( statString );
		bw_snprintf( statString, STR_SIZE - 1, " Chunk Instances (Peak):    %d\n",
			Chunk::s_instanceCountPeak_ );
		console.print( statString );

		bw_snprintf( statString, STR_SIZE - 1, " Item Instances  (Current): %d\n",
			ChunkItemBase::s_instanceCount_ );
		console.print( statString );

		bw_snprintf( statString, STR_SIZE - 1, " Item Instances  (Peak):    %d\n",
			ChunkItemBase::s_instanceCountPeak_ );
		console.print( statString );

#if ENABLE_NVIDIA_PERFKIT
		console.print( "\n-----------------------------------------------\n");
		console.print( "NVidia Perfkit Statistics\n" );
		console.print( "-----------------------------------------------\n");
		Moo::GPUProfiler::PerfkitStatsArray pkStats;
		Moo::GPUProfiler::instance().perfkitGetStats( pkStats );
		if (pkStats.size() == 0)
		{
			console.print("Not available\n");
		}
		else
		{
			for (size_t i = 0; i < pkStats.size(); ++i)
			{
				bw_snprintf( statString, STR_SIZE - 1, " %s:\t\t%d\n", pkStats[i].first, pkStats[i].second );
				console.print( statString );
			}
		}
		console.print( "-----------------------------------------------\n");
#endif // NVIDIA_PERFKIT

		dogWatchDisplay(console);

		console.print(
			"\n"
			"---------------------------------------------\n");
#if ENABLE_PROFILER
	}
	else if (pMode == Profiler::GRAPHS)
	{
		g_profiler.setProfileMode((Profiler::ProfileMode)pMode, inclusive);

		static ProfileGraph* graph=NULL;
		if(!graph)
		{
			Vector2 topLeft(-0.9f, 0.9f);
			Vector2 bottomRight(0.8f,-0.3f);

			graph = new ProfileGraph(topLeft, bottomRight);
			graph->setNumPoints(Profiler::HISTORY_LENGTH);
		}

		for(int g = 0; g < Profiler::MAXIMUM_NUMBER_OF_THREADS; g++)
		{
			const char* threadName = g_profiler.getThreadName(g);
			if(!threadName)
				continue;

			int offset=0;
			float* data = g_profiler.getGraph(g, &offset);
			graph->drawGraph(threadName, data, offset, console);

		}
		graph->drawBorder(console);
		
		console.setCursor(0,38);
		console.print(
			"\n"
			"---------------------------------------------\n"
			"Press 'F' to freeze/unfreeze the graph display\n");
	}
	else if (pMode == Profiler::HIERARCHICAL)
	{

		BW::vector<Profiler::ProfileLine> profileOutput_;
		g_profiler.setProfileMode((Profiler::ProfileMode)pMode, inclusive);
		g_profiler.getStatistics(&profileOutput_);
		int i=0;
		int lineNumber=0;
		for(BW::vector<Profiler::ProfileLine>::iterator it = profileOutput_.begin();it !=profileOutput_.end(); ++it)
		{
			uint32 state = (uint32)(*it).colour_;
			Moo::Colour colour = 0xffffff00; // yellow
			if(state<=0x7)
			{
				if(state&HierEntry::GRAPHED)
				{
					colour = ProfileGraph::getColour(i);
					i++;
				}
				if(state&HierEntry::HIGHLIGHTED)
				{
					colour = 0xffffffff; // white
					lineNumber = console.cursorY()+1;
				}

			}
			else
				colour = state;
			console.lineColourOverride(console.cursorY(),colour);
			console.print((*it).text_);

		}
		if(lineNumber - console.scrollOffset()>=console.visibleHeight())
			console.scrollDown();
		if(lineNumber - console.scrollOffset()<5)
			console.scrollUp();

		static ProfileGraph* hgraph=NULL;
		if(!hgraph)
		{
			Vector2 topLeft(-0.0f, 0.9f);
			Vector2 bottomRight(0.8f,-0.3f);

			hgraph = new ProfileGraph(topLeft, bottomRight);
			hgraph->setNumPoints(256);
		}
		BW::vector<HierEntry*> &entries = g_profiler.getHierGraphs();

		for(uint g=0;g<entries.size();g++)
		{
			HierEntry* e = entries[g];

			int offset=0;
			if(e->timeArray_)
				hgraph->drawGraph(e->name_,e->timeArray_, g_profiler.getFrameNumber(), console);

		}
		hgraph->drawBorder(console);
		console.print(
			"\n"
			"-----------------------------------------------\n"
			"Navigate with 'Page Up'/'Page Down' or [ ]\n"
			"'+'/'-' to scroll up/down\n"
			"Expand or close '...' entries with 'Return'\n"
			"'G' to Toggle Graph\n"
			"'SHIFT G' to toggle all graphs under that entry\n");
	}
	else  // new profile modes (Bar graphs and Thread graph view)
	{
		BW::vector< Profiler::ProfileLine > profileOutput_;
		g_profiler.setProfileMode( (Profiler::ProfileMode)pMode, inclusive );
		g_profiler.getStatistics( &profileOutput_ );
		for (BW::vector< Profiler::ProfileLine >::iterator it 
				= profileOutput_.begin(); it != profileOutput_.end(); ++it)
		{
			console.lineColourOverride( console.cursorY(), (*it).colour_ );
			console.print( (*it).text_ );
		}

		BW::vector< Profiler::ProfileVertex > * verts = NULL;
		int threadNum = 0;
		Moo::rc().setPixelShader( NULL );
		Moo::rc().setVertexShader( NULL );
		Moo::rc().setFVF( D3DFVF_XYZRHW|D3DFVF_DIFFUSE );

		// lock the mesh mutex so that we aren't writing to the mesh as we draw it.
		SimpleMutexHolder meshMutex( g_profiler.getMeshMutex() );

		for (int t = 0; t < Profiler::MAXIMUM_NUMBER_OF_THREADS; t++)
		{
			verts = g_profiler.getDisplayMesh(t);

			if(!verts)
			{
				continue;
			}

			uint32 lockIndex = 0;
			if (verts->size())
			{
				// scroll the bars up with the text
				for (size_t v=0; v<verts->size(); v++)
				{
					verts->at(v).y -= console.scrollOffset() * 17;
				}

				//DynamicVertexBuffer
				typedef Moo::VertexTL VertexType;
				int vertexSize = sizeof(VertexType);
				Moo::DynamicVertexBufferBase2& vb =
					Moo::DynamicVertexBufferBase2::instance( vertexSize );
				if ( vb.lockAndLoad( 
					reinterpret_cast<VertexType*>(&verts->front()), 
					static_cast<uint32>(verts->size()), lockIndex ) &&
					SUCCEEDED(vb.set( 0 )) )
				{
					HRESULT res = Moo::rc().drawPrimitive( 
						D3DPT_TRIANGLESTRIP, lockIndex, (uint)verts->size()-2 );
					//Reset stream
					vb.unset( 0 );
				}
			}
		}
		console.print(
			"\n"
			"-----------------------------------------------\n"
			"'0' - '9' to toggle thread displays\n"
			"'F' to freeze display and dump profile data\n"
			"'I' to toggle inclusive/exclusive timing information\n"
#if ENABLE_HITCH_DETECTION
			"'E' to toggle hitch detection\n"
#endif // ENABLE_HITCH_DETECTION
			"'B' to toggle the profile bar graphs\n");
	}
#endif // ENABLE_PROFILER
	console.print(
		"'P' to step through float profile modes\n"
		"'H' for hierarchical profiling\n"
		"'T' for thread graphing\n"
		"','/'.' to change the current filter\n"
		"'M' to return to this main screen.\n");

#if ENABLE_PROFILER
	g_profiler.freeze(((StatisticsConsole*)&console)->profileModeFreeze_, g_profiler.hitchFreeze() == false);
#endif // ENABLE_PROFILER
	// DirectX Resource Manager statistics	
	if (ResourceManagerStats::instance().enabled())
	{
		ResourceManagerStats::instance().displayStatistics( console );
	}
}

void EngineStatistics::dogWatchDisplay(XConsole& console)
{
	// DogWatches

	// we use flags as:
	// 1: children visible
	// 2: graphed
	// 4: selected


	// first figure out how many frames ago 1s was
	DogWatchManager::iterator fIter = DogWatchManager::instance().begin();
	uint64	fAcc = 0;
	int		period;
	for (period = 1; period < 60 && fAcc < stampsPerSecond()/2; period++)
	{
		fAcc += fIter.value( period );
	}

	uint64		overall = max( uint64(1), fIter.average( period, 1 ) );
//	uint64		average = fIter.average( period, 1 );
//	uint64		overall = average > 1 ? average : 1;

	double lastFrameTime = double(fIter.value( 1 )) / stampsPerSecondD();

	int	numGraphs = 0;
	int selectedLine = -1;
	int headerLines = console.cursorY();

	// now print out the tree
	BW::vector<TreeTrack>	stack;

	DogWatchManager::iterator dwmBegin = DogWatchManager::instance().begin();
	DogWatchManager::iterator dwmEnd = DogWatchManager::instance().end();

	TreeTrack treeTrack( dwmBegin, dwmEnd, (uint64)0 );

	stack.push_back( treeTrack );

	while (!stack.empty())
	{
		int iIndex = static_cast<int>(stack.size())-1;

		// have we come to the end of this lot?
		TreeTrack	&i = stack.back();
		if (i.i == i.end)
		{
			if (i.acc != 0 && i.tot != 0)
			{
				if (i.acc > i.tot) i.acc = i.tot;
				displayOneStatistic( console, "<Remainder>", (int)(stack.size())-1,
					i.tot - i.acc, overall );
			}

			stack.pop_back();

			continue;
		}

		bool	setColour = false;

		// change our colour if we're selected
		if (i.i.flags() & 2)
		{		// and draw a graph if we're doing that
			EngineStatisticsGraphTrack graph( i.i, fIter, numGraphs++ );
			graph.draw();

			console.lineColourOverride( console.cursorY(),
				graph.colour() );
			setColour = true;
		}

		if (i.i.flags() & 4)
		{
			selectedLine = console.cursorY();
			console.lineColourOverride( console.cursorY(), 0xFFFFFFFFU );
			setColour = true;
		}

		if (!setColour)
		{
			console.lineColourOverride( console.cursorY() );
		}

		// figure out what we're going to call it
		BW::string	label( i.i.name() );
		if (!(i.i.flags() & 1) && i.i.begin() != i.i.end())
		{
			label.append( "..." );
		}

		// ok, print out i.i then
		uint64	val = i.i.average( period, 1 );
		displayOneStatistic( console, label, (int)(stack.size())-1,
			val, overall );
		i.acc += val;

		// do we draw our children?
		if (i.i.flags() & 1)
		{
			// fix up the stack
			DogWatchManager::iterator iiBegin =
				i.i.begin();
			DogWatchManager::iterator iiEnd =
				i.i.end();

			stack.push_back( TreeTrack( iiBegin, iiEnd, (uint64)val ) );
		}

		// move on to the next one
		//  re-get reference after vector modification!
		++(stack[iIndex].i);
	}

	// scroll the console if the selection is off screen
	const int scro = console.scrollOffset();
	if (selectedLine <= scro + headerLines)
		console.scrollUp();
	if (selectedLine >= scro + console.visibleHeight()-2) // extra for frame remainder
		console.scrollDown();
}


/**
 *	This method updates the engine statistics given that the input number of
 *	seconds has elapsed.
 */
void EngineStatistics::tick( float dTime )
{
	BW_GUARD;
	// How often, in second, the frame-rate is updated.
	const float UPDATE_PERIOD = 1.f;

	timeToNextUpdate_ -= dTime;

	if (timeToNextUpdate_ < 0.f)
	{
		lastFrameTime_ = dTime;
		timeToNextUpdate_ = UPDATE_PERIOD;
	}

	DogWatchManager::instance().tick();
}

bool EngineStatistics::logSlowFrames_ = false;

// -----------------------------------------------------------------------------
// Section: Streaming
// -----------------------------------------------------------------------------

/**
 *	Streaming operator for EngineStatistics.
 */
std::ostream& operator<<(std::ostream& o, const EngineStatistics& t)
{
	BW_GUARD;
	o << "EngineStatistics\n";
	return o;
}

BW_END_NAMESPACE

// engine_statistics.cpp
#endif // ENABLE_CONSOLES
