#ifndef CVSWRAPPER_HPP
#define CVSWRAPPER_HPP


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "cstdmf/bw_set.hpp"

BW_BEGIN_NAMESPACE

struct CVSLog
{
	virtual ~CVSLog(){};
	virtual void add( const BW::wstring& msg ) = 0;
};

class CVSWrapper
{
	BW::string output_;
	static BW::string cvsPath_;
	BW::string workingPath_;
	static unsigned int batchLimit_;
	static bool enabled_;
	static bool directoryCommit_;
	static BW::string dirToIgnore_;

	CVSLog* log_;

	enum Status
	{
		UNKNOWN = 0,
		ADDED,
		REMOVED,
		UPTODATE,
		LOCALMODIFIED,
		NEEDCHECKOUT
	};

	static bool isFile( const BW::string& pathName );
	static bool isDirectory( const BW::string& pathName );
	static bool exists( const BW::string& pathName );
	static bool exec( BW::string cmd, BW::string workingDir, int& exitCode, BW::string& output, CVSLog* log );
public:
	enum InitResult
	{
		SUCCESS = 0,
		DISABLED,
		FAILURE
	};
	static InitResult init();

	CVSWrapper( const BW::string& workingPath, CVSLog* log = NULL );

	static bool ignoreDir( const BW::string & dir ) { return dir == dirToIgnore_; }

	void refreshFolder( const BW::string& relativePathName );

	bool editFiles( BW::vector<BW::string> filesToEdit );
	bool revertFiles( BW::vector<BW::string> filesToRevert );
	bool updateFolder( const BW::string& relativePathName );
	bool commitFiles( const BW::set<BW::string>& filesToCommit,
		const BW::set<BW::string>& foldersToCommit,
		const BW::string& commitMsg );

	bool isInCVS( const BW::string& relativePathName );
	static bool enabled()	{	return enabled_;	}

	void removeFile( const BW::string& relativePathName );
	BW::set<BW::string> addFolder( BW::string relativePathName, const BW::string& commitMsg, bool checkParent = true );
	bool addFile( BW::string relativePathName, bool isBinary, bool recursive );

	const BW::string& output() const;
};

BW_END_NAMESPACE

#endif // CVSWRAPPER_HPP
