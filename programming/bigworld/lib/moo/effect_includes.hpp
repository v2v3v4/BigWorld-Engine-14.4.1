#ifndef __EFFECT_INCLUDES_HPP__
#define __EFFECT_INCLUDES_HPP__

#include "moo_dx.hpp"


BW_BEGIN_NAMESPACE

namespace Moo
{

/**
*	This class implements the ID3DXInclude interface used for loading effect 
*	files and their includes.
*/
class EffectIncludes : public ID3DXInclude, public ReferenceCount
{
public:	
	typedef BW::list< BW::string > StringList;
	typedef BW::vector<BW::string> IncludePaths;

	EffectIncludes();

	void addIncludePaths( const BW::string& pathString );

	const IncludePaths& includePaths() const { return includePaths_; }
	IncludePaths& includePaths() { return includePaths_; }

	void resetDependencies();
	StringList& dependencies();

	void currentPath( const BW::string& currentPath );
	const BW::string& currentPath() const;

	BW::string resolveInclude( const BW::string& name );


private:
	HRESULT WINAPI Open( D3DXINCLUDE_TYPE IncludeType, 
						LPCSTR	pFileName,
						LPCVOID pParentData, 
						LPCVOID *ppData, 
						UINT	*pBytes );
	HRESULT WINAPI Close( LPCVOID pData );

private:
	typedef std::pair<LPCVOID, BinaryPtr> IncludeFile;
	typedef BW::vector<IncludeFile> IncludeFileVector;

	IncludePaths includePaths_;

	IncludeFileVector includes_;
	StringList includesNames_;
	BW::string currentPath_;
};

} // namespace Moo

BW_END_NAMESPACE

#endif // __EFFECT_INCLUDES_HPP__
