#pragma warning ( disable : 4530 )
#pragma warning ( disable : 4786 )
#pragma warning ( disable : 4244 )

#include "cstdmf/stdmf.hpp"
#include "cstdmf/bw_set.hpp"
#include "cstdmf/dprintf.hpp"
#include "cstdmf/binaryfile.hpp"
#include "cstdmf/log_msg.hpp"

#include "resmgr/bwresource.hpp"

#include "mfxexp.hpp"
#include "aboutbox.hpp"
#include "vertcont.hpp"
#include "utility.hpp"
#include "cuetrack.hpp"
#include "data_section_cache_purger.hpp"
#include "resmgr/xml_section.hpp"

#include "iparamm2.h"
#include "iskin.h"

#ifdef MAX_RELEASE_R14
#include "MorpherClassID.h"
#else
#include "wm3.h"
#endif // MAX_RELEASE_R14


#ifndef BW_EXPORTER_DEBUG
#pragma warning ( disable : 4244 )

// File names were changed in max 2012
#ifdef MAX_RELEASE_R14
#include "maxscript/maxscript.h"
#include "maxscript/macros/define_instantiation_functions.h"
#else
#include "maxscrpt/maxscrpt.h"
#include "maxscrpt/maxobj.h"
#include "maxscrpt/definsfn.h"
#endif // MAX_RELEASE_R14

#endif // BW_EXPORTER_DEBUG


BW_BEGIN_NAMESPACE

namespace
{
	DataSectionPtr pSettingsOverride;
};

// Settings are only scriptable in release mode as the debug version 
// crashes when linked against the maxscript library
#ifndef BW_EXPORTER_DEBUG
def_visible_primitive(BWAnimationSetting, "BWAnimationSetting");

Value* BWAnimationSetting_cf(Value **arg_list, int count)
{
	if (!pSettingsOverride)
	{
		pSettingsOverride = XMLSection::creator()->create( NULL, "settings" );
	}

	for (int i = 0; i < count; i += 2)
	{
#ifdef _UNICODE
		BW::string setting = bw_wtoutf8( arg_list[i]->to_string() );
#else
		BW::string setting = arg_list[i]->to_string();
#endif
		
		if (count > i + 1)
		{
			if (setting == "allow_scale")
			{
				pSettingsOverride->writeBool( "allowScale", arg_list[i + 1]->to_bool() == TRUE );
			}
			else if (setting == "node")
			{
				pSettingsOverride->writeBool( "exportNodeAnimation", arg_list[i + 1]->to_bool() == TRUE );
			}
			else if (setting == "cue_track")
			{
				pSettingsOverride->writeBool( "exportCueTrack", arg_list[i + 1]->to_bool() == TRUE );
			}
			else if (setting == "opposite_facing")
			{
				pSettingsOverride->writeBool( "useLegacyOrientation", arg_list[i + 1]->to_bool() == TRUE );
			}
			else if (setting == "reference_hierarchy")
			{
#ifdef _UNICODE
				pSettingsOverride->writeString( "referenceNodesFile", bw_wtoutf8( arg_list[i + 1]->to_string() ) );
#else
				pSettingsOverride->writeString( "referenceNodesFile", arg_list[i + 1]->to_string() );
#endif
			}
		}

		if (setting == "reset")
		{
			pSettingsOverride = NULL;
			return &ok;
		}

		
	}

	return &ok;
}
#endif

MFXExport::MFXExport()
: nodeCount_ ( 0 ),
  animNodeCount_( 0 )/*,
  settings_( ExportSettings::instance() )*/
{
}

MFXExport::~MFXExport()
{
}

/*
 * Number of extensions supported, one (animation)
 */
int MFXExport::ExtCount()
{
	return 1;
}

/*
 * Extension #n (only supports one, animation )
 */
const TCHAR* MFXExport::Ext(int n)
{
	switch( n )
	{
	case 0:
		return _T( "animation" );
	}
	return _T("");
}

/*
 * Long ASCII description
 */
const TCHAR* MFXExport::LongDesc()
{
	return GetString(IDS_LONGDESC);
}


