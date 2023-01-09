#ifndef MFXEXP_HPP
#define MFXEXP_HPP

#pragma warning ( disable : 4786 )
#pragma warning ( disable : 4530 )

#include "cstdmf/bw_vector.hpp"
#include <algorithm>

#include "resmgr/bwresource.hpp"
#include "cstdmf/stringmap.hpp"
#include "resmgr/dataresource.hpp"

#include "Max.h"
#include "resource.h"
#include "istdplug.h"
#include "stdmat.h"
#include "decomp.h"
#include "shape.h"
#include "interpik.h"
#include "modstack.h"
#include "phyexp.h"

#include "expsets.hpp"
#include "hull_mesh.hpp"
#include "mfxnode.hpp"
#include "visual_mesh.hpp"
#include "visual_envelope.hpp"
#include "visual_portal.hpp"

// forward declarations from the max sdk.
class IEditNormalsMod;

BW_BEGIN_NAMESPACE

//external functions
extern TCHAR *GetString(int id);
extern HINSTANCE hInstance;

//Version number of exporter * 100
const int MFX_EXPORT_VERSION = 100;

//Global class id.
#if defined BW_EXPORTER_DEBUG
#define MFEXP_CLASS_ID	Class_ID(0x73d06c76, 0xf143737)
#else
#define MFEXP_CLASS_ID	Class_ID(0x793130d, 0x6601416c)
#endif


//Config fileName
const BW::string CFGFilename("visualexporter.cfg");

typedef BW::vector< INode* > INodeVector;

class MaterialList
{
private:

	BW::vector< Mtl* > materials_;

public:
	void addMaterial( Mtl * mtl )
	{
		if( mtl )
		{
			BW::vector< Mtl* >::iterator it = std::find( materials_.begin(), materials_.end(), mtl );
			if( it == materials_.end() )
			{
				materials_.push_back( mtl );
			}
		}
	}

	Mtl *getMaterial( int index ) const
	{
		return materials_[ index ];
	}

	int getNMaterials( void ) const
	{
		return static_cast<int>(materials_.size());
	}


};

typedef struct
{
	INode *node;
	BW::vector< int > vertexIndices;
} Bone;

class BoneList
{
private:

	BW::vector< Bone > bones_;

public:

	/*
	 * Find a bone by node reference
	 */
	Bone *findBone( INode *node )
	{
		for( int i = 0; i < bones_.size(); i++ )
		{
			if( node == bones_[ i ].node )
				return &bones_[ i ];
		}
		return NULL;
	}

	/*
	 * Add a bone to the bone list
	 * If there is a bone attached to the node already, the index gets added to the
	 * vertexIndex list of that bone.
	 */
	void addBone( INode *node, int index )
	{
		Bone *bone = findBone( node );
		if( bone )
		{
			bone->vertexIndices.push_back( index );
		}
		else
		{
			Bone bn;
			bn.node = node;
			bn.vertexIndices.push_back( index );
			bones_.push_back( bn );
		}
	}

	Bone *getBone( int index )
	{
		return &bones_[ index ];
	}

	int getNBones( void ) const
	{
		return static_cast<int>(bones_.size());
	}
};

typedef struct 
{
	Mtl *mtl_;
	BW::vector< BW::string > children_;

} MultiSubMaterial;

class MFXExport : public SceneExport
{
public:
	typedef BW::map<BW::string, int>	BoneCountMap;
	typedef BoneCountMap::iterator		BoneCountMapIt;
	
	MFXExport();
	~MFXExport();

	int				ExtCount();					// Number of extensions supported
	const TCHAR *	Ext(int n);					// Extension #n
	const TCHAR *	LongDesc();					// Long ASCII description
	const TCHAR *	ShortDesc();				// Short ASCII description
	const TCHAR *	AuthorName();				// ASCII Author name
	const TCHAR *	CopyrightMessage();			// ASCII Copyright message
	const TCHAR *	OtherMessage1();			// Other message #1
	const TCHAR *	OtherMessage2();			// Other message #2
	unsigned int	Version();					// Version number * 100
	void			ShowAbout(HWND hWnd);		// Show DLL's "About..." box
	int				DoExport(const TCHAR *name,ExpInterface *ei,Interface *i, BOOL suppressPrompts=FALSE, DWORD options = 0);	// Export file
	BOOL			SupportsOptions( int ext, DWORD options ) { return options == SCENE_EXPORT_SELECTED; }

	static Modifier* findPhysiqueModifier( INode* node );
	static Modifier* findMorphModifier( INode* node );
	static TriObject* getTriObject( INode *node, TimeValue t, bool &needDelete );
	static IEditNormalsMod* findEditNormalsMod( INode* node );
	static class Modifier* findSkinMod( INode* node );

private:
	void preProcess( INode* node, MFXNode* mfxParent = NULL );
//	void loadReferenceNodes( const BW::string & mfxFile );
	/*
	 * Export functions
	 */
	bool exportMaterial( StdMat *mat, bool overrideMaterialName = false, const BW::string &overrideName = "" );
	bool exportMeshes( bool snapVertices );
	void exportMesh( INode *node, bool snapVertices );
	bool exportEnvelopes( BW::vector<BW::string>& errorModels );

	void exportPortals();
	void exportPortal( INode* node );

	void generateHull( DataSectionPtr pVisualSection, const BoundingBox& bb );
	void exportHull( DataSectionPtr pVisualSection );


	/*
	 * Helper functions
	 */
	TimeValue staticFrame( void );
	BW::string getCfgFilename( void );
	MultiSubMaterial *findMultiSubMaterial( Mtl *mtl );
	bool isShell( const BW::string& resName );

	
	void planeFromBoundarySection( const DataSectionPtr pBoundarySection,
		PlaneEq& ret );
	bool portalOnBoundary( const PlaneEq& portal, const PlaneEq& boundary );
	void exportPortalsToBoundaries( DataSectionPtr pVisualSection, const BoundingBox& bb );

	static Point3			applyUnitScale( const Point3& p );
	static Matrix3			applyUnitScale( const Matrix3& m );

	Interface*		ip_; //max Interface

	MaterialList	materials_; //Our list of materials
	unsigned int	nodeCount_; //Number of nodes
	unsigned int	animNodeCount_; //Number of animated nodes

	bool			errors_; //exported with errors
	bool			suppressPrompts_;

	ExportSettings&	settings_;

	INodeVector portalNodes_;
	INodeVector envelopeNodes_;
	INodeVector meshNodes_;

	BW::vector<VisualMeshPtr> visualMeshes_;
	BW::vector<VisualPortalPtr> visualPortals_;
	BW::vector<VisualMeshPtr> bspMeshes_;
	BW::vector<HullMeshPtr> hullMeshes_;


	MFXNode* mfxRoot_;

	MFXExport(const MFXExport&);
	MFXExport& operator=(const MFXExport&);
};

BW_END_NAMESPACE

#ifdef CODE_INLINE
#include "mfxexp.ipp"
#endif

#endif // mfxexp.hpp
