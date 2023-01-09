#ifndef _INCLUDE_VISUALFILETRANSLATOR_HPP
#define _INCLUDE_VISUALFILETRANSLATOR_HPP

#include <maya/MPxFileTranslator.h>

#include "hull_mesh.hpp"
#include "visual_mesh.hpp"
#include "visual_portal.hpp"

#include "cstdmf/stringmap.hpp"

BW_BEGIN_NAMESPACE

// our plugin file translator class. This must inherit
// from the base class MPxFileTranslator
class VisualFileTranslator : public MPxFileTranslator
{
public:
	typedef BW::map<BW::string, size_t>	BoneCountMap;
	typedef BoneCountMap::iterator		BoneCountMapIt;

	VisualFileTranslator() : MPxFileTranslator() {};
	~VisualFileTranslator() {};

	// we overload this method to perform any 
	// file export operations needed
	MStatus writer( const MFileObject& file, const MString& optionsString, FileAccessMode mode); 

	// returns true if this class can export files
	bool haveWriteMethod() const;

	// returns the default extension of the file supported
	// by this FileTranslator.
	MString defaultExtension() const;

	// add a filter for open file dialog
	MString filter() const;

	// Used by Maya to create a new instance of our
	// custom file translator
	static void* creator();

	// some option flags set by the mel script
	bool	exportMesh( BW::string fileName );

	// Returns the number of unique bones
	void	getBoneCounts( BoneCountMap& boneCountMap );

private:

	class AutoCleanup;

	void cleanup();
	void addMeshName( const BW::string& name );

	void loadReferenceNodes( const BW::string & mfxFile );
	void readReferenceHierarchy( DataSectionPtr hierarchy );

	BW::string lookupBackupFilename( const BW::string& fileName );

	StringHashMap<BW::string> nodeParents_;

	BW::vector<VisualMeshPtr>		visualMeshes;
	BW::vector<HullMeshPtr>			hullMeshes;
	BW::vector<VisualMeshPtr>		bspMeshes;
	BW::vector<VisualPortalPtr>		visualPortals;
	
	BW::set<BW::string>				meshNames_;
	BW::set<BW::string>				duplicateMeshNames_;
};

BW_END_NAMESPACE

#endif