/*
 * Short ASCII description (i.e. "3D Studio")
 */
const TCHAR* MFXExport::ShortDesc()
{
#if defined BW_EXPORTER_DEBUG
	static const char shortDesc[] = "Animation Exporter Debug";
	return shortDesc;
#else
	return GetString(IDS_SHORTDESC);
#endif
}

/*
 * ASCII Author name
 */
const TCHAR* MFXExport::AuthorName()
{
	return GetString( IDS_AUTHORNAME );
}

/*
 * ASCII Copyright message
 */
const TCHAR* MFXExport::CopyrightMessage()
{
	return GetString(IDS_COPYRIGHT);
}

/*
 * Other message #1
 */
const TCHAR* MFXExport::OtherMessage1()
{
	return _T("");
}

/*
 * Other message #2
 */
const TCHAR* MFXExport::OtherMessage2()
{
	return _T("");
}

/*
 * Version number * 100 (i.e. v3.01 = 301)
 */
unsigned int MFXExport::Version()
{
	return MFX_EXPORT_VERSION;
}

/*
 * Show DLL's "About..." box
 */
void MFXExport::ShowAbout(HWND hWnd)
{
	AboutBox ab;
	ab.display( hWnd );
}

void mungeIncluded( MFXNode* node )
{
	if (node->include())
	{
		node->includeAncestors();
	}
	for (int i = 0; i < node->getNChildren(); i++)
	{
		mungeIncluded( node->getChild( i ) );
	}
}


/*
 * Main entrypoint to exporter.
 * Do export.
 */

