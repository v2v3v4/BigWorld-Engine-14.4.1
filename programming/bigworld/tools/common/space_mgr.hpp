#ifndef SPACE_MGR_HPP
#define SPACE_MGR_HPP

#include <commctrl.h>
#include <shtypes.h>
#include "cstdmf/bw_vector.hpp"
#include "cstdmf/bw_string.hpp"


BW_BEGIN_NAMESPACE

struct MRUProvider
{
	virtual ~MRUProvider(){}
	virtual void set( const BW::string& name, const BW::string& value ) = 0;
	virtual const BW::string get( const BW::string& name ) const = 0;
};

class SpaceNameManager
{
	BW::vector<BW::string>::size_type maxMRUEntries_;
	BW::vector<BW::string> recentSpaces_;

	static int CALLBACK BrowseCallbackProc( HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData );
	static BW::wstring getFolderByPidl( LPITEMIDLIST pidl );
	static LPITEMIDLIST commonRoot();

	MRUProvider& mruProvider_;
public:
	SpaceNameManager( MRUProvider& mruProvider, BW::vector<BW::string>::size_type maxMRUEntries = 10 );
	void addSpaceIntoRecent( const BW::string& space );
	void removeSpaceFromRecent( const BW::string& space );
	BW::vector<BW::string>::size_type num() const;
	BW::string entry( BW::vector<BW::string>::size_type index ) const;

	BW::string browseForSpaces( HWND parent );

	static BW::string browseForSpaces( HWND parent, const BW::wstring& defaultPath );
};

BW_END_NAMESPACE

#endif//SPACE_MGR_HPP
