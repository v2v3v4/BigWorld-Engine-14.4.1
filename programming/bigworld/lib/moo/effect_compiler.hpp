#ifndef EFFECT_COMPILER_HPP
#define EFFECT_COMPILER_HPP

#include "cstdmf/bw_string.hpp"

#include "cstdmf/concurrency.hpp"
#include "cstdmf/md5.hpp"

#include "effect_includes.hpp"


BW_BEGIN_NAMESPACE

namespace Moo {

class EffectCompiler
{
public:
	typedef BW::map< BW::string, BW::string > StringStringMap;

public:
	EffectCompiler( bool purgeResCache = false, bool debugSymbols = false );

	bool checkModified( const BW::string& resourceID,
				MD5::Digest* outResDigest = NULL );

	bool getIncludes( const BW::string& resourceID,
				BW::list< BW::string > & outResult );

	BinaryPtr compile( const BW::string& resourceID,
					BW::string* outResult = NULL,
					BW::string* fxoResourceID = NULL);

public:

	static void setMacroDefinition(
		const BW::string & key, 
		const BW::string & value );

	static const BW::string& fxoInfix();
	static void fxoInfix( const BW::string & infix );

private:
	static bool hashResource( const BW::string& name, 
							MD5::Digest& result );	
	static void purgeCachedHash( const BW::string& name );
	static void fillD3dxMacroArray( BW::vector<D3DXMACRO>& macros );

private:
	SimpleMutex mutex_;
	EffectIncludes includes_;
	uint32 compileFlags_;
	bool purgeResCache_;

private:
	typedef BW::map<BW::string, MD5::Digest> StrDigestMap;
	typedef BW::map<BW::string, BW::string> StringStringMap;

	static StrDigestMap s_hashCache_;
	static SimpleMutex s_hashCacheMutex_;
	static BW::string s_fxoInfix_;
	static StringStringMap s_macros_;
};

} // namespace Moo

BW_END_NAMESPACE

#endif
