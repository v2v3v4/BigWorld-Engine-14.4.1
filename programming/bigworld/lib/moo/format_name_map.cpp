#include "pch.hpp"
#include "moo/format_name_map.hpp"

BW_BEGIN_NAMESPACE

namespace Moo
{

/*static */ const TwoWayFormatNameMatch& FormatNameMap::getTwoWayFormatNameMatch()
{
	// This can be called by multiple texture converters on parallel threads.
	static SimpleMutex twoWayMapMutex;
	SimpleMutexHolder smh( twoWayMapMutex );

	static TwoWayFormatNameMatch twoWayMap;

	if ( twoWayMap.f2nm_.empty() )
	{
		/* See d3d9types.h */
		FNM_ENTRY( UNKNOWN )

		FNM_ENTRY( R8G8B8 )
		FNM_ENTRY( A8R8G8B8 )
		FNM_ENTRY( X8R8G8B8 )
		FNM_ENTRY( R5G6B5 )
		FNM_ENTRY( X1R5G5B5 )
		FNM_ENTRY( A1R5G5B5 )
		FNM_ENTRY( A4R4G4B4 )
		FNM_ENTRY( R3G3B2 )
		FNM_ENTRY( A8 )
		FNM_ENTRY( A8R3G3B2 )
		FNM_ENTRY( X4R4G4B4 )
		FNM_ENTRY( A2B10G10R10 )
		FNM_ENTRY( G16R16 )

		FNM_ENTRY( A8P8 )
		FNM_ENTRY( P8 )

		FNM_ENTRY( L8 )
		FNM_ENTRY( A8L8 )
		FNM_ENTRY( A4L4 )

		FNM_ENTRY( V8U8 )
		FNM_ENTRY( L6V5U5 )
		FNM_ENTRY( X8L8V8U8 )
		FNM_ENTRY( Q8W8V8U8 )
		FNM_ENTRY( V16U16 )
		//	FNM_ENTRY( W11V11U10 )
		FNM_ENTRY( A2W10V10U10 )

		FNM_ENTRY( UYVY )
		FNM_ENTRY( YUY2 )
		FNM_ENTRY( DXT1 )
		FNM_ENTRY( DXT2 )
		FNM_ENTRY( DXT3 )
		FNM_ENTRY( DXT4 )
		FNM_ENTRY( DXT5 )

		FNM_ENTRY( D16_LOCKABLE )
		FNM_ENTRY( D32 )
		FNM_ENTRY( D15S1 )
		FNM_ENTRY( D24S8 )
		FNM_ENTRY( D16 )
		FNM_ENTRY( D24X8 )
		FNM_ENTRY( D24X4S4 )


		FNM_ENTRY( VERTEXDATA )
		FNM_ENTRY( INDEX16 )
		FNM_ENTRY( INDEX32 )
	}
	return twoWayMap;
}


const BW::string& FormatNameMap::formatString( D3DFORMAT fmt )
{
	static const BW::string unknown("D3DFMT_UNKNOWN");
	const TwoWayFormatNameMatch& match = getTwoWayFormatNameMatch();
	FormatToNameMap::const_iterator it = match.f2nm_.find( fmt );
	if (it != match.f2nm_.end())
	{
		return it->second;
	}
	return unknown;
}

const D3DFORMAT& FormatNameMap::formatType( BW::string& name )
{
	static const D3DFORMAT unknown = D3DFMT_UNKNOWN;
	const TwoWayFormatNameMatch& match = getTwoWayFormatNameMatch();
	NameToFormatMap::const_iterator it = match.n2fm_.find( name );
	if (it != match.n2fm_.end())
	{
		return it->second;
	}
	return unknown;
}
} // namespace Moo

BW_END_NAMESPACE