int MFXExport::DoExport(const TCHAR *nameFromMax,ExpInterface *ei,Interface *i, BOOL suppressPrompts, DWORD options)
{
	// Purges the datasection cache after finishing each export
	DataSectionCachePurger dscp;

	// Force the extension to lowercase
#ifdef _UNICODE
	BW::string filename = bw_acptoutf8( bw_wtoutf8( nameFromMax ) );
#else
	BW::string filename = bw_acptoutf8( nameFromMax );
#endif
	
	filename = filename.substr(0, filename.size() - 10) + ".animation";

	errors_ = false;
	suppressPrompts_ = suppressPrompts == TRUE;
	
	// when using noPrompt flag, send error message to log.debug
	LogMsg::automatedTest( suppressPrompts_, 
		filename.substr(0, filename.find("\\testmaxfiles")).c_str());

	ip_ = i;

	// Check if the filename is in the right res path.
	BW::string name2 = BWResolver::dissolveFilename( filename );
	if ((name2.length() == filename.length()) || (name2.find_last_of("/\\") == BW::string::npos))
	{
		errors_ = true;
		BW::string errorString;
		errorString = BW::string( "The exported file \"" ) + filename + BW::string( "\" is not in a valid game path.\n" );
		errorString += "Need to export to a *subfolder* of:";
        uint32 pathIndex = 0;
		while (BWResource::getPath(pathIndex) != BW::string(""))
		{
			errorString += BW::string("\n") + BWResource::getPath(pathIndex++);
		}
		if (!suppressPrompts_)
		{
#ifdef _UNICODE
			MessageBox( ip_->GetMAXHWnd(), bw_utf8tow( errorString.c_str() ).c_str(), L"Animation Exporter Error", MB_OK | MB_ICONWARNING );
#else
			MessageBox( ip_->GetMAXHWnd(), errorString.c_str(), "Animation Exporter Error", MB_OK | MB_ICONWARNING );
#endif
			
		}
		if ( LogMsg::automatedTest() )
		{
			LogMsg::logToFile( errorString.c_str() );
		}
		return 0;
	}

	Interval inter = ip_->GetAnimRange();
	ExportSettings::instance().setFrameRange( inter.Start() / GetTicksPerFrame(), inter.End() / GetTicksPerFrame() );
	BW::string cfgFilename = getCfgFilename();

	BW::string settingsFilename = BWResource::removeExtension( filename ) + ".animationsettings";

	ExportSettings::instance().readSettings( cfgFilename.c_str() );
	ExportSettings::instance().readSettings( settingsFilename.c_str() );
	ExportSettings::instance().readSettings( pSettingsOverride );
	
	ExportSettings::instance().setStaticFrame( ip_->GetTime() / GetTicksPerFrame() );

	if (!suppressPrompts_)
	{
		if( !ExportSettings::instance().displayDialog( ip_->GetMAXHWnd() ) )
		{
#ifdef _UNICODE
			OutputDebugString( L"Canceled\n" );
#else
			OutputDebugString( "Canceled\n" );
#endif		
			return 0;
		}
	}
	pSettingsOverride = NULL;

	if (options == SCENE_EXPORT_SELECTED)
	{
		ExportSettings::instance().nodeFilter( ExportSettings::SELECTED );
	}
	else
	{
		ExportSettings::instance().nodeFilter( ExportSettings::VISIBLE );
	}

	ExportSettings::instance().writeSettings( cfgFilename );

	// clear any old cue track as plugins is kept loaded and CueTrack is a singleton
	if( ExportSettings::instance().exportCueTrack() )
		CueTrack::clear();

	preProcess( ip_->GetRootNode() );

	exportMeshes();
	exportEnvelopes();

	if (ExportSettings::instance().referenceNodesFile() != "")
	{
		this->loadReferenceNodes( ExportSettings::instance().referenceNodesFile() );
		StringHashMap<BW::string>::iterator it = nodeParents_.begin();
		StringHashMap<BW::string>::iterator end = nodeParents_.end();
		while (it != end)
		{
			BW::string parent = it->second;
			BW::string child = it->first;
			it++;

			MFXNode* p = mfxRoot_->find( parent );
			MFXNode* c = mfxRoot_->find( child );
			if ((p && c) &&
				(p != c->getParent()))
			{
				MFXNode* parent = p;
				while (parent && parent->getParent() != c)
				{
					parent = parent->getParent();
				}
				if (parent)
				{
					c->removeChild( parent );
					c->getParent()->addChild( parent );
				}

				MFXNode* op = c->getParent();
				if (op)
					op->removeChild( c );
				p->addChild( c );
			}
		}
	}

	// Order the immediate children of mfxRoot_ in decending order based on their respective
	// child count.
	//
	// 1) Populate a list
	BW::list<MFXNode*> attachedToRoot;
	while ( mfxRoot_->getNChildren() )
	{
		attachedToRoot.push_back( mfxRoot_->getChild( 0 ) );
		mfxRoot_->removeChild( 0 );
	}

	// 2) Sort list in descending order
	attachedToRoot.sort( numChildDescending );

	// 3) Repopulate mfxRoot_
	while ( attachedToRoot.size() )
	{
		mfxRoot_->addChild( attachedToRoot.front() );
		attachedToRoot.pop_front();
	}

	mungeIncluded( mfxRoot_ );

	// Check if we have any duplicate node id's or morph target ids
	BW::string errorMsg;
	if (!validateAnimationIDs(errorMsg))
	{	
		if (!suppressPrompts_)
		{
#ifdef _UNICODE
			::MessageBox( 0, bw_utf8tow( errorMsg.c_str() ).c_str(), L"Animation failed validity checking", MB_ICONERROR );
#else
			::MessageBox( 0, errorMsg.c_str(), "Animation failed validity checking", MB_ICONERROR );
#endif
		}
		if ( LogMsg::automatedTest() )
		{
			LogMsg::logToFile( errorMsg.c_str() );
		}
		return 0;
	}

	BW::string animName = filename;
	size_t cropIndex = 0;
	if ((cropIndex = animName.find_last_of( "\\/" )) > 0)
	{
		cropIndex++;
		animName = animName.substr( cropIndex, animName.length() - cropIndex );
	}

	if ((cropIndex = animName.find_last_of( "." )) > 0 )
	{
		animName = animName.substr( 0, cropIndex );
	}

	int nChannels = 0;
	if (mfxRoot_ && ExportSettings::instance().exportNodeAnimation() )
		nChannels += mfxRoot_->nIncludedNodes();

	if( ExportSettings::instance().exportCueTrack() && CueTrack::hasCues() )
		nChannels += 1;

	if (nChannels)
	{
		CompressionInfo ci;
		ci.specifyAmounts_ = false;

		// Try to read in compression amounts
		FILE* file = _wfopen( bw_utf8tow( filename ).c_str(), L"rb" );
		if (file)
		{
			BinaryFile animation( file );
			float f;
			BW::string s;
			int numChannels;
			int channelType;

			// Read in the header
			animation >> f;
			animation >> s;
			animation >> s;
			animation >> numChannels;

			// Get the compression amounts from the first channel
			if (numChannels > 0)
			{
				animation >> channelType;
				// Provided it's the correct type
				if (channelType == 4)
				{
					animation >> s;

					ci.specifyAmounts_ = true;
					animation >> ci.scaleCompressionError_;
					animation >> ci.positionCompressionError_;
					animation >> ci.rotationCompressionError_;
				}
			}

			fclose(file);
		}


		file = _wfopen( bw_utf8tow( filename ).c_str(), L"wb" );

		if (file)
		{
			BinaryFile animation( file );

			animation << float( ExportSettings::instance().lastFrame() - ExportSettings::instance().firstFrame() ) << animName << animName;
			animation << nChannels;

			if (mfxRoot_ && ExportSettings::instance().exportNodeAnimation())
			{
				mfxRoot_->setMaxNode( NULL );
				mfxRoot_->setIdentifier( "Scene Root" );
				mfxRoot_->exportAnimation( animation, ci );
			}

			if( ExportSettings::instance().exportCueTrack() )
				CueTrack::writeFile( animation );

			fclose(file);
		}
#ifdef _UNICODE
		::DeleteFile( bw_utf8tow( settingsFilename ).c_str() );
#else
		::DeleteFile( settingsFilename.c_str() );
#endif
		
		ExportSettings::instance().writeSettings( settingsFilename );
	}

	return 1;
}

