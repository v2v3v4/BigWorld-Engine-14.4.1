#ifndef FORMAT_NAME_MAP_HPP
#define FORMAT_NAME_MAP_HPP

BW_BEGIN_NAMESPACE

namespace Moo
{

#define FNM_ENTRY( x ) \
	twoWayMap.n2fm_.insert( std::make_pair( #x, D3DFMT_##x ) ); \
	twoWayMap.f2nm_.insert( std::make_pair( D3DFMT_##x, #x ) );

typedef StringHashMap<D3DFORMAT> NameToFormatMap;
typedef BW::map<D3DFORMAT,BW::string> FormatToNameMap;

typedef struct 
{
	NameToFormatMap n2fm_;
	FormatToNameMap f2nm_;
} TwoWayFormatNameMatch;

class FormatNameMap
{
public:
	static const BW::string& FormatNameMap::formatString( D3DFORMAT fmt );
	static const D3DFORMAT& FormatNameMap::formatType( BW::string& name );
private:
	static const TwoWayFormatNameMatch& getTwoWayFormatNameMatch();
};


} // namespace Moo

BW_END_NAMESPACE

#endif // FORMAT_NAME_MAP_HPP
