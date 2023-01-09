#include "pch.hpp"

#if ENABLE_CONSOLES
#include "resource_manager_stats.hpp"


BW_BEGIN_NAMESPACE

ResourceManagerStats* ResourceManagerStats::s_instance_ = NULL;


ResourceManagerStats& ResourceManagerStats::instance()
{
	if (!s_instance_)
		s_instance_ = new ResourceManagerStats;
	return *s_instance_;
}


void ResourceManagerStats::fini()
{
	delete s_instance_; 
	s_instance_ = NULL;
}

ResourceManagerStats::ResourceManagerStats():
	enabled_( true ),
	frame_( 0 ),
	pEventQuery_( NULL )
{
	createUnmanagedObjects();
}


ResourceManagerStats::~ResourceManagerStats()
{
	deleteUnmanagedObjects();
}


void ResourceManagerStats::displayStatistics( XConsole & console )
{
	BW_GUARD;
	static const size_t STR_SIZE = 64;
	char statString[STR_SIZE];
	statString[STR_SIZE - 1] = 0;

	this->getData();
	for (uint32 i=0; i<D3DRTYPECOUNT; i++)
	{			
		D3DRESOURCESTATS& stats = results_[frame_][i];

		_snprintf( statString, STR_SIZE-1, "%s\n", resourceString(i).c_str() );
		console.print( statString );
		_snprintf( statString, STR_SIZE-1, "Thrashing  : %s\tBytes Downloaded : %d   \n", stats.bThrashing ? "Yes" : "No", stats.ApproxBytesDownloaded );
		console.print( statString );
		_snprintf( statString, STR_SIZE-1, "Evicts : %d\tCreates : %d\t\n", stats.NumEvicts, stats.NumVidCreates  );
		console.print( statString );
	}	
}


void ResourceManagerStats::getData()
{
	BW_GUARD;
	pEventQuery_->Issue( D3DISSUE_END );
	HRESULT hr = pEventQuery_->GetData(&results_[frame_],
		sizeof(D3DRESOURCESTATS) * D3DRTYPECOUNT, 0);
	frame_ = (frame_+1) % NUM_FRAMES;
}


void ResourceManagerStats::logStatistics( std::ostream& f_logFile )
{
	BW_GUARD;
	this->getData();	

	for (uint32 f=0; f<NUM_FRAMES; f++)
	{
		for (uint32 i=0; i<D3DRTYPECOUNT; i++)
		{			
			D3DRESOURCESTATS& stats = results_[f][i];

			f_logFile << resourceString(i);
			f_logFile << " : Thrashing (";
			if (stats.bThrashing)
				f_logFile << "Yes)";
			else
				f_logFile << "No)";
			f_logFile << ", Bytes Downloaded : " << stats.ApproxBytesDownloaded;
			f_logFile << ", Evicts : " << stats.NumEvicts;
			f_logFile << ", Creates : " << stats.NumVidCreates << std::endl;			
		}
	}
}


const BW::string& ResourceManagerStats::resourceString( uint32 i )
{	
	BW_GUARD;
	switch (i)
	{
	case D3DRTYPE_SURFACE:
		{
			static BW::string szRet( "Surface" );
			return szRet;
		}
	case D3DRTYPE_VOLUME:
		{
			static BW::string szRet( "Volume" );
			return szRet;
		}
	case D3DRTYPE_TEXTURE:
		{
			static BW::string szRet( "Texture" );
			return szRet;
		}
	case D3DRTYPE_VOLUMETEXTURE:
		{
			static BW::string szRet( "Volume Texture" );
			return szRet;
		}
	case D3DRTYPE_CUBETEXTURE:
		{
			static BW::string szRet( "Cube Texture" );
			return szRet;
		}
	case D3DRTYPE_VERTEXBUFFER:
		{
			static BW::string szRet( "Vertex Buffer" );
			return szRet;
		}
	case D3DRTYPE_INDEXBUFFER:
		{
			static BW::string szRet( "Index Buffer" );
			return szRet;
		}
	default:
		{
			static BW::string szRet( "Unkown Resource Type" );
			return szRet;
		}
	}
}


void ResourceManagerStats::createUnmanagedObjects()
{
	BW_GUARD;
	HRESULT hr = Moo::rc().device()->CreateQuery(
		D3DQUERYTYPE_RESOURCEMANAGER, NULL);

	available_ = SUCCEEDED(hr);
	if (available_)
	{
		Moo::rc().device()->CreateQuery(D3DQUERYTYPE_RESOURCEMANAGER,
										&pEventQuery_);	
		memset( &results_, 0, sizeof(D3DRESOURCESTATS) * D3DRTYPECOUNT * NUM_FRAMES );
	}
}


void ResourceManagerStats::deleteUnmanagedObjects()
{
	BW_GUARD;
	if (pEventQuery_)
	{
		pEventQuery_->Release();
		pEventQuery_ = NULL;
	}
}

BW_END_NAMESPACE

#endif // ENABLE_CONSOLES
// resource_manager_stats.cpp