void MFXExport::readReferenceHierarchy( DataSectionPtr hierarchy )
{
	BW::string id = hierarchy->readString( "identifier" );
	BW::vector< DataSectionPtr> nodeSections;
	hierarchy->openSections( "node", nodeSections );

	for (uint i = 0; i < nodeSections.size(); i++)
	{
		DataSectionPtr p = nodeSections[i];
		nodeParents_[ p->readString( "identifier" ) ] = id;
		readReferenceHierarchy( p );
	}
}

/**
 *	Load a reference node hierarchy from an existing visual file
 */
void MFXExport::loadReferenceNodes( const BW::string & visualFile )
{

	BW::string filename = BWResolver::dissolveFilename( visualFile );

	DataSectionPtr pVisualSection = BWResource::openSection( filename );

	if (pVisualSection)
	{
		readReferenceHierarchy( pVisualSection->openSection( "node" ) );
	}
}


/*
 * Preprocess all the nodes, counts the total number of nodes.
 * Adds all used materials to the material list etc.
 */
void MFXExport::preProcess( INode * node, MFXNode *mfxParent )
{
	nodeCount_++;

	bool includeNode = false;

	if (ExportSettings::instance().nodeFilter() == ExportSettings::ALL || mfxParent == NULL)
	{
		includeNode = true;
	}
	else if (ExportSettings::instance().nodeFilter() == ExportSettings::SELECTED )
	{
		if (node->Selected())
			includeNode = true;
	}
	else if (ExportSettings::instance().nodeFilter() == ExportSettings::VISIBLE)
	{
		if (!node->IsHidden())
			includeNode = true;
	}

	ObjectState os = node->EvalWorldState( staticFrame() );

	if( os.obj )
	{
		if( os.obj->ClassID() == Class_ID(TARGET_CLASS_ID, 0) )
			includeNode = false;

		// Particle view class id
		if( os.obj->ClassID() == Class_ID(0x74f93b07, 0x1eb34300) )
			includeNode = false;

		if( os.obj->SuperClassID() == CAMERA_CLASS_ID )
			includeNode = false;

		if( os.obj->SuperClassID() == LIGHT_CLASS_ID )
			includeNode = false;

		if( os.obj->SuperClassID() == SHAPE_CLASS_ID )
			includeNode = false;
	}


	if (includeNode)
	{
		Modifier *mod = findPhysiqueModifier( node );
		Modifier *mod2 = findSkinMod( node );

		if( mod || mod2 )
		{
			envelopeNodes_.push_back( node );
			includeNode = false;
		}
		else
		{
			meshNodes_.push_back( node );
		}
	}

	MFXNode* thisNode = mfxParent;

	thisNode = new MFXNode( node );
	thisNode->include( includeNode );
	if (mfxParent)
		mfxParent->addChild( thisNode );
	else
		mfxRoot_ = thisNode;

	for( int index = 0; index < node->NumberOfChildren(); index++ )
		preProcess( node->GetChildNode( index ), thisNode );
}

