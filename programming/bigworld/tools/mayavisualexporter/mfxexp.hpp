#ifndef MFXEXP_HPP
#define MFXEXP_HPP

#pragma warning ( disable : 4786 )
#pragma warning ( disable : 4530 )

//~ #include <vector>
//~ #include <algorithm>

//~ #include "cstdmf/stringmap.hpp"
//~ #include "resmgr/dataresource.hpp"

//~ #include "Max.h"
//~ #include "resource.h"
//~ #include "istdplug.h"
//~ #include "stdmat.h"
//~ #include "decomp.h"
//~ #include "shape.h"
//~ #include "interpik.h"
//~ #include "modstack.h"
//~ #include "phyexp.h"

//~ #include "expsets.hpp"
//~ #include "mfxfile.hpp"
//~ #include "mfxnode.hpp"
//~ #include "visual_mesh.hpp"
//~ #include "visual_envelope.hpp"
//~ #include "visual_portal.hpp"


//~ //external functions
//~ extern TCHAR *GetString(int id);
//~ extern HINSTANCE hInstance;

//~ //Version number of exporter * 100
//~ const int MFX_EXPORT_VERSION = 100;

//~ //Global class id.
//~ #ifndef BW_EVALUATION
//~ #define MFEXP_CLASS_ID	Class_ID(0x793130d, 0x6601416c)
//~ #else
//~ #define MFEXP_CLASS_ID	Class_ID(0x7cf663bc, 0x6b3a6e7e)
//~ #endif

//~ //Config fileName
//~ const std::string CFGFilename("visualexporter.cfg");

//~ typedef BW::vector< INode* > INodeVector;

//~ class MaterialList
//~ {
//~ private:

	//~ BW::vector< Mtl* > materials_;

//~ public:
	//~ void addMaterial( Mtl * mtl )
	//~ {
		//~ if( mtl )
		//~ {
			//~ BW::vector< Mtl* >::iterator it = std::find( materials_.begin(), materials_.end(), mtl );
			//~ if( it == materials_.end() )
			//~ {
				//~ materials_.push_back( mtl );
			//~ }
		//~ }
	//~ }

	//~ Mtl *getMaterial( int index ) const
	//~ {
		//~ return materials_[ index ];
	//~ }

	//~ int getNMaterials( void ) const
	//~ {
		//~ return materials_.size();
	//~ }


//~ };

//~ typedef struct
//~ {
	//~ INode *node;
	//~ BW::vector< int > vertexIndices;
//~ } Bone;

//~ class BoneList
//~ {
//~ private:

	//~ BW::vector< Bone > bones_;

//~ public:

	//~ /*
	 //~ * Find a bone by node reference
	 //~ */
	//~ Bone *findBone( INode *node )
	//~ {
		//~ for( int i = 0; i < bones_.size(); i++ )
		//~ {
			//~ if( node == bones_[ i ].node )
				//~ return &bones_[ i ];
		//~ }
		//~ return NULL;
	//~ }

	//~ /*
	 //~ * Add a bone to the bone list
	 //~ * If there is a bone attached to the node already, the index gets added to the
	 //~ * vertexIndex list of that bone.
	 //~ */
	//~ void addBone( INode *node, int index )
	//~ {
		//~ Bone *bone = findBone( node );
		//~ if( bone )
		//~ {
			//~ bone->vertexIndices.push_back( index );
		//~ }
		//~ else
		//~ {
			//~ Bone bn;
			//~ bn.node = node;
			//~ bn.vertexIndices.push_back( index );
			//~ bones_.push_back( bn );
		//~ }
	//~ }

	//~ Bone *getBone( int index )
	//~ {
		//~ return &bones_[ index ];
	//~ }

	//~ int getNBones( void ) const
	//~ {
		//~ return bones_.size();
	//~ }
//~ };

//~ typedef struct 
//~ {
	//~ Mtl *mtl_;
	//~ BW::vector< std::string > children_;

//~ } MultiSubMaterial;

