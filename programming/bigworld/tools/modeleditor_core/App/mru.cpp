#include "pch.hpp"

#include "mru.hpp"

#include "appmgr/options.hpp"
#include "cstdmf/guard.hpp"
#include "resmgr/bwresource.hpp"

BW_BEGIN_NAMESPACE

/*static*/ MRU& MRU::instance()
{
	static MRU s_instance_;
	return s_instance_;
}

MRU::MRU():
	maxMRUs_(10)
{}

void MRU::update( const BW::string& mruName, const BW::string& file, bool add /* = true */ )
{
	BW_GUARD;

	BW::vector<BW::string> files;
	if (add)
	{
		files.push_back( file );
	}

	char entry[8];
	BW::string tempStr;

	for (unsigned i=0; i<maxMRUs_; i++)
	{
		bw_snprintf( entry, sizeof(entry), "file%d", i );
		tempStr = Options::getOptionString( mruName +"/"+ entry, file );
		Options::setOptionString( mruName +"/"+ entry, "" );
		if ( tempStr != file )
		{
			files.push_back( tempStr );
		}
	}

	for (size_t i=0; i < min( (size_t ) maxMRUs_, files.size() ); i++)
	{
		bw_snprintf( entry, sizeof(entry), "file%d", i );
		Options::setOptionString( mruName +"/"+ entry, files[i] );
	}

	Options::save();
}

void MRU::read( const BW::string& mruName, BW::vector<BW::string>& files )
{
	BW_GUARD;

	char entry[8];
			
	for (unsigned i=0; i<maxMRUs_; i++)
	{
		bw_snprintf( entry, sizeof(entry), "file%d", i );
		BW::string tempStr = Options::getOptionString( mruName +"/"+ entry, "" );
		if ( tempStr != "" )
		{
			files.push_back( tempStr );
		}
	}
}

void MRU::getDir( const BW::string& mruName, BW::string& dir, const BW::string& defaultDir /* = "" */ )
{
	BW_GUARD;

	dir = Options::getOptionString( mruName +"/file0", defaultDir );

	if ( dir != "" )
	{
		dir = BWResource::resolveFilename( dir );
		std::replace( dir.begin(), dir.end(), '/', '\\' );
		dir = dir.substr( 0, dir.find_last_of( "\\" ));
	}
}

BW_END_NAMESPACE