/*
 * Create the config filename and path from the predefined config filename
 * and the global plugin config directory.
 */
BW::string MFXExport::getCfgFilename( void )
{
#ifdef _UNICODE
	BW::string filename = bw_wtoutf8( ip_->GetDir(APP_PLUGCFG_DIR) );
#else
	BW::string filename(ip_->GetDir(APP_PLUGCFG_DIR));
#endif
	filename += "\\";
	filename += CFGFilename;
	return filename;
}


void MFXExport::exportMeshes( void )
{
	for( uint i = 0; i < meshNodes_.size(); i++ )
	{
		exportMesh( meshNodes_[ i ] );
	}
}

void MFXExport::exportMesh( INode *node )
{

	bool deleteIt = false;

	VisualMeshPtr spVisualMesh = new VisualMesh;

	if (spVisualMesh)
		if (spVisualMesh->init( node ) )
			visualMeshes_.push_back( spVisualMesh );
}

//
//	- exportEnvelopes
//  Goes through the list of envelopenodes and exports them through exportenvelope
//
void MFXExport::exportEnvelopes( void )
{
	for( uint32 i = 0; i < envelopeNodes_.size(); i++ )
	{
		exportEnvelope( envelopeNodes_[ i ] );
	}
}

/*
 *
 */
void MFXExport::exportEnvelope( INode *node )
{
	VisualEnvelopePtr spVisualEnvelope = new VisualEnvelope;
	if (spVisualEnvelope)
	{
		if(spVisualEnvelope->init( node, mfxRoot_ ))
			visualMeshes_.push_back( spVisualEnvelope );
	}
}

TriObject* MFXExport::getTriObject( INode *node, TimeValue t, bool &needDelete )
{
	needDelete = false;
	ObjectState os = node->EvalWorldState( t );

	if( os.obj && ( os.obj->SuperClassID() == GEOMOBJECT_CLASS_ID ) )
	{
		Object *obj = os.obj;
		if (obj->CanConvertToType(Class_ID(TRIOBJ_CLASS_ID, 0)))
		{
			TriObject *tri = (TriObject *) obj->ConvertToType( t, Class_ID(TRIOBJ_CLASS_ID, 0) );
			if ( obj != tri )
				needDelete = true;

			return tri;
		}
	}
	return NULL;
}

Modifier* MFXExport::findMorphModifier( INode* node )
{
	// Get object from node. Abort if no object.
	Object* object = node->GetObjectRef();

	// Is derived object ?
	while( object && object->SuperClassID() == GEN_DERIVOB_CLASS_ID )
	{
		// Yes -> Cast.
		IDerivedObject* derivedObject = static_cast< IDerivedObject* >( object );

		// Iterate over all entries of the modifier stack.
		int modStackIndex = 0;
		while( modStackIndex < derivedObject->NumModifiers() )
		{
			// Get current modifier.
			Modifier* modifier = derivedObject->GetModifier( modStackIndex );

			// Is this Physique ?
			if( modifier->ClassID() == MR3_CLASS_ID )
			{
				// Yes -> Exit.
				return modifier;
			}

			// Next modifier stack entry.
			modStackIndex++;
		}
		object = derivedObject->GetObjRef();
	}

	// Not found.
	return NULL;
}

