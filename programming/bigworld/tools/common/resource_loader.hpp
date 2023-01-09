#ifndef _RESOURCE_LOADER_HPP_
#define _RESOURCE_LOADER_HPP_

#include "cstdmf/bw_list.hpp"
#include "cstdmf/bw_vector.hpp"
#include "cstdmf/bw_string.hpp"
#include "resmgr/datasection.hpp"

BW_BEGIN_NAMESPACE

// Interface to control the visibility of the splash dialog
class ISplashVisibilityControl
{
public:
	// Pure virtual method to set splash visibility
	virtual void setSplashVisible( bool setVisibility ) = 0;
};


// This class can be used to preload resources
class ResourceLoader
{
public:
	typedef BW::list< BW::string > EffectList;
	// Accessors to the singleton instance
	static ResourceLoader& instance();
	static ResourceLoader* instancePtr();
	static void fini();

	// Precompiles effect files
	void precompileEffects( BW::vector<ISplashVisibilityControl*>& SVCs );

private:
	// Private singleton constructor
	ResourceLoader() {}

	// Returns true if the folder should be excluded from the search
	bool excludedFolder( const BW::string& folderName ) const;

	// Returns the list of effect file folders from the resources.xml files
	bool getEffectFileFolders( BW::vector<DataSectionPtr>& shaderFolders );

	// Finds all files with "extension" in "folderName".
	void findInFolder(
			EffectList& result, const BW::string& folderName,
			const BW::string& extension, const bool searchSubfolders );

	// Caches a particular effect
	bool compileEffects( EffectList& effects, BW::vector<ISplashVisibilityControl*>& SVCs );

	// Helper message to pump all paint messages through the message
	// dispatcher
	void pumpPaintMsgs();

	// Singleton instance
	static ResourceLoader* instance_;
};


BW_END_NAMESPACE
#endif  // _RESOURCE_LOADER_HPP_