//~ class MFXExport : public SceneExport
//~ {
//~ public:
	//~ MFXExport();
	//~ ~MFXExport();

	//~ int				ExtCount();					// Number of extensions supported
	//~ const TCHAR *	Ext(int n);					// Extension #n
	//~ const TCHAR *	LongDesc();					// Long ASCII description
	//~ const TCHAR *	ShortDesc();				// Short ASCII description
	//~ const TCHAR *	AuthorName();				// ASCII Author name
	//~ const TCHAR *	CopyrightMessage();			// ASCII Copyright message
	//~ const TCHAR *	OtherMessage1();			// Other message #1
	//~ const TCHAR *	OtherMessage2();			// Other message #2
	//~ unsigned int	Version();					// Version number * 100
	//~ void			ShowAbout(HWND hWnd);		// Show DLL's "About..." box
	//~ int				DoExport(const TCHAR *name,ExpInterface *ei,Interface *i, BOOL suppressPrompts=FALSE, DWORD options = 0);	// Export file

//~ //	static Matrix3 normaliseMatrix( const Matrix3 &m );
//~ //	static bool isMirrored( const Matrix3 &m );
	//~ static Modifier* findPhysiqueModifier( INode* node );
	//~ static Modifier* findMorphModifier( INode* node );
	//~ static TriObject* getTriObject( INode *node, TimeValue t, bool &needDelete );

//~ private:
	//~ void preProcess( INode* node, MFXNode* mfxParent = NULL );
	//~ void loadReferenceNodes( const std::string & mfxFile );
	//~ /*
	 //~ * Export functions
	 //~ */
	//~ bool exportMaterials( void );
	//~ bool exportMaterial( StdMat *mat, bool overrideMaterialName = false, const std::string &overrideName = "" );
	//~ void exportNodes( void );
	//~ void traverseExportNodes( MFXNode *node );
	//~ void exportMeshes( bool snapVertices );
	//~ void exportMesh( INode *node, bool snapVertices );
	//~ void exportAnimations( void );
	//~ void traverseExportAnimations( MFXNode *node );
	//~ void exportEnvelopes( void );
	//~ void exportEnvelope( INode *node );
	//~ void exportEnvelopeMesh( INode *node, class UniqueVertices &verts );
	//~ void exportBone( IPhyContextExport* contextExport, UniqueVertices &uqvs, Bone *bone, const std::string &envelopeName );
	
	//~ void exportPortals();
	//~ void exportPortal( INode* node );

	//~ void generateHull( DataSectionPtr pVisualSection, const BoundingBox& bb );
	//~ void exportHull( DataSectionPtr pVisualSection );


	//~ /*
	 //~ * Helper functions
	 //~ */
	//~ TimeValue staticFrame( void );
	//~ Point3 getScale( INode * node, TimeValue t );
	//~ Point3 getScale( const Matrix3 &m );
	//~ std::string getCfgFilename( void );
	//~ MultiSubMaterial *findMultiSubMaterial( Mtl *mtl );
	//~ bool isShell( const std::string& resName );

	
	//~ void planeFromBoundarySection( const DataSectionPtr pBoundarySection,
		//~ PlaneEq& ret );
	//~ bool portalOnBoundary( const PlaneEq& portal, const PlaneEq& boundary );
	//~ void exportPortalsToBoundaries( DataSectionPtr pVisualSection );

	//~ MFXNode*		stripEmptyLeafNodes( MFXNode* node );

	//~ Point3			applyUnitScale( const Point3& p );
	//~ Matrix3			applyUnitScale( const Matrix3& m );

	//~ Interface*		ip_; //max Interface

	//~ MaterialList	materials_; //Our list of materials
	//~ unsigned int	nodeCount_; //Number of nodes
	//~ unsigned int	animNodeCount_; //Number of animated nodes

	//~ bool			errors_; //exported with errors

	//~ MFXFile			mfx_;
	//~ ExportSettings&	settings_;

	//~ INodeVector portalNodes_;
	//~ INodeVector envelopeNodes_;
	//~ INodeVector meshNodes_;
	//~ INodeVector touchedNodes_;
	//~ BW::vector< MultiSubMaterial > multiSubs_;

	//~ BW::vector<VisualMeshPtr> visualMeshes_;
	//~ BW::vector<VisualEnvelopePtr> visualEnvelopes_;
	//~ BW::vector<VisualPortalPtr> visualPortals_;
	//~ BW::vector<VisualMeshPtr> bspMeshes_;
	//~ BW::vector<VisualMeshPtr> hullMeshes_;


	//~ BW::vector< MFXNode* > mfxEnvelopeNodes_;
	//~ MFXNode* mfxRoot_;

	//~ StringMap<std::string> nodeParents_;
	
	//~ MFXExport(const MFXExport&);
	//~ MFXExport& operator=(const MFXExport&);
//~ };

#ifdef CODE_INLINE
#include "mfxexp.ipp"
#endif


#endif // mfxexp.hpp