Modifier* MFXExport::findPhysiqueModifier( INode* node )
{
	// Get object from node. Abort if no object.
	Object* object = node->GetObjectRef();

	if( !object )
		return NULL;

	// Is derived object ?
	if( object->SuperClassID() == GEN_DERIVOB_CLASS_ID )
	{
		// Yes -> Cast.
		IDerivedObject* derivedObject = static_cast< IDerivedObject* >( object );

		// Iterate over all entries of the modifier stack.
		int modStackIndex = 0;
		while( modStackIndex < derivedObject->NumModifiers() )
		{
			// Get current modifier.
			Modifier* modifier = derivedObject->GetModifier( modStackIndex );

			// Is this Physique ?
			if( modifier->ClassID() == Class_ID( PHYSIQUE_CLASS_ID_A, PHYSIQUE_CLASS_ID_B ) )
			{
				// Yes -> Exit.
				return modifier;
			}

			// Next modifier stack entry.
			modStackIndex++;
		}
	}

	// Not found.
	return NULL;
}

Modifier* MFXExport::findSkinMod( INode* node )
{
	// Get object from node. Abort if no object.
	Object* object = node->GetObjectRef();

	if( !object )
		return NULL;

	// Is derived object ?
	if( object->SuperClassID() == GEN_DERIVOB_CLASS_ID )
	{
		// Yes -> Cast.
		IDerivedObject* derivedObject = static_cast< IDerivedObject* >( object );

		// Iterate over all entries of the modifier stack.
		int modStackIndex = 0;
		while( modStackIndex < derivedObject->NumModifiers() )
		{
			// Get current modifier.
			Modifier* modifier = derivedObject->GetModifier( modStackIndex );

			// Is this Physique ?
			if( modifier->ClassID() == SKIN_CLASSID )
			{
				// Yes -> Exit.
				return modifier;
			}

			// Next modifier stack entry.
			modStackIndex++;
		}
	}

	// Not found.
	return NULL;
}

/**
 *	Recursively check all node names for duplicates.
 */
void checkDuplicateNodeNames( MFXNode* node, BW::set<BW::string>& nodeNames, BW::set<BW::string>& duplicates )
{
	if (node->include())
	{
		BW::string id = node->getIdentifier();
		if (nodeNames.find( id ) == nodeNames.end())
		{
			nodeNames.insert( id );
		}
		else
		{
			duplicates.insert( id );
		}
	}

	for (int i = 0; i < node->getNChildren(); i++)
	{
		checkDuplicateNodeNames( node->getChild( i ), nodeNames, duplicates );
	}
}

/**
 *	This method validates the node and animation id's
 *	It validates that there are no duplicate node id's,
 *		no duplicate morph target id's and
 *		no morph target and node id's that clash
 *	@param errorMsg the error msg to output
 *	@return true if the animation id's appear valid
 */
bool MFXExport::validateAnimationIDs( BW::string& errorMsg )
{
	bool res = true;
	
	// collect node names and duplicate node names
	BW::set<BW::string> nodeNames;
	BW::set<BW::string> duplicateNodeNames;
	
	if (ExportSettings::instance().exportNodeAnimation())
	{
		checkDuplicateNodeNames( mfxRoot_, nodeNames, duplicateNodeNames );
	}

	// If we have duplicate node names, build a list of errors for them
	if (duplicateNodeNames.size())
	{
		res = false;
		BW::set<BW::string>::iterator it = duplicateNodeNames.begin();
		
		for (; it != duplicateNodeNames.end(); it++)
		{
			errorMsg.append( BW::string( "Duplicate node name " ) + *it + BW::string("\n") );
		}
	}

	return res;
}

std::ostream& operator<<(std::ostream& o, const MFXExport& t)
{
	o << "MFXExport\n";
	return o;
}

BW_END_NAMESPACE
// mfxexp.cpp
