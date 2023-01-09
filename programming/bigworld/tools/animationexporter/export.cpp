//************************************************************************** 
//* Export.cpp	- MFX File Exporter
//* 
//***************************************************************************

#include "mfexp old.hpp"
#include "chunkids.hpp"

#include "polycont.hpp"
#include "vertcont.hpp"
#include <algorithm>
#include <string>

/****************************************************************************

  Helper functions, MFX export
  
****************************************************************************/

void writeMFXPoint( Point3 &p, FILE *stream )
{
	fwrite( &p.x, 4, 1, stream );
	fwrite( &p.z, 4, 1, stream );
	fwrite( &p.y, 4, 1, stream );
}

void writeMFXUV( Point3 &p, FILE *stream )
{
	fwrite( &p.x, 4, 1, stream );
	fwrite( &p.y, 4, 1, stream );
}

void writeMFXMatrix( Matrix3 &m, FILE *stream )
{
	Point3 p = m.GetRow(0);
	writeMFXPoint( p, stream );
	p = m.GetRow(2);
	writeMFXPoint( p, stream );
	p = m.GetRow(1);
	writeMFXPoint( p, stream );
	p = m.GetRow(3);
	writeMFXPoint( p, stream );
}

void writeMFXChunkHeader( ChunkID identifier, int totalSize, int size, FILE *stream )
{
	fwrite( &identifier, 4, 1, stream );
	fwrite( &totalSize, 4, 1, stream );
	fwrite( &size, 4, 1, stream );
}

int getMFXStrLen( TCHAR *string )
{
	if( string )
		return strlen( string ) + 4;

	return 4;
}

int getMFXStrLen( const std::string &str )
{
/*	if( string )
		return strlen( string ) + 4;*/

//	return 4;

	return str.size() + 4;
}

void writeMFXStr( const std::string &s, FILE *stream )
{
	TCHAR string[400];

	strcpy( string, s.data() );

	int length=0;
	if( string )
	{
		length = strlen( string );
	}

	fwrite( &length, 4, 1, stream );

	if( length )
	{
		fwrite( string, length, 1, stream );
	}

}

std::string readMFXStr( FILE *stream )
{
	char s[80];
	int length;
	fread( &length, 4, 1, stream );
	if( length > 79 )
		return " ";

	fread( s, length, 1, stream );
	s[length] = 0;
	return s;
}



void writeMFXStr( TCHAR * string, FILE *stream )
{
	int length=0;
	if( string )
	{
		length = strlen( string );
	}

	fwrite( &length, 4, 1, stream );

	if( length )
	{
		fwrite( string, length, 1, stream );
	}

}

int findMFXVertexIndex( VCVector &vcv, VertexContainer &vc )
{
	for( int i=0 ;i< vcv.size(); i++ )
	{
		if( vc == vcv[i] )
			return i;
	}
	return i;
}

Matrix3 normaliseMatrix( Matrix3 m )
{
	Point3 p = m.GetRow( 0 );
	p = Normalize( p );
	m.SetRow( 0, p );
	p = m.GetRow( 1 );
	p = Normalize( p );
	m.SetRow( 1, p );
	p = m.GetRow( 2 );
	p = Normalize( p );
	m.SetRow( 2, p );
	return m;
}

/****************************************************************************

  Global output [Scene info]
  
****************************************************************************/

// Dump some global animation information.
void MFExp::ExportGlobalInfo()
{
	Interval range = ip->GetAnimRange();

	struct tm *newtime;
	time_t aclock;

	time( &aclock );
	newtime = localtime(&aclock);

	TSTR today = _tasctime(newtime);	// The date string has a \n appended.
	today.remove(today.length()-1);		// Remove the \n

	// Start with a file identifier and format version
//	fprintf(pStream, "%s\t%d\n", ID_FILEID, VERSION);

	// Text token describing the above as a comment.
//	fprintf(pStream, "%s \"%s  %1.2f - %s\"\n", ID_COMMENT, GetString(IDS_VERSIONSTRING), VERSION / 100.0f, today);

/*	fprintf(pStream, "%s {\n", ID_SCENE);
	fprintf(pStream, "\t%s \"%s\"\n", ID_FILENAME, FixupName(ip->GetCurFileName()));
	fprintf(pStream, "\t%s %d\n", ID_FIRSTFRAME, range.Start() / GetTicksPerFrame());
	fprintf(pStream, "\t%s %d\n", ID_LASTFRAME, range.End() / GetTicksPerFrame());
	fprintf(pStream, "\t%s %d\n", ID_FRAMESPEED, GetFrameRate());
	fprintf(pStream, "\t%s %d\n", ID_TICKSPERFRAME, GetTicksPerFrame());*/
	
	Texmap* env = ip->GetEnvironmentMap();

	Control* ambLightCont;
	Control* bgColCont;
	
	if (env) {
		// Output environment texture map
		DumpTexture(env, Class_ID(0,0), 0, 1.0f, 0);
	}
	else {
		// Output background color
		bgColCont = ip->GetBackGroundController();
		if (bgColCont && bgColCont->IsAnimated()) {
			// background color is animated.
//			fprintf(pStream, "\t%s {\n", ID_ANIMBGCOLOR);
			
			DumpPoint3Keys(bgColCont, 0);
				
//			fprintf(pStream, "\t}\n");
		}
		else {
			// Background color is not animated
			Color bgCol = ip->GetBackGround(GetStaticFrame(), FOREVER);
//			fprintf(pStream, "\t%s %s\n", ID_STATICBGCOLOR, Format(bgCol));
		}
	}
	
	// Output ambient light
	ambLightCont = ip->GetAmbientController();
	if (ambLightCont && ambLightCont->IsAnimated()) {
		// Ambient light is animated.
//		fprintf(pStream, "\t%s {\n", ID_ANIMAMBIENT);
		
		DumpPoint3Keys(ambLightCont, 0);
		
//		fprintf(pStream, "\t}\n");
	}
	else {
		// Ambient light is not animated
		Color ambLight = ip->GetAmbient(GetStaticFrame(), FOREVER);
//		fprintf(pStream, "\t%s %s\n", ID_STATICAMBIENT, Format(ambLight));
	}

//	fprintf(pStream,"}\n");
}
/****************************************************************************

  GeomObject output
  
****************************************************************************/

void MFExp::ExportGeomObject(INode* node, int indentLevel)
{
	ObjectState os = node->EvalWorldState(GetStaticFrame());
	if (!os.obj)
		return;
	
	// Targets are actually geomobjects, but we will export them
	// from the camera and light objects, so we skip them here.
	if (os.obj->ClassID() == Class_ID(TARGET_CLASS_ID, 0))
		return;
	
	
	TSTR indent = GetIndent(indentLevel);
	
	ExportNodeHeader(node, ID_GEOMETRY, indentLevel);
	
	ExportNodeTM(node, indentLevel);
	
	if (GetIncludeMesh()) {
		ExportMesh(node, GetStaticFrame(), indentLevel);
	}

	// Node properties (only for geomobjects)
/*	fprintf(pStream, "%s\t%s %d\n", indent.data(), ID_PROP_MOTIONBLUR, node->MotBlur());
	fprintf(pStream, "%s\t%s %d\n", indent.data(), ID_PROP_CASTSHADOW, node->CastShadows());
	fprintf(pStream, "%s\t%s %d\n", indent.data(), ID_PROP_RECVSHADOW, node->RcvShadows());*/

/*	if (GetIncludeAnim()) {
		ExportAnimKeys(node, indentLevel);
	}*/

	// Export the visibility track
	Control* visCont = node->GetVisController();
	if (visCont) {
//		fprintf(pStream, "%s\t%s {\n", indent.data(), ID_VISIBILITY_TRACK);
		DumpFloatKeys(visCont, indentLevel);
//		fprintf(pStream, "\t}\n");
	}

	if (GetIncludeMtl()) {
		ExportMaterial(node, indentLevel);
	}

/*	if (GetIncludeMeshAnim()) {
		ExportAnimMesh(node, indentLevel);
	}
	
	if (GetIncludeIKJoints()) {
		ExportIKJoints(node, indentLevel);
	}*/
	
//	fprintf(pStream,"%s}\n", indent.data());
}

/****************************************************************************

  Shape output
  
****************************************************************************/

void MFExp::ExportShapeObject(INode* node, int indentLevel)
{
	ExportNodeHeader(node, ID_SHAPE, indentLevel);
	ExportNodeTM(node, indentLevel);
	TimeValue t = GetStaticFrame();
	Matrix3 tm = node->GetObjTMAfterWSM(t);

	TSTR indent = GetIndent(indentLevel);
	
	ObjectState os = node->EvalWorldState(t);
	if (!os.obj || os.obj->SuperClassID()!=SHAPE_CLASS_ID) {
//		fprintf(pStream,"%s}\n", indent.data());
		return;
	}
	
	ShapeObject* shape = (ShapeObject*)os.obj;
	PolyShape pShape;
	int numLines;

	// We will output ahspes as a collection of polylines.
	// Each polyline contains collection of line segments.
	shape->MakePolyShape(t, pShape);
	numLines = pShape.numLines;
	
//	fprintf(pStream,"%s\t%s %d\n", indent.data(), ID_SHAPE_LINECOUNT, numLines);
	
	for(int poly = 0; poly < numLines; poly++) {
//		fprintf(pStream,"%s\t%s %d {\n", indent.data(), ID_SHAPE_LINE, poly);
		DumpPoly(&pShape.lines[poly], tm, indentLevel);
//		fprintf(pStream, "%s\t}\n", indent.data());
	}
	
	if (GetIncludeAnim()) {
		ExportAnimKeys(node, indentLevel);
	}
	
//	fprintf(pStream,"%s}\n", indent.data());
}

void MFExp::DumpPoly(PolyLine* line, Matrix3 tm, int indentLevel)
{
	int numVerts = line->numPts;
	
	TSTR indent = GetIndent(indentLevel);
	
	if(line->IsClosed()) {
//		fprintf(pStream,"%s\t\t%s\n", indent.data(), ID_SHAPE_CLOSED);
	}
	
//	fprintf(pStream,"%s\t\t%s %d\n", indent.data(), ID_SHAPE_VERTEXCOUNT, numVerts);
	
	// We differ between true and interpolated knots
	for (int i=0; i<numVerts; i++) {
		PolyPt* pt = &line->pts[i];
		if (pt->flags & POLYPT_KNOT) {
/*			fprintf(pStream,"%s\t\t%s\t%d\t%s\n", indent.data(), ID_SHAPE_VERTEX_KNOT, i,
				Format(tm * pt->p));*/
		}
		else {
/*			fprintf(pStream,"%s\t\t%s\t%d\t%s\n", indent.data(), ID_SHAPE_VERTEX_INTERP,
				i, Format(tm * pt->p));*/
		}
		
	}
}

/************************************************************************

  MFX bone Output

************************************************************************/

void MFExp::mfxCreateBones( INode *node )
{
	std::string identifier = node->GetName();

	if( ( identifier.substr( 0, bonePre_.size() ) == bonePre_ ) && ( !node->IsHidden() ) )
	{
		ObjectState os = node->EvalWorldState(GetStaticFrame());
		if (os.obj && (os.obj->SuperClassID()==GEOMOBJECT_CLASS_ID) ) 
		{
			BOOL needDel;
			TriObject* tri = GetTriObjectFromNode(node, GetStaticFrame(), needDel);
			if (tri) 
			{
				Mesh* mesh = &tri->mesh;
				if( mesh->getNumFaces() )
				{
					BoneContainer bc;
					bc.node_ = node;

					int iV1, iV2, iV3;

					Matrix3 m = node->GetObjTMAfterWSM( GetStaticFrame() );

					if( TMNegParity( m ) )
					{
						iV1 = 2;
						iV2 = 1;
						iV3 = 0;
					}
					else
					{
						iV1 = 0;
						iV2 = 1;
						iV3 = 2;
					}
					for( int i = 0; i < mesh->getNumFaces(); i++)
					{
						BoneFace bf;
						bf.p1_ = mesh->verts[ mesh->faces[i].v[iV1] ] * m;
						bf.p2_ = mesh->verts[ mesh->faces[i].v[iV2] ] * m;
						bf.p3_ = mesh->verts[ mesh->faces[i].v[iV3] ] * m;

/*						bf.p1_.x = 0; bf.p1_.y = 0;	bf.p1_.z = 0;
						bf.p2_.x = 0; bf.p2_.y = 0;	bf.p2_.z = 1;
						bf.p3_.x = 1; bf.p3_.y = 0;	bf.p3_.z = 0;*/

						Point3 p1 = bf.p2_ - bf.p1_;
						Point3 p2 = bf.p3_ - bf.p1_;

						bf.peqN_ = CrossProd( p1, p2 );
						bf.peqN_ = Normalize( bf.peqN_ );
						bf.peqD_ = DotProd( bf.peqN_, bf.p1_ );

						p1 = bf.p2_ - bf.p1_;
						bf.l1N_ = CrossProd( p1, bf.peqN_ );
						bf.l1N_ = Normalize( bf.l1N_ );
						bf.l1D_ = DotProd( bf.l1N_, bf.p1_ );

						p1 = bf.p3_ - bf.p2_;
						bf.l2N_ = CrossProd( p1, bf.peqN_ );
						bf.l2N_ = Normalize( bf.l2N_ );
						bf.l2D_ = DotProd( bf.l2N_, bf.p2_ );

						p1 = bf.p1_ - bf.p3_;
						bf.l3N_ = CrossProd( p1, bf.peqN_ );
						bf.l3N_ = Normalize( bf.l3N_ );
						bf.l3D_ = DotProd( bf.l3N_, bf.p3_ );

						
						bc.faces_.push_back( bf );
					
					}
					boneList_.push_back( bc );
				}
			}

			if( needDel )
				delete tri;

		}
	}

	for( int childN = 0; childN < node->NumberOfChildren(); childN++ )
	{
		mfxCreateBones( node->GetChildNode(childN) );
	}
}

typedef BW::vector < int > intVec;

typedef struct Triangle_
{
	int iV1, iV2, iV3; //vertex indexes
}Triangle;

typedef struct EnvelopeBone_
{
	std::string identifier_;
	int nVertices_;
	int firstVertex_;
	INode *boneParent_;
	intVec tIndices_; //these triangles belong to this bone
}EnvelopeBone;

typedef struct EnvelopeContainer_
{
	BW::vector < EnvelopeBone > bones_;
	std::string identifier_;
	BW::vector < VertexContainer > vertices_;
	BW::vector < Triangle > triangles_;
	intVec tIndices_; // these triangles have vertices that belong to different bones
}EnvelopeContainer;

void MFExp::mfxCreateEnvelopes( INode *node )
{
	std::string identifier = node->GetName();

	if( ( identifier.substr( 0, envPre_.size() ) == envPre_ ) && ( !node->IsHidden() ) )
	{
		ObjectState os = node->EvalWorldState(GetStaticFrame());
		if (os.obj && (os.obj->SuperClassID()==GEOMOBJECT_CLASS_ID) ) 
		{
			BOOL needDel;
			TriObject* triObj = GetTriObjectFromNode(node, GetStaticFrame(), needDel);
			if (triObj)
			{	

				Mesh* mesh = &triObj->mesh;
				if( mesh->getNumVerts() && mesh->getNumFaces() )
				{

					//create vertexlist, from face/tface list
					VCVector vertices;
					BOOL hasTCoords = FALSE;
					if( GetIncludeTextureCoords() ) 
					{
						int numTVx = mesh->getNumTVerts();
						if ( numTVx && mesh->tvFace )
						{
							hasTCoords = TRUE;
						}
					}

					for(int i=0 ; i < mesh->getNumFaces() ; i++ )
					{
						for( int j=0 ; j < 3 ; j++)
						{
							VertexContainer vc( mesh->faces[i].v[j], hasTCoords ? mesh->tvFace[i].t[j] : 0 );
							VCVector::iterator itVC = std::find( vertices.begin(), vertices.end(), vc );

							if( itVC == vertices.end( ) )
							{
								vertices.push_back( vc );
							}
						}
					}

					//remove all traces of last exported bone
					for(i = 0; i < boneList_.size(); i++ )
					{
						boneList_[i].vIndices_.erase( boneList_[i].vIndices_.begin(), boneList_[i].vIndices_.end() );
					}

					//find closest bone to each vertex
					Matrix3 m = node->GetObjTMAfterWSM( GetStaticFrame() );
					for( i = 0; i < vertices.size(); i++ )
					{
						Point3 p = mesh->verts[ vertices[i].getV() ] * m;
						float distance = getDistanceToBone( boneList_.front(), p );
						BoneContainer *bc = &boneList_[ 0 ] ;
						

						for( int j = 0;j < boneList_.size(); j++ )
						{
							float thisdist = getDistanceToBone( boneList_[j], p );

							if( distance > thisdist )
							{
								distance = thisdist;
								bc = &boneList_[ j ];
							}
						}
						bc->vIndices_.push_back( i );
					}

					//create the real vertex list, ordered by bone.
					EnvelopeContainer env;
					env.identifier_ = node->GetName();
					env.identifier_ += "envelope";

					for( i=0; i < boneList_.size(); i++ )
					{
						BoneContainer &bc = boneList_[i];
						if( bc.vIndices_.size() > 0 )
						{
							EnvelopeBone bone;
							bone.boneParent_ = bc.node_;
							bone.identifier_ = node->GetName();
							bone.identifier_ += bone.boneParent_->GetName();
							bone.identifier_ += "bone";

							bone.firstVertex_= env.vertices_.size();
							bone.nVertices_ = bc.vIndices_.size();

							for( intVec::iterator iit = bc.vIndices_.begin(); iit != bc.vIndices_.end(); iit++ )
							{
								env.vertices_.push_back( vertices[ *iit ] );
							}

							env.bones_.push_back( bone );
						}
					}
					
					//since we have to transform the envelope to max-worldspace we have to determine
					//if the polygons will be counter clockwise or clockwise, in max-space.
					int iV1, iV2, iV3;
					if( TMNegParity( m ) )
					{
						iV1 = 0;
						iV2 = 1;
						iV3 = 2;
					}
					else
					{
						iV1 = 2;
						iV2 = 1;
						iV3 = 0;
					}

					//create the trianglelist according to our new vertexlist.
					for( i = 0; i < mesh->getNumFaces() ; i++ )
					{
						VertexContainer vc( mesh->faces[i].v[iV1], hasTCoords ? mesh->tvFace[i].t[iV1] : 0 );
						VertexContainer vc2( mesh->faces[i].v[iV2], hasTCoords ? mesh->tvFace[i].t[iV2] : 0 );
						VertexContainer vc3( mesh->faces[i].v[iV3], hasTCoords ? mesh->tvFace[i].t[iV3] : 0 );
						Triangle tri;

						tri.iV1 = findMFXVertexIndex( env.vertices_, vc );
						tri.iV2 = findMFXVertexIndex( env.vertices_, vc2 );
						tri.iV3 = findMFXVertexIndex( env.vertices_, vc3 );

						env.triangles_.push_back( tri );

					}

					//add triangle indices to bones, or to the envelope, depending on where they belong.

					for( i = 0; i < env.triangles_.size(); i++ )
					{
						intVec *iv = &env.tIndices_;
						
						Triangle &tri = env.triangles_[i];

						for( BW::vector < EnvelopeBone >::iterator bIt = env.bones_.begin(); bIt != env.bones_.end(); bIt ++ )
						{
							int i1 = bIt->firstVertex_;
							int i2 = bIt->firstVertex_ + bIt->nVertices_ ;
							if( ( tri.iV1 >= i1 && tri.iV1 < i2 ) &&
								( tri.iV2 >= i1 && tri.iV2 < i2 ) &&
								( tri.iV3 >= i1 && tri.iV3 < i2 ) )
							{
								iv = &bIt->tIndices_;
							}
						}
						
						iv->push_back( i );
					}

					//export envelope

					Mtl* nodeMtl = node->GetMtl();

					TCHAR * matId = NULL;

					if( nodeMtl )
						matId = nodeMtl->GetName();


					int size = CHUNK_HEADER_SIZE;
					size += 4; //nVertices
					size += 4; //nTriangles
					size += 4; //nTextureCoordinates
					size += getMFXStrLen( env.identifier_ ); //identifier
					size += getMFXStrLen( matId ); //material
					size += 4; //nEnvelopePolygons
					size += env.tIndices_.size() * 4; //envelopePolyIndices

					int nVertices = env.vertices_.size();
					int nFaces = env.triangles_.size();

					int verticesSize = CHUNK_HEADER_SIZE + ( nVertices * 3 * 4 ); //size of vertices;
					int tVerticesSize = hasTCoords ? ( CHUNK_HEADER_SIZE + ( nVertices * 2 * 4 ) ) : 0 ; //size of texture coordinates;
					int trianglesSize = CHUNK_HEADER_SIZE + ( nFaces * 3 * 4 ); //size of triangles;

					int totalSize = size + verticesSize + tVerticesSize + trianglesSize;

					writeMFXChunkHeader( CHUNKID_ENVELOPE, totalSize, size, pStream );
					fwrite( &nVertices, 4, 1, pStream );
					fwrite( &nFaces, 4, 1, pStream );
					int nTc = hasTCoords ? 1 : 0;
					fwrite( &nTc, 4, 1, pStream );

					writeMFXStr( env.identifier_, pStream );
					writeMFXStr( matId , pStream );

					int nEnvelopePolys = env.tIndices_.size();
					fwrite( &nEnvelopePolys, 4, 1, pStream );

					for( i = 0; i < nEnvelopePolys; i++ )
					{
						fwrite( &env.tIndices_[i], 4, 1, pStream );
					}
					

					writeMFXChunkHeader( CHUNKID_VERTEXLIST, verticesSize, verticesSize, pStream );
					for( i=0 ; i < nVertices ; i++ )
					{
						Point3 v = mesh->verts[ env.vertices_[i].getV() ] * m;
						writeMFXPoint( v, pStream );
					}

					if( hasTCoords )
					{
						writeMFXChunkHeader( CHUNKID_TEXCOORDLIST, tVerticesSize, tVerticesSize, pStream );
						for( i=0 ; i < nVertices ; i++ )
						{
							UVVert tv = mesh->tVerts[ env.vertices_[i].getTV() ];
							writeMFXUV( tv , pStream );
						}
					}

					writeMFXChunkHeader( CHUNKID_TRIANGLELIST, trianglesSize, trianglesSize, pStream );
					for( i=0 ; i < nFaces ; i++ )
					{
						fwrite( &env.triangles_[i].iV1, 4, 1, pStream );
						fwrite( &env.triangles_[i].iV2, 4, 1, pStream );
						fwrite( &env.triangles_[i].iV3, 4, 1, pStream );

					}

					overrideRoot_ = TRUE;
					overrideRootIds_.push_back( env.identifier_ );
					//export bones
					for( BW::vector < EnvelopeBone >::iterator bIt = env.bones_.begin(); bIt != env.bones_.end(); bIt ++ )
					{
						int size = CHUNK_HEADER_SIZE;
						size += getMFXStrLen( bIt->identifier_ ); //identifier
						size += getMFXStrLen( env.identifier_ ); //envelope
						size += 4; //firstVertexIndex
						size += 4; //nVertices
						size += 4; //nEnvelopePolys
						size += 4 * bIt->tIndices_.size();

						writeMFXChunkHeader( CHUNKID_BONE, size, size, pStream );

						writeMFXStr( bIt->identifier_, pStream );
						writeMFXStr( env.identifier_, pStream );

						fwrite( &bIt->firstVertex_, 4, 1, pStream );
						fwrite( &bIt->nVertices_, 4, 1, pStream );
						int nEnvelopePolys = bIt->tIndices_.size();
						fwrite( &nEnvelopePolys, 4, 1, pStream );

						for( i = 0; i < bIt->tIndices_.size(); i++ )
						{
							fwrite( &bIt->tIndices_[i], 4, 1, pStream );
						}

						NodeOverride no;
						no.nodeId_ = bIt->identifier_;
						no.parentId_ = bIt->boneParent_->GetName();
						if( GetUseCharacterMode() )
						{
							no.transform_ = Inverse( normaliseMatrix( bIt->boneParent_->GetObjTMAfterWSM( GetStaticFrame() ) ) );
						}
						else
						{
							no.transform_ = Inverse( bIt->boneParent_->GetObjTMAfterWSM( GetStaticFrame() ) );
						}
						nodeOverrides_.push_back( no );
					}

				}
			}

			if( needDel )
				delete triObj;
		}
	}

	for( int childN = 0; childN < node->NumberOfChildren(); childN++ )
	{
		mfxCreateEnvelopes( node->GetChildNode(childN) );
	}
}

float MFExp::getDistanceToBone( BoneContainer &bc, Point3 &p )
{
	float ret = Length( p - bc.faces_[0].p1_ );

	for( BW::vector < BoneFace >::iterator iBF = bc.faces_.begin(); iBF != bc.faces_.end(); iBF++ )
	{	
		//is our point within our three lines, if so, calculate the distance to the plane
		if( ( DotProd( iBF->l1N_ , p  ) < iBF->l1D_ ) && ( DotProd( iBF->l2N_ , p ) < iBF->l2D_ ) && ( DotProd( iBF->l3N_ , p ) < iBF->l3D_ ) )
		{
			float len = fabs( DotProd( p, iBF->peqN_ ) - iBF->peqD_ );

			if(ret > len )
				ret = len;
		}
		else
		{
			//is our point within the first line of the triangle, if it is, calculate the distance to the line
			//if not calculate the distance to the first point in the line
			Point3 v = iBF->p2_ - iBF->p1_;
			Point3 v2 = p - iBF->p1_;

			float t = ( DotProd( v, v2 ) / DotProd( v, v ) );
			if( t >= 0 && t <= 1 )
			{
				float len = Length( v2 - ( v * t ) );
				if( ret > len )
				{
					ret = len;
				}
			}
			else
			{
				float len = Length( v2 );
				if( ret > len )
				{
					ret = len;
				}
			}

			//is our point within the second line of the triangle, if it is, calculate the distance to the line
			//if not calculate the distance to the first point in the line
			v = iBF->p3_ - iBF->p2_;
			v2 = p - iBF->p2_;

			t = ( DotProd( v, v2 ) / DotProd( v, v ) );
			if( t >= 0 && t <= 1 )
			{
				float len = Length( v2 - ( v * t ) );

				if( ret > len )
				{
					ret = len;
				}
			}
			else
			{
				float len = Length( v2 );
				if( ret > len )
				{
					ret = len;
				}
			}

			//is our point within the third line of the triangle, if it is, calculate the distance to the line
			//if not calculate the distance to the first point in the line
			v = iBF->p1_ - iBF->p3_;
			v2 = p - iBF->p3_;

			t = ( DotProd( v, v2 ) / DotProd( v, v ) );
			if( t >= 0 && t <= 1 )
			{
				float len = Length( v2 - ( v * t ) );

				if( ret > len )
				{
					ret = len;
				}
			}
			else
			{
				float len = Length( v2 );
				if( ret > len )
				{
					ret = len;
				}
			}
		}

	}

	return ret;
}

/************************************************************************

  MFX animation output

************************************************************************/

void MFExp::mfxAnimationExport( INode *root )
{
	std::string id = GetAnimID();
	int size = 0;


	size += CHUNK_HEADER_SIZE;
	size += getMFXStrLen( id );
	size += 8; //two ints

	writeMFXChunkHeader( CHUNKID_ANIMATION, size, size, pStream );
	writeMFXStr( id, pStream );
	fwrite( &nTotalNodeCount, 4, 1, pStream );

	Interval range = ip->GetAnimRange();
	int time = ( range.End() - range.Start() ) / GetTicksPerFrame();

	fwrite( &time, 4, 1, pStream );

	mfxAnimationNodeEnum( root );
}
void MFExp::mfxAnimationNodeEnum( INode *node )
{
	Interval range = ip->GetAnimRange();
	int time = ( range.End() - range.Start() ) / GetTicksPerFrame();
	int tpf = GetTicksPerFrame();
	int cf = range.Start();
	TCHAR *name = node->GetName();

	int size = CHUNK_HEADER_SIZE;
	size += getMFXStrLen( name );
	size += 4;

	writeMFXChunkHeader( CHUNKID_ANIMATIONCHANNEL, size, size, pStream );
	fwrite( &time, 4, 1, pStream );
	writeMFXStr( name, pStream );

	size = 4 * 3 *4; //sizeof matrix34
	size += 4; // size of int
	
	size *= time;
	
	size += CHUNK_HEADER_SIZE;

	writeMFXChunkHeader( CHUNKID_KEYFRAMEMATRIX34, size, size, pStream );

	INode *parent = node->GetParentNode();

	for( int i = 0; i < time; i++ )
	{
		Matrix3 nodeMatrix = node->GetObjectTM( cf );

		if( GetUseCharacterMode() )
		{
			nodeMatrix = normaliseMatrix( nodeMatrix );
		}

		if( parent )
		{
//			parentId = parent->GetName();
			Matrix3 parentMatrix;
			if( GetUseCharacterMode() )
			{
				parentMatrix = Inverse( normaliseMatrix( parent->GetObjectTM( cf ) ) );
			}
			else
			{
				parentMatrix = Inverse( parent->GetObjectTM( cf ) );
			}

			nodeMatrix = nodeMatrix * parentMatrix;
		}
		
		cf += tpf;

		fwrite( &i, 4, 1, pStream );
		writeMFXMatrix( nodeMatrix, pStream );
	}

	for (int idx=0; idx < node->NumberOfChildren(); idx++) {
		mfxAnimationNodeEnum( node->GetChildNode(idx) );
	}

}


/****************************************************************************

  Light output
  
****************************************************************************/

void MFExp::ExportLightObject(INode* node, int indentLevel)
{
	TimeValue t = GetStaticFrame();
	TSTR indent = GetIndent(indentLevel);

	ExportNodeHeader(node, ID_LIGHT, indentLevel);
	
	ObjectState os = node->EvalWorldState(t);
	if (!os.obj) {
//		fprintf(pStream, "%s}\n", indent.data());
		return;
	}
	
	GenLight* light = (GenLight*)os.obj;
	struct LightState ls;
	Interval valid = FOREVER;
	Interval animRange = ip->GetAnimRange();

	light->EvalLightState(t, valid, &ls);

	// This is part os the lightState, but it doesn't
	// make sense to output as an animated setting so
	// we dump it outside of ExportLightSettings()

//	fprintf(pStream, "%s\t%s ", indent.data(), ID_LIGHT_TYPE);
	switch(ls.type) {
	case OMNI_LIGHT:  /*fprintf(pStream, "%s\n", ID_LIGHT_TYPE_OMNI);*/ break;
	case TSPOT_LIGHT: /*fprintf(pStream, "%s\n", ID_LIGHT_TYPE_TARG);*/  break;
	case DIR_LIGHT:   /*fprintf(pStream, "%s\n", ID_LIGHT_TYPE_DIR);*/ break;
	case FSPOT_LIGHT: /*fprintf(pStream, "%s\n", ID_LIGHT_TYPE_FREE);*/ break;
	}

	ExportNodeTM(node, indentLevel);
	// If we have a target object, export Node TM for the target too.
	INode* target = node->GetTarget();
	if (target) {
		ExportNodeTM(target, indentLevel);
	}

	int shadowMethod = light->GetShadowMethod();
/*	fprintf(pStream,"%s\t%s %s\n", indent.data(), ID_LIGHT_SHADOWS,
			shadowMethod == LIGHTSHADOW_NONE ? ID_LIGHT_SHAD_OFF :
			shadowMethod == LIGHTSHADOW_MAPPED ? ID_LIGHT_SHAD_MAP :
			ID_LIGHT_SHAD_RAY);*/

	
//	fprintf(pStream,"%s\t%s %s\n", indent.data(), ID_LIGHT_USELIGHT, Format(light->GetUseLight()));
	
/*	fprintf(pStream,"%s\t%s %s\n", indent.data(), ID_LIGHT_SPOTSHAPE, 
		light->GetSpotShape() == RECT_LIGHT ? ID_LIGHT_SHAPE_RECT : ID_LIGHT_SHAPE_CIRC);*/

/*	fprintf(pStream,"%s\t%s %s\n", indent.data(), ID_LIGHT_USEGLOBAL, Format(light->GetUseGlobal()));
	fprintf(pStream,"%s\t%s %s\n", indent.data(), ID_LIGHT_ABSMAPBIAS, Format(light->GetAbsMapBias()));
	fprintf(pStream,"%s\t%s %s\n", indent.data(), ID_LIGHT_OVERSHOOT, Format(light->GetOvershoot()));*/

	NameTab* el = light->GetExclList();
	if (el->Count()) {
/*		fprintf(pStream,"%s\t%s {\n", indent.data(), ID_LIGHT_EXCLUSIONLIST);
		fprintf(pStream,"%s\t\t%s %s\n", indent.data(), ID_LIGHT_NUMEXCLUDED, Format(el->Count()));
		fprintf(pStream,"%s\t\t%s %s\n", indent.data(), ID_LIGHT_EXCLINCLUDE, Format(el->TestFlag(NT_INCLUDE)));
		fprintf(pStream,"%s\t\t%s %s\n", indent.data(), ID_LIGHT_EXCL_AFFECT_ILLUM, Format(el->TestFlag(NT_AFFECT_ILLUM)));
		fprintf(pStream,"%s\t\t%s %s\n", indent.data(), ID_LIGHT_EXCL_AFFECT_SHAD, Format(el->TestFlag(NT_AFFECT_SHADOWCAST)));*/
		for (int nameid = 0; nameid < el->Count(); nameid++) {
//			fprintf(pStream,"%s\t\t%s \"%s\"\n", indent.data(), ID_LIGHT_EXCLUDED, (*el)[nameid]);
		}
//		fprintf(pStream,"%s\t}\n", indent.data());
	}

	// Export light settings for frame 0
	ExportLightSettings(&ls, light, t, indentLevel);

	// Export animated light settings
	if (!valid.InInterval(animRange) && GetIncludeCamLightAnim()) {
//		fprintf(pStream,"%s\t%s {\n", indent.data(), ID_LIGHT_ANIMATION);

		TimeValue t = animRange.Start();
		
		while (1) {
			valid = FOREVER; // Extend the validity interval so the camera can shrink it.
			light->EvalLightState(t, valid, &ls);

			t = valid.Start() < animRange.Start() ? animRange.Start() : valid.Start();
			
			// Export the light settings at this frame
			ExportLightSettings(&ls, light, t, indentLevel+1);
			
			if (valid.End() >= animRange.End()) {
				break;
			}
			else {
				t = (valid.End()/GetTicksPerFrame()+GetMeshFrameStep()) * GetTicksPerFrame();
			}
		}

//		fprintf(pStream,"%s\t}\n", indent.data());
	}

	// Export animation keys for the light node
	if (GetIncludeAnim()) {
		ExportAnimKeys(node, indentLevel);
		
		// If we have a target, export animation keys for the target too.
		if (target) {
			ExportAnimKeys(target, indentLevel);
		}
	}
	
//	fprintf(pStream,"%s}\n", indent.data());
}

void MFExp::ExportLightSettings(LightState* ls, GenLight* light, TimeValue t, int indentLevel)
{
	TSTR indent = GetIndent(indentLevel);

//	fprintf(pStream,"%s\t%s {\n", indent.data(), ID_LIGHT_SETTINGS);

	// Frame #
/*	fprintf(pStream, "%s\t\t%s %d\n",indent.data(), ID_TIMEVALUE, t);

	fprintf(pStream,"%s\t\t%s %s\n", indent.data(), ID_LIGHT_COLOR, Format(ls->color));
	fprintf(pStream,"%s\t\t%s %s\n", indent.data(), ID_LIGHT_INTENS, Format(ls->intens));
	fprintf(pStream,"%s\t\t%s %s\n", indent.data(), ID_LIGHT_ASPECT, Format(ls->aspect));*/
	
	if (ls->type != OMNI_LIGHT) {
/*		fprintf(pStream,"%s\t\t%s %s\n", indent.data(), ID_LIGHT_HOTSPOT, Format(ls->hotsize));
		fprintf(pStream,"%s\t\t%s %s\n", indent.data(), ID_LIGHT_FALLOFF, Format(ls->fallsize));*/
	}
	if (ls->type != DIR_LIGHT && ls->useAtten) {
/*		fprintf(pStream,"%s\t\t%s %s\n", indent.data(), ID_LIGHT_ATTNSTART, Format(ls->attenStart));
		fprintf(pStream,"%s\t\t%s %s\n", indent.data(), ID_LIGHT_ATTNEND,	Format(ls->attenEnd));*/
	}

/*	fprintf(pStream,"%s\t\t%s %s\n", indent.data(), ID_LIGHT_TDIST, Format(light->GetTDist(t, FOREVER)));
	fprintf(pStream,"%s\t\t%s %s\n", indent.data(), ID_LIGHT_MAPBIAS, Format(light->GetMapBias(t, FOREVER)));
	fprintf(pStream,"%s\t\t%s %s\n", indent.data(), ID_LIGHT_MAPRANGE, Format(light->GetMapRange(t, FOREVER)));
	fprintf(pStream,"%s\t\t%s %s\n", indent.data(), ID_LIGHT_MAPSIZE, Format(light->GetMapSize(t, FOREVER)));
	fprintf(pStream,"%s\t\t%s %s\n", indent.data(), ID_LIGHT_RAYBIAS, Format(light->GetRayBias(t, FOREVER)));

	fprintf(pStream,"%s\t}\n", indent.data());*/
}


/****************************************************************************

  Camera output
  
****************************************************************************/

void MFExp::ExportCameraObject(INode* node, int indentLevel)
{
	TSTR indent = GetIndent(indentLevel);

	ExportNodeHeader(node, ID_CAMERA, indentLevel);

	INode* target = node->GetTarget();
	if (target) {
//		fprintf(pStream,"%s\t%s %s\n", indent.data(), ID_CAMERA_TYPE, ID_CAMERATYPE_TARGET);
	}
	else {
//		fprintf(pStream,"%s\t%s %s\n", indent.data(), ID_CAMERA_TYPE, ID_CAMERATYPE_FREE);
	}


	ExportNodeTM(node, indentLevel);
	// If we have a target object, export animation keys for the target too.
	if (target) {
		ExportNodeTM(target, indentLevel);
	}
	
	CameraState cs;
	TimeValue t = GetStaticFrame();
	Interval valid = FOREVER;
	// Get animation range
	Interval animRange = ip->GetAnimRange();
	
	ObjectState os = node->EvalWorldState(t);
	CameraObject *cam = (CameraObject *)os.obj;
	
	cam->EvalCameraState(t,valid,&cs);
	
	ExportCameraSettings(&cs, cam, t, indentLevel);

	// Export animated camera settings
	if (!valid.InInterval(animRange) && GetIncludeCamLightAnim()) {

//		fprintf(pStream,"%s\t%s {\n", indent.data(), ID_CAMERA_ANIMATION);

		TimeValue t = animRange.Start();
		
		while (1) {
			valid = FOREVER; // Extend the validity interval so the camera can shrink it.
			cam->EvalCameraState(t,valid,&cs);

			t = valid.Start() < animRange.Start() ? animRange.Start() : valid.Start();
			
			// Export the camera settings at this frame
			ExportCameraSettings(&cs, cam, t, indentLevel+1);
			
			if (valid.End() >= animRange.End()) {
				break;
			}
			else {
				t = (valid.End()/GetTicksPerFrame()+GetMeshFrameStep()) * GetTicksPerFrame();
			}
		}

//		fprintf(pStream,"%s\t}\n", indent.data());
	}
	
	// Export animation keys for the light node
	if (GetIncludeAnim()) {
		ExportAnimKeys(node, indentLevel);
		
		// If we have a target, export animation keys for the target too.
		if (target) {
			ExportAnimKeys(target, indentLevel);
		}
	}
	
//	fprintf(pStream,"%s}\n", indent.data());
}

void MFExp::ExportCameraSettings(CameraState* cs, CameraObject* cam, TimeValue t, int indentLevel)
{
	TSTR indent = GetIndent(indentLevel);

//	fprintf(pStream,"%s\t%s {\n", indent.data(), ID_CAMERA_SETTINGS);

	// Frame #
//	fprintf(pStream, "%s\t\t%s %d\n", indent.data(), ID_TIMEVALUE, t);

	if (cs->manualClip) {
/*		fprintf(pStream,"%s\t\t%s %s\n", indent.data(), ID_CAMERA_HITHER, Format(cs->hither));
		fprintf(pStream,"%s\t\t%s %s\n", indent.data(), ID_CAMERA_YON, Format(cs->yon));*/
	}

/*	fprintf(pStream,"%s\t\t%s %s\n", indent.data(), ID_CAMERA_NEAR, Format(cs->nearRange));
	fprintf(pStream,"%s\t\t%s %s\n", indent.data(), ID_CAMERA_FAR, Format(cs->farRange));
	fprintf(pStream,"%s\t\t%s %s\n", indent.data(), ID_CAMERA_FOV, Format(cs->fov));
	fprintf(pStream,"%s\t\t%s %s\n", indent.data(), ID_CAMERA_TDIST, Format(cam->GetTDist(t)));

	fprintf(pStream,"%s\t}\n",indent.data());*/
}


/****************************************************************************

  Helper object output
  
****************************************************************************/

void MFExp::ExportHelperObject(INode* node, int indentLevel)
{
	TSTR indent = GetIndent(indentLevel);
	ExportNodeHeader(node, ID_HELPER, indentLevel);

	// We don't really know what kind of helper this is, but by exporting
	// the Classname of the helper object, the importer has a chance to
	// identify it.
	Object* helperObj = node->EvalWorldState(0).obj;
	if (helperObj) {
		TSTR className;
		helperObj->GetClassName(className);
//		fprintf(pStream,"%s\t%s \"%s\"\n", indent.data(), ID_HELPER_CLASS, className);
	}

	ExportNodeTM(node, indentLevel);

	if (helperObj) {
		TimeValue	t = GetStaticFrame();
		Matrix3		oTM = node->GetObjectTM(t);
		Box3		bbox;

		helperObj->GetDeformBBox(t, bbox, &oTM);

/*		fprintf(pStream,"%s\t%s %s\n", indent.data(), ID_BOUNDINGBOX_MIN, Format(bbox.pmin));
		fprintf(pStream,"%s\t%s %s\n", indent.data(), ID_BOUNDINGBOX_MAX, Format(bbox.pmax));*/
	}

	if (GetIncludeAnim()) {
		ExportAnimKeys(node, indentLevel);
	}
	
//	fprintf(pStream,"%s}\n", indent.data());
}


/****************************************************************************

  Node Header
  
****************************************************************************/

// The Node Header consists of node type (geometry, helper, camera etc.)
// node name and parent node
void MFExp::ExportNodeHeader(INode* node, TCHAR* type, int indentLevel)
{
	TSTR indent = GetIndent(indentLevel);
	
	// Output node header and object type 
//	fprintf(pStream,"%s%s {\n", indent.data(), type);
	
	// Node name
//	fprintf(pStream,"%s\t%s \"%s\"\n", indent.data(), ID_NODE_NAME, FixupName(node->GetName()));
	
	//  If the node is linked, export parent node name
	INode* parent = node->GetParentNode();
	if (parent && !parent->IsRootNode()) {
//		fprintf(pStream,"%s\t%s \"%s\"\n", indent.data(), ID_NODE_PARENT, FixupName(parent->GetName()));
	}
}


/****************************************************************************

  Node Transformation
  
****************************************************************************/

void MFExp::ExportNodeTM(INode* node, int indentLevel)
{
	Matrix3 pivot = node->GetNodeTM(GetStaticFrame());
	TSTR indent = GetIndent(indentLevel);
	
//	fprintf(pStream,"%s\t%s {\n", indent.data(), ID_NODE_TM);
	
	// Node name
	// We export the node name together with the nodeTM, because some objects
	// (like a camera or a spotlight) has an additional node (the target).
	// In that case the nodeTM and the targetTM is exported after eachother
	// and the nodeName is how you can tell them apart.
//	fprintf(pStream,"%s\t\t%s \"%s\"\n", indent.data(), ID_NODE_NAME, FixupName(node->GetName()));

	// Export TM inheritance flags
	DWORD iFlags = node->GetTMController()->GetInheritanceFlags();
/*	fprintf(pStream,"%s\t\t%s %d %d %d\n", indent.data(), ID_INHERIT_POS,
		INHERIT_POS_X & iFlags ? 1 : 0,
		INHERIT_POS_Y & iFlags ? 1 : 0,
		INHERIT_POS_Z & iFlags ? 1 : 0);

	fprintf(pStream,"%s\t\t%s %d %d %d\n", indent.data(), ID_INHERIT_ROT,
		INHERIT_ROT_X & iFlags ? 1 : 0,
		INHERIT_ROT_Y & iFlags ? 1 : 0,
		INHERIT_ROT_Z & iFlags ? 1 : 0);

	fprintf(pStream,"%s\t\t%s %d %d %d\n", indent.data(), ID_INHERIT_SCL,
		INHERIT_SCL_X & iFlags ? 1 : 0,
		INHERIT_SCL_Y & iFlags ? 1 : 0,
		INHERIT_SCL_Z & iFlags ? 1 : 0);*/

	// Dump the full matrix
	DumpMatrix3(&pivot, indentLevel+2);
	
//	fprintf(pStream,"%s\t}\n", indent.data());
}

/****************************************************************************

  Animation output
  
****************************************************************************/

// If the object is animated, then we will output the entire mesh definition
// for every specified frame. This can result in, dare I say, rather large files.
//
// Many target systems (including MAX itself!) cannot always read back this
// information. If the objects maintains the same number of verices it can be
// imported as a morph target, but if the number of vertices are animated it
// could not be read back in withou special tricks.
// Since the target system for this exporter is unknown, it is up to the
// user (or developer) to make sure that the data conforms with the target system.

void MFExp::ExportAnimMesh(INode* node, int indentLevel)
{
	ObjectState os = node->EvalWorldState(GetStaticFrame());
	if (!os.obj)
		return;
	
	TSTR indent = GetIndent(indentLevel);
	
	// Get animation range
	Interval animRange = ip->GetAnimRange();
	// Get validity of the object
	Interval objRange = os.obj->ObjectValidity(GetStaticFrame());
	
	// If the animation range is not fully included in the validity
	// interval of the object, then we're animated.
	if (!objRange.InInterval(animRange)) {
		
//		fprintf(pStream,"%s\t%s {\n", indent.data(), ID_MESH_ANIMATION);
		
		TimeValue t = animRange.Start();
		
		while (1) {
			// This may seem strange, but the object in the pipeline
			// might not be valid anymore.
			os = node->EvalWorldState(t);
			objRange = os.obj->ObjectValidity(t);
			t = objRange.Start() < animRange.Start() ? animRange.Start() : objRange.Start();
			
			// Export the mesh definition at this frame
			ExportMesh(node, t, indentLevel+1);
			
			if (objRange.End() >= animRange.End()) {
				break;
			}
			else {
				t = (objRange.End()/GetTicksPerFrame()+GetMeshFrameStep()) * GetTicksPerFrame();
			}
		}
//		fprintf(pStream,"%s\t}\n", indent.data());
	}
}

/****************************************************************************

  mfx node output
  
****************************************************************************/
void MFExp::mfxNodeExport( INode * node )
{
	INode *parent = node->GetParentNode();

	TCHAR * nodeId = node->GetName();
	std::string parentId;

	Matrix3 nodeMatrix = node->GetObjectTM( GetStaticFrame() );

	if( GetUseCharacterMode() )
	{
		nodeMatrix = normaliseMatrix( nodeMatrix );
	}

	if( parent )
	{
		parentId = parent->GetName();
		Matrix3 parentMatrix;
		if( GetUseCharacterMode() )
		{
			parentMatrix = Inverse( normaliseMatrix( parent->GetObjectTM( GetStaticFrame() ) ) );
		}
		else
		{
			parentMatrix = Inverse( parent->GetObjectTM( GetStaticFrame() ) );
		}

		nodeMatrix = nodeMatrix * parentMatrix;
	}
	else
		if( overrideRoot_ )
		{
			parentId = overrideRootIds_[0];
		}

	int size = CHUNK_HEADER_SIZE;
	
	size += getMFXStrLen( nodeId ) + getMFXStrLen( nodeId ) + getMFXStrLen( parentId );

	size += 3 * 4 * 4; //matrix34 size;

	writeMFXChunkHeader( CHUNKID_NODE, size, size, pStream );
	
	writeMFXStr( nodeId, pStream );
	writeMFXStr( parentId, pStream );
	writeMFXStr( nodeId, pStream );

	writeMFXMatrix( nodeMatrix, pStream );
}

void MFExp::writeMFXNodeOverrides( void )
{
	for( int i = 0; i < nodeOverrides_.size(); i++ )
	{
		int size = CHUNK_HEADER_SIZE;
		
		size += getMFXStrLen( nodeOverrides_[i].nodeId_ ) + getMFXStrLen( nodeOverrides_[i].nodeId_ ) + getMFXStrLen( nodeOverrides_[i].parentId_ );

		size += 3 * 4 * 4; //matrix34 size;

		writeMFXChunkHeader( CHUNKID_NODE, size, size, pStream );
		
		writeMFXStr( nodeOverrides_[i].nodeId_, pStream );
		writeMFXStr( nodeOverrides_[i].parentId_, pStream );
		writeMFXStr( nodeOverrides_[i].nodeId_, pStream );

		writeMFXMatrix( nodeOverrides_[i].transform_, pStream );
	}
}

void MFExp::writeMFXRootNodeOverrides( void )
{
	Matrix3 m;
	m.IdentityMatrix();
	if( overrideRoot_ )
	{
		int size = CHUNK_HEADER_SIZE;

		std::string parent;
			
		size += getMFXStrLen( overrideRootIds_.back() ) + getMFXStrLen( overrideRootIds_.back() ) + getMFXStrLen( parent );

		size += 3 * 4 * 4; //matrix34 size;

		writeMFXChunkHeader( CHUNKID_NODE, size, size, pStream );
			
		writeMFXStr( overrideRootIds_.back(), pStream );
		writeMFXStr( parent, pStream );
		writeMFXStr( overrideRootIds_.back(), pStream );

		writeMFXMatrix( m, pStream );
		
		for( int i = ( overrideRootIds_.size() -1 ) ; i > 0 ; i-- )
		{
			int size = CHUNK_HEADER_SIZE;
			
			size += getMFXStrLen( overrideRootIds_[i-1] ) + getMFXStrLen( overrideRootIds_[i-1] ) + getMFXStrLen( overrideRootIds_[i] );

			size += 3 * 4 * 4; //matrix34 size;

			writeMFXChunkHeader( CHUNKID_NODE, size, size, pStream );
			
			writeMFXStr( overrideRootIds_[i-1], pStream );
			writeMFXStr( overrideRootIds_[i], pStream );
			writeMFXStr( overrideRootIds_[i-1], pStream );

			writeMFXMatrix( m, pStream );
		}
	}
}

/****************************************************************************

  Mesh output
  
****************************************************************************/

void MFExp::ExportMesh(INode* node, TimeValue t, int indentLevel)
{
	int i;
	Mtl* nodeMtl = node->GetMtl();
	Matrix3 tm = node->GetObjTMAfterWSM(t);
	BOOL negScale = TMNegParity(tm);
	int vx1, vx2, vx3;
	TSTR indent;

	indent = GetIndent(indentLevel+1);
	
	ObjectState os = node->EvalWorldState(t);
	if (!os.obj || os.obj->SuperClassID()!=GEOMOBJECT_CLASS_ID) {
		return; // Safety net. This shouldn't happen.
	}
	
	// Order of the vertices. Get 'em counter clockwise if the objects is
	// negatively scaled.
	vx1 = 0;
	vx2 = 1;
	vx3 = 2;
	float scaleX = 1;
	float scaleY = 1;
	float scaleZ = 1;

	if( GetUseCharacterMode() )
	{
		INode *parent = node->GetParentNode();
		Matrix3 nodeMatrix = node->GetObjectTM( GetStaticFrame() );
//		nodeMatrix = normaliseMatrix( nodeMatrix );

		if( parent )
		{
			Matrix3 parentMatrix = Inverse( normaliseMatrix( parent->GetObjectTM( GetStaticFrame() ) ) );
			nodeMatrix = nodeMatrix * parentMatrix;
		}
		scaleX = Length( nodeMatrix.GetRow( 0 ) );
		scaleY = Length( nodeMatrix.GetRow( 1 ) );
		scaleZ = Length( nodeMatrix.GetRow( 2 ) );
		
	}
	
	BOOL needDel;
	TriObject* tri = GetTriObjectFromNode(node, t, needDel);
	if (!tri) {
		return;
	}
	
	Mesh* mesh = &tri->mesh;

	BOOL hasTCoords = FALSE;
	if (GetIncludeTextureCoords() && !CheckForAndExportFaceMap(nodeMtl, mesh, indentLevel+1)) 
	{
		int numTVx = mesh->getNumTVerts();
		if ( numTVx && mesh->tvFace )
		{
			hasTCoords = TRUE;
		}
	}

	std::string meshId = node->GetName();

/*	if( hasTCoords )*/
	if( !node->IsHidden() && ( GetIncludeEnvelopes() ?  ( meshId.substr( 0, envPre_.size() ) != envPre_ ) && ( meshId.substr( 0, bonePre_.size() ) != bonePre_ ) : TRUE ) )
	{
		VCVector vcV;

		int nVertices = 0;
		int nRFaces = mesh->getNumFaces();
		int nFaces = 0;
//		PCVector pc;

		for( i=0 ; i < nRFaces ; i++ )
		{

			if(  Length( mesh->verts[ mesh->faces[i].v[0] ] - mesh->verts[ mesh->faces[i].v[1] ] ) == 0 )
				continue;
			if(  Length( mesh->verts[ mesh->faces[i].v[1] ] - mesh->verts[ mesh->faces[i].v[2] ] ) == 0 )
				continue;
			if(  Length( mesh->verts[ mesh->faces[i].v[2] ] - mesh->verts[ mesh->faces[i].v[0] ] ) == 0 )
				continue;
			
			nFaces++;
			
			for( int j=0 ; j < 3 ; j++)
			{
				VertexContainer vc( mesh->faces[i].v[j], hasTCoords ? mesh->tvFace[i].t[j] : 0 );
				VCVector::iterator itVC = std::find( vcV.begin(), vcV.end(), vc );

				if( itVC == vcV.end( ) )
				{
					vcV.push_back( vc );
					nVertices++;
				}
			}
		}

		if( ( nVertices >= 3 ) && ( nFaces >= 1 ) )
		{

			TCHAR * meshId = node->GetName();
			TCHAR * materialId = NULL;

			int size = getMFXStrLen( meshId );
			size += 24; //header size for MESH_CHUNK

			if( nodeMtl )
			{
				materialId = nodeMtl->GetName();
			}

			size += getMFXStrLen( materialId );

			int totalSize = size;
			int verticesSize = CHUNK_HEADER_SIZE + ( nVertices * 3 * 4 ); //size of vertices;
			int tVerticesSize = hasTCoords ? ( CHUNK_HEADER_SIZE + ( nVertices * 2 * 4 ) ) : 0 ; //size of texture coordinates;
			int trianglesSize = CHUNK_HEADER_SIZE + ( nFaces * 3 * 4 ); //size of triangles;
			
			totalSize += verticesSize + tVerticesSize + trianglesSize;
			
			writeMFXChunkHeader( CHUNKID_MESH, totalSize, size, pStream );

			fwrite( &nVertices, 4, 1, pStream );
			fwrite( &nFaces, 4, 1, pStream );
			int nTc = hasTCoords ? 1 : 0;
			fwrite( &nTc, 4, 1, pStream );
		
			writeMFXStr( meshId, pStream );
			writeMFXStr( materialId, pStream );

			writeMFXChunkHeader( CHUNKID_VERTEXLIST, verticesSize, verticesSize, pStream );
			for( i=0 ; i < nVertices ; i++ )
			{
				Point3 v = mesh->verts[ vcV[i].getV() ];
				v.x *= scaleX;
				v.y *= scaleY;
				v.z *= scaleZ;
				writeMFXPoint( v, pStream );
			}

			if( hasTCoords )
			{
				writeMFXChunkHeader( CHUNKID_TEXCOORDLIST, tVerticesSize, tVerticesSize, pStream );
				for( i=0 ; i < nVertices ; i++ )
				{
					UVVert tv = mesh->tVerts[ vcV[i].getTV() ];
					writeMFXUV( tv , pStream );
				}
			}

			writeMFXChunkHeader( CHUNKID_TRIANGLELIST, trianglesSize, trianglesSize, pStream );
			for( i=0 ; i < nRFaces ; i++ )
			{
				if(  Length( mesh->verts[ mesh->faces[i].v[0] ] - mesh->verts[ mesh->faces[i].v[1] ] ) == 0 )
					continue;
				if(  Length( mesh->verts[ mesh->faces[i].v[1] ] - mesh->verts[ mesh->faces[i].v[2] ] ) == 0 )
					continue;
				if(  Length( mesh->verts[ mesh->faces[i].v[2] ] - mesh->verts[ mesh->faces[i].v[0] ] ) == 0 )
					continue;
				for( int j=0; j < 3 ; j++ )
				{
					int vi = j == 1 ? 2 : j == 2 ? 1 : 0;

					VertexContainer vc( mesh->faces[i].v[vi], hasTCoords ? (mesh->tvFace[i].t[vi]) : 0 );
					int vin = findMFXVertexIndex( vcV, vc );
					fwrite( &vin, 4, 1, pStream );
				}
			}
		}
	}

/*	else
	{
		int nVertices = mesh->getNumVerts();
		int nFaces = mesh->getNumFaces();
	}*/

	
	mesh->buildNormals();
	
/*	fprintf(pStream, "%s%s {\n",indent.data(),  ID_MESH);
	fprintf(pStream, "%s\t%s %d\n",indent.data(), ID_TIMEVALUE, t);
	fprintf(pStream, "%s\t%s %d\n",indent.data(), ID_MESH_NUMVERTEX, mesh->getNumVerts());
    fprintf(pStream, "%s\t%s %d\n",indent.data(), ID_MESH_NUMFACES, mesh->getNumFaces());*/
	
	// Export the vertices
//	fprintf(pStream,"%s\t%s {\n",indent.data(), ID_MESH_VERTEX_LIST);
	for (i=0; i<mesh->getNumVerts(); i++) {
		Point3 v = tm * mesh->verts[i];
//		fprintf(pStream, "%s\t\t%s %4d\t%s\n",indent.data(), ID_MESH_VERTEX, i, Format(v));
	}
//	fprintf(pStream,"%s\t}\n",indent.data()); // End vertex list
	
	// To determine visibility of a face, get the vertices in clockwise order.
	// If the objects has a negative scaling, we must compensate for that by
	// taking the vertices counter clockwise
//	fprintf(pStream, "%s\t%s {\n",indent.data(), ID_MESH_FACE_LIST);
	for (i=0; i<mesh->getNumFaces(); i++) {
/*		fprintf(pStream,"%s\t\t%s %4d:    A: %4d B: %4d C: %4d AB: %4d BC: %4d CA: %4d",
			indent.data(),
			ID_MESH_FACE, i,
			mesh->faces[i].v[vx1],
			mesh->faces[i].v[vx2],
			mesh->faces[i].v[vx3],
			mesh->faces[i].getEdgeVis(vx1) ? 1 : 0,
			mesh->faces[i].getEdgeVis(vx2) ? 1 : 0,
			mesh->faces[i].getEdgeVis(vx3) ? 1 : 0);
		fprintf(pStream,"\t %s ", ID_MESH_SMOOTHING);*/
		for (int j=0; j<32; j++) {
			if (mesh->faces[i].smGroup & (1<<j)) {
				if (mesh->faces[i].smGroup>>(j+1)) {
//					fprintf(pStream,"%d,",j+1); // Add extra comma
				} else {
//					fprintf(pStream,"%d ",j+1);
				}
			}
		}
		
		// This is the material ID for the face.
		// Note: If you use this you should make sure that the material ID
		// is not larger than the number of sub materials in the material.
		// The standard approach is to use a modulus function to bring down
		// the material ID.
/*		fprintf(pStream,"\t%s %d", ID_MESH_MTLID, mesh->faces[i].getMatID());
		
		fprintf(pStream,"\n");*/
	}
//	fprintf(pStream,"%s\t}\n", indent.data()); // End face list
	
	// Export face map texcoords if we have them...
	if (GetIncludeTextureCoords() && !CheckForAndExportFaceMap(nodeMtl, mesh, indentLevel+1)) {
		// If not, export standard tverts
		int numTVx = mesh->getNumTVerts();

//		fprintf(pStream, "%s\t%s %d\n",indent.data(), ID_MESH_NUMTVERTEX, numTVx);

		if (numTVx) {
//			fprintf(pStream,"%s\t%s {\n",indent.data(), ID_MESH_TVERTLIST);
			for (i=0; i<numTVx; i++) {
				UVVert tv = mesh->tVerts[i];
//				fprintf(pStream, "%s\t\t%s %d\t%s\n",indent.data(), ID_MESH_TVERT, i, Format(tv));
			}
//			fprintf(pStream,"%s\t}\n",indent.data());
			
//			fprintf(pStream, "%s\t%s %d\n",indent.data(), ID_MESH_NUMTVFACES, mesh->getNumFaces());

//			fprintf(pStream, "%s\t%s {\n",indent.data(), ID_MESH_TFACELIST);
			for (i=0; i<mesh->getNumFaces(); i++) {
/*				fprintf(pStream,"%s\t\t%s %d\t%d\t%d\t%d\n",
					indent.data(),
					ID_MESH_TFACE, i,
					mesh->tvFace[i].t[vx1],
					mesh->tvFace[i].t[vx2],
					mesh->tvFace[i].t[vx3]);*/
			}
//			fprintf(pStream, "%s\t}\n",indent.data());
		}
	}

	// Export color per vertex info
	if (GetIncludeVertexColors()) {
		int numCVx = mesh->numCVerts;

//		fprintf(pStream, "%s\t%s %d\n",indent.data(), ID_MESH_NUMCVERTEX, numCVx);
		if (numCVx) {
//			fprintf(pStream,"%s\t%s {\n",indent.data(), ID_MESH_CVERTLIST);
			for (i=0; i<numCVx; i++) {
				Point3 vc = mesh->vertCol[i];
//				fprintf(pStream, "%s\t\t%s %d\t%s\n",indent.data(), ID_MESH_VERTCOL, i, Format(vc));
			}
/*			fprintf(pStream,"%s\t}\n",indent.data());
			
			fprintf(pStream, "%s\t%s %d\n",indent.data(), ID_MESH_NUMCVFACES, mesh->getNumFaces());

			fprintf(pStream, "%s\t%s {\n",indent.data(), ID_MESH_CFACELIST);*/
			for (i=0; i<mesh->getNumFaces(); i++) {
/*				fprintf(pStream,"%s\t\t%s %d\t%d\t%d\t%d\n",
					indent.data(),
					ID_MESH_CFACE, i,
					mesh->vcFace[i].t[vx1],
					mesh->vcFace[i].t[vx2],
					mesh->vcFace[i].t[vx3]);*/
			}
//			fprintf(pStream, "%s\t}\n",indent.data());
		}
	}
	
	if (GetIncludeNormals()) {
		// Export mesh (face + vertex) normals
//		fprintf(pStream, "%s\t%s {\n",indent.data(), ID_MESH_NORMALS);
		
		Point3 fn;  // Face normal
		Point3 vn;  // Vertex normal
		int  vert;
		Face* f;
		
		// Face and vertex normals.
		// In MAX a vertex can have more than one normal (but doesn't always have it).
		// This is depending on the face you are accessing the vertex through.
		// To get all information we need to export all three vertex normals
		// for every face.
		for (i=0; i<mesh->getNumFaces(); i++) {
			f = &mesh->faces[i];
			fn = mesh->getFaceNormal(i);
//			fprintf(pStream,"%s\t\t%s %d\t%s\n", indent.data(), ID_MESH_FACENORMAL, i, Format(fn));
			
			vert = f->getVert(vx1);
			vn = GetVertexNormal(mesh, i, mesh->getRVertPtr(vert));
//			fprintf(pStream,"%s\t\t\t%s %d\t%s\n",indent.data(), ID_MESH_VERTEXNORMAL, vert, Format(vn));
			
			vert = f->getVert(vx2);
			vn = GetVertexNormal(mesh, i, mesh->getRVertPtr(vert));
//			fprintf(pStream,"%s\t\t\t%s %d\t%s\n",indent.data(), ID_MESH_VERTEXNORMAL, vert, Format(vn));
			
			vert = f->getVert(vx3);
			vn = GetVertexNormal(mesh, i, mesh->getRVertPtr(vert));
//			fprintf(pStream,"%s\t\t\t%s %d\t%s\n",indent.data(), ID_MESH_VERTEXNORMAL, vert, Format(vn));
		}
		
//		fprintf(pStream, "%s\t}\n",indent.data());
	}
	
//	fprintf(pStream, "%s}\n",indent.data());
	
	if (needDel) {
		delete tri;
	}
}

Point3 MFExp::GetVertexNormal(Mesh* mesh, int faceNo, RVertex* rv)
{
	Face* f = &mesh->faces[faceNo];
	DWORD smGroup = f->smGroup;
	int numNormals;
	Point3 vertexNormal;
	
	// Is normal specified
	// SPCIFIED is not currently used, but may be used in future versions.
	if (rv->rFlags & SPECIFIED_NORMAL) {
		vertexNormal = rv->rn.getNormal();
	}
	// If normal is not specified it's only available if the face belongs
	// to a smoothing group
	else if ((numNormals = rv->rFlags & NORCT_MASK) && smGroup) {
		// If there is only one vertex is found in the rn member.
		if (numNormals == 1) {
			vertexNormal = rv->rn.getNormal();
		}
		else {
			// If two or more vertices are there you need to step through them
			// and find the vertex with the same smoothing group as the current face.
			// You will find multiple normals in the ern member.
			for (int i = 0; i < numNormals; i++) {
				if (rv->ern[i].getSmGroup() & smGroup) {
					vertexNormal = rv->ern[i].getNormal();
				}
			}
		}
	}
	else {
		// Get the normal from the Face if no smoothing groups are there
		vertexNormal = mesh->getFaceNormal(faceNo);
	}
	
	return vertexNormal;
}

/****************************************************************************

  Inverse Kinematics (IK) Joint information
  
****************************************************************************/

void MFExp::ExportIKJoints(INode* node, int indentLevel)
{
	Control* cont;
	TSTR indent = GetIndent(indentLevel);

/*	if (node->TestAFlag(A_INODE_IK_TERMINATOR)) 
		fprintf(pStream,"%s\t%s\n", indent.data(), ID_IKTERMINATOR);

	if(node->TestAFlag(A_INODE_IK_POS_PINNED))
		fprintf(pStream,"%s\t%s\n", indent.data(), ID_IKPOS_PINNED);

	if(node->TestAFlag(A_INODE_IK_ROT_PINNED))
		fprintf(pStream,"%s\t%s\n", indent.data(), ID_IKROT_PINNED);*/

	// Position joint
	cont = node->GetTMController()->GetPositionController();
	if (cont) {
		JointParams* joint = (JointParams*)cont->GetProperty(PROPID_JOINTPARAMS);
		if (joint && !joint->IsDefault()) {
			// Has IK Joints!!!
//			fprintf(pStream,"%s\t%s {\n", indent.data(), ID_IKJOINT);
			DumpJointParams(joint, indentLevel+1);
//			fprintf(pStream,"%s\t}\n", indent.data());
		}
	}

	// Rotational joint
	cont = node->GetTMController()->GetRotationController();
	if (cont) {
		JointParams* joint = (JointParams*)cont->GetProperty(PROPID_JOINTPARAMS);
		if (joint && !joint->IsDefault()) {
			// Has IK Joints!!!
//			fprintf(pStream,"%s\t%s {\n", indent.data(), ID_IKJOINT);
			DumpJointParams(joint, indentLevel+1);
//			fprintf(pStream,"%s\t}\n", indent.data());
		}
	}
}

void MFExp::DumpJointParams(JointParams* joint, int indentLevel)
{
	TSTR indent = GetIndent(indentLevel);
	float scale = joint->scale;

/*	fprintf(pStream,"%s\t%s %s\n", indent.data(), ID_IKTYPE,   joint->Type() == JNT_POS ? ID_IKTYPEPOS : ID_IKTYPEROT);
	fprintf(pStream,"%s\t%s %d\n", indent.data(), ID_IKDOF,    joint->dofs);

	fprintf(pStream,"%s\t%s %d\n", indent.data(), ID_IKXACTIVE,  joint->flags & JNT_XACTIVE  ? 1 : 0);
	fprintf(pStream,"%s\t%s %d\n", indent.data(), ID_IKYACTIVE,  joint->flags & JNT_YACTIVE  ? 1 : 0);
	fprintf(pStream,"%s\t%s %d\n", indent.data(), ID_IKZACTIVE,  joint->flags & JNT_ZACTIVE  ? 1 : 0);

	fprintf(pStream,"%s\t%s %d\n", indent.data(), ID_IKXLIMITED, joint->flags & JNT_XLIMITED ? 1 : 0);
	fprintf(pStream,"%s\t%s %d\n", indent.data(), ID_IKYLIMITED, joint->flags & JNT_YLIMITED ? 1 : 0);
	fprintf(pStream,"%s\t%s %d\n", indent.data(), ID_IKZLIMITED, joint->flags & JNT_ZLIMITED ? 1 : 0);

	fprintf(pStream,"%s\t%s %d\n", indent.data(), ID_IKXEASE,    joint->flags & JNT_XEASE    ? 1 : 0);
	fprintf(pStream,"%s\t%s %d\n", indent.data(), ID_IKYEASE,    joint->flags & JNT_YEASE    ? 1 : 0);
	fprintf(pStream,"%s\t%s %d\n", indent.data(), ID_IKZEASE,    joint->flags & JNT_ZEASE    ? 1 : 0);

	fprintf(pStream,"%s\t%s %d\n", indent.data(), ID_IKLIMITEXACT, joint->flags & JNT_LIMITEXACT ? 1 : 0);*/

	for (int i=0; i<joint->dofs; i++) {
//		fprintf(pStream,"%s\t%s %d %s %s %s\n", indent.data(), ID_IKJOINTINFO, i, Format(joint->min[i]), Format(joint->max[i]), Format(joint->damping[i]));
	}

}

/****************************************************************************

  Material and Texture Export
  
****************************************************************************/

void MFExp::ExportMaterialList()
{
	if (!GetIncludeMtl()) {
		return;
	}

//	fprintf(pStream, "%s {\n", ID_MATERIAL_LIST);

	int numMtls = mtlList.Count();
//	fprintf(pStream, "\t%s %d\n", ID_MATERIAL_COUNT, numMtls);

	for (int i=0; i<numMtls; i++) {
		DumpMaterial(mtlList.GetMtl(i), i, -1, 0);
	}

//	fprintf(pStream, "}\n");
}

void MFExp::ExportMaterial(INode* node, int indentLevel)
{
	Mtl* mtl = node->GetMtl();
	
	TSTR indent = GetIndent(indentLevel);
	
	// If the node does not have a material, export the wireframe color
	if (mtl) {
		int mtlID = mtlList.GetMtlID(mtl);
		if (mtlID >= 0) {
//			fprintf(pStream,"%s\t%s %d\n", indent.data(), ID_MATERIAL_REF, mtlID);
		}
	}
	else {
		DWORD c = node->GetWireColor();
/*		fprintf(pStream,"%s\t%s %s\n", indent.data(), ID_WIRECOLOR,
			Format(Color(GetRValue(c)/255.0f, GetGValue(c)/255.0f, GetBValue(c)/255.0f)));*/
	}
}

void MFExp::DumpMaterial(Mtl* mtl, int mtlID, int subNo, int indentLevel)
{
	int i;
	TimeValue t = GetStaticFrame();
	
	if (!mtl) return;
	
	TSTR indent = GetIndent(indentLevel+1);
	
	TSTR className;
	mtl->GetClassName(className);
	
	
	if (subNo == -1) {
		// Top level material
//		fprintf(pStream,"%s%s %d {\n",indent.data(), ID_MATERIAL, mtlID);
	}
	else {
//		fprintf(pStream,"%s%s %d {\n",indent.data(), ID_SUBMATERIAL, subNo);
	}
/*	fprintf(pStream,"%s\t%s \"%s\"\n",indent.data(), ID_MATNAME, FixupName(mtl->GetName()));
	fprintf(pStream,"%s\t%s \"%s\"\n",indent.data(), ID_MATCLASS, FixupName(className));*/
	
	// We know the Standard material, so we can get some extra info
	if (mtl->ClassID() == Class_ID(DMTL_CLASS_ID, 0)) {
		StdMat* std = (StdMat*)mtl;

		Point3 ambient = std->GetAmbient(t);
		Point3 diffuse = std->GetDiffuse(t);
		Point3 specular = std->GetSpecular(t);
		float selfIllum = std->GetSelfIllum(t);

		int nTextures = 0;
		TCHAR * texName = NULL;

		for (i=0; i<mtl->NumSubTexmaps(); i++) {
			Texmap* subTex = mtl->GetSubTexmap(i);
			float amt = 1.0f;
			if (subTex) {
				// If it is a standard material we can see if the map is enabled.
				if (mtl->ClassID() == Class_ID(DMTL_CLASS_ID, 0)) {
					if (!((StdMat*)mtl)->MapEnabled(i))
						continue;
					amt = ((StdMat*)mtl)->GetTexmapAmt(i, 0);
					
				}
				if( i== ID_DI )
				{
					if (subTex->ClassID() == Class_ID(BMTEX_CLASS_ID, 0x00))
					{
						nTextures = 1;
						texName = ((BitmapTex *)subTex)->GetMapName();
					}
				}
			}
		}

		TCHAR *matId = std->GetName();

		int size = CHUNK_HEADER_SIZE;
		size += 11 * 4;
		size += getMFXStrLen( matId );
		int textureSize = 0;

		int totalSize = size;

		if( nTextures )
		{
			textureSize = CHUNK_HEADER_SIZE;
			textureSize += getMFXStrLen( texName );
		}

		totalSize += textureSize;

		writeMFXChunkHeader( CHUNKID_MATERIAL, totalSize, size, pStream );

		writeMFXStr( matId, pStream );
		fwrite( &selfIllum, 4, 1, pStream );
		writeMFXPoint( ambient, pStream );
		writeMFXPoint( diffuse, pStream );
		writeMFXPoint( specular, pStream );
		fwrite( &nTextures, 4, 1, pStream );

		if( nTextures )
		{
			writeMFXChunkHeader( CHUNKID_TEXTURE, textureSize, textureSize, pStream );
			writeMFXStr( texName, pStream );
		}


/*		fprintf(pStream,"%s\t%s %s\n",indent.data(), ID_AMBIENT, Format(std->GetAmbient(t)));
		fprintf(pStream,"%s\t%s %s\n",indent.data(), ID_DIFFUSE, Format(std->GetDiffuse(t)));
		fprintf(pStream,"%s\t%s %s\n",indent.data(), ID_SPECULAR, Format(std->GetSpecular(t)));
		fprintf(pStream,"%s\t%s %s\n",indent.data(), ID_SHINE, Format(std->GetShininess(t)));
		fprintf(pStream,"%s\t%s %s\n",indent.data(), ID_SHINE_STRENGTH, Format(std->GetShinStr(t)));
		fprintf(pStream,"%s\t%s %s\n",indent.data(), ID_TRANSPARENCY, Format(std->GetXParency(t)));
		fprintf(pStream,"%s\t%s %s\n",indent.data(), ID_WIRESIZE, Format(std->WireSize(t)));

		fprintf(pStream,"%s\t%s ", indent.data(), ID_SHADING);*/
		switch(std->GetShading()) {
		case SHADE_CONST: /*fprintf(pStream,"%s\n", ID_MAT_SHADE_CONST);*/ break;
		case SHADE_PHONG: /*fprintf(pStream,"%s\n", ID_MAT_SHADE_PHONG);*/ break;
		case SHADE_METAL: /*fprintf(pStream,"%s\n", ID_MAT_SHADE_METAL);*/ break;
		case SHADE_BLINN: /*fprintf(pStream,"%s\n", ID_MAT_SHADE_BLINN);*/ break;
		default: /*fprintf(pStream,"%s\n", ID_MAT_SHADE_OTHER);*/ break;
		}
		
/*		fprintf(pStream,"%s\t%s %s\n", indent.data(), ID_XP_FALLOFF, Format(std->GetOpacFalloff(t)));
		fprintf(pStream,"%s\t%s %s\n", indent.data(), ID_SELFILLUM, Format(std->GetSelfIllum(t)));*/
		
		if (std->GetTwoSided()) {
//			fprintf(pStream,"%s\t%s\n", indent.data(), ID_TWOSIDED);
		}
		
		if (std->GetWire()) {
//			fprintf(pStream,"%s\t%s\n", indent.data(), ID_WIRE);
		}
		
		if (std->GetWireUnits()) {
//			fprintf(pStream,"%s\t%s\n", indent.data(), ID_WIREUNITS);
		}
		
//		fprintf(pStream,"%s\t%s %s\n", indent.data(), ID_FALLOFF, std->GetFalloffOut() ? ID_FALLOFF_OUT : ID_FALLOFF_IN);
		
		if (std->GetFaceMap()) {
//			fprintf(pStream,"%s\t%s\n", indent.data(), ID_FACEMAP);
		}
		
		if (std->GetSoften()) {
//			fprintf(pStream,"%s\t%s\n", indent.data(), ID_SOFTEN);
		}
		
//		fprintf(pStream,"%s\t%s ", indent.data(), ID_XP_TYPE);
		switch (std->GetTransparencyType()) {
		case TRANSP_FILTER: /*fprintf(pStream,"%s\n", ID_MAP_XPTYPE_FLT);*/ break;
		case TRANSP_SUBTRACTIVE: /*fprintf(pStream,"%s\n", ID_MAP_XPTYPE_SUB);*/ break;
		case TRANSP_ADDITIVE: /*fprintf(pStream,"%s\n", ID_MAP_XPTYPE_ADD);*/ break;
		default: /*fprintf(pStream,"%s\n", ID_MAP_XPTYPE_OTH);*/ break;
		}
	}
	else {
		// Note about material colors:
		// This is only the color used by the interactive renderer in MAX.
		// To get the color used by the scanline renderer, we need to evaluate
		// the material using the mtl->Shade() method.
		// Since the materials are procedural there is no real "diffuse" color for a MAX material
		// but we can at least take the interactive color.
		
/*		fprintf(pStream,"%s\t%s %s\n",indent.data(), ID_AMBIENT, Format(mtl->GetAmbient()));
		fprintf(pStream,"%s\t%s %s\n",indent.data(), ID_DIFFUSE, Format(mtl->GetDiffuse()));
		fprintf(pStream,"%s\t%s %s\n",indent.data(), ID_SPECULAR, Format(mtl->GetSpecular()));
		fprintf(pStream,"%s\t%s %s\n",indent.data(), ID_SHINE, Format(mtl->GetShininess()));
		fprintf(pStream,"%s\t%s %s\n",indent.data(), ID_SHINE_STRENGTH, Format(mtl->GetShinStr()));
		fprintf(pStream,"%s\t%s %s\n",indent.data(), ID_TRANSPARENCY, Format(mtl->GetXParency()));
		fprintf(pStream,"%s\t%s %s\n",indent.data(), ID_WIRESIZE, Format(mtl->WireSize()));*/
	}

	for (i=0; i<mtl->NumSubTexmaps(); i++) {
		Texmap* subTex = mtl->GetSubTexmap(i);
		float amt = 1.0f;
		if (subTex) {
			// If it is a standard material we can see if the map is enabled.
			if (mtl->ClassID() == Class_ID(DMTL_CLASS_ID, 0)) {
				if (!((StdMat*)mtl)->MapEnabled(i))
					continue;
				amt = ((StdMat*)mtl)->GetTexmapAmt(i, 0);
				
			}
		}
	}
	
/*	if (mtl->NumSubMtls() > 0)  {
//		fprintf(pStream,"%s\t%s %d\n",indent.data(), ID_NUMSUBMTLS, mtl->NumSubMtls());
		
		for (i=0; i<mtl->NumSubMtls(); i++) {
			Mtl* subMtl = mtl->GetSubMtl(i);
			if (subMtl) {
				DumpMaterial(subMtl, 0, i, indentLevel+1);
			}
		}
	}*/
//	fprintf(pStream,"%s}\n", indent.data());
}


// For a standard material, this will give us the meaning of a map
// givien its submap id.
TCHAR* MFExp::GetMapID(Class_ID cid, int subNo)
{
	static TCHAR buf[50];
	
	if (cid == Class_ID(0,0)) {
		strcpy(buf, ID_ENVMAP);
	}
	else if (cid == Class_ID(DMTL_CLASS_ID, 0)) {
		switch (subNo) {
		case ID_AM: strcpy(buf, ID_MAP_AMBIENT); break;
		case ID_DI: strcpy(buf, ID_MAP_DIFFUSE); break;
		case ID_SP: strcpy(buf, ID_MAP_SPECULAR); break;
		case ID_SH: strcpy(buf, ID_MAP_SHINE); break;
		case ID_SS: strcpy(buf, ID_MAP_SHINESTRENGTH); break;
		case ID_SI: strcpy(buf, ID_MAP_SELFILLUM); break;
		case ID_OP: strcpy(buf, ID_MAP_OPACITY); break;
		case ID_FI: strcpy(buf, ID_MAP_FILTERCOLOR); break;
		case ID_BU: strcpy(buf, ID_MAP_BUMP); break;
		case ID_RL: strcpy(buf, ID_MAP_REFLECT); break;
		case ID_RR: strcpy(buf, ID_MAP_REFRACT); break;
		}
	}
	else {
		strcpy(buf, ID_MAP_GENERIC);
	}
	
	return buf;
}

void MFExp::DumpTexture(Texmap* tex, Class_ID cid, int subNo, float amt, int indentLevel)
{
	if (!tex) return;
	
	TSTR indent = GetIndent(indentLevel+1);
	
	TSTR className;
	tex->GetClassName(className);
	
/*	fprintf(pStream,"%s%s {\n", indent.data(), GetMapID(cid, subNo));
	
	fprintf(pStream,"%s\t%s \"%s\"\n", indent.data(), ID_TEXNAME, FixupName(tex->GetName()));
	fprintf(pStream,"%s\t%s \"%s\"\n", indent.data(), ID_TEXCLASS, FixupName(className));*/
	
	// If we include the subtexture ID, a parser could be smart enough to get
	// the class name of the parent texture/material and make it mean something.
/*	fprintf(pStream,"%s\t%s %d\n", indent.data(), ID_TEXSUBNO, subNo);
	
	fprintf(pStream,"%s\t%s %s\n", indent.data(), ID_TEXAMOUNT, Format(amt));*/
	
	// Is this a bitmap texture?
	// We know some extra bits 'n pieces about the bitmap texture
	if (tex->ClassID() == Class_ID(BMTEX_CLASS_ID, 0x00)) {
		TSTR mapName = ((BitmapTex *)tex)->GetMapName();
//		fprintf(pStream,"%s\t%s \"%s\"\n", indent.data(), ID_BITMAP, FixupName(mapName));
		
		StdUVGen* uvGen = ((BitmapTex *)tex)->GetUVGen();
		if (uvGen) {
			DumpUVGen(uvGen, indentLevel+1);
		}
		
		TextureOutput* texout = ((BitmapTex*)tex)->GetTexout();
		if (texout->GetInvert()) {
//			fprintf(pStream,"%s\t%s\n", indent.data(), ID_TEX_INVERT);
		}
		
//		fprintf(pStream,"%s\t%s ", indent.data(), ID_BMP_FILTER);
		switch(((BitmapTex*)tex)->GetFilterType()) {
		case FILTER_PYR:  /*fprintf(pStream,"%s\n", ID_BMP_FILT_PYR);*/ break;
		case FILTER_SAT: /*fprintf(pStream,"%s\n", ID_BMP_FILT_SAT);*/ break;
		default: /*fprintf(pStream,"%s\n", ID_BMP_FILT_NONE);*/ break;
		}
	}
	
	for (int i=0; i<tex->NumSubTexmaps(); i++) {
		DumpTexture(tex->GetSubTexmap(i), tex->ClassID(), i, 1.0f, indentLevel+1);
	}
	
//	fprintf(pStream, "%s}\n", indent.data());
}

void MFExp::DumpUVGen(StdUVGen* uvGen, int indentLevel)
{
	int mapType = uvGen->GetCoordMapping(0);
	TimeValue t = GetStaticFrame();
	TSTR indent = GetIndent(indentLevel+1);
	
//	fprintf(pStream,"%s%s ", indent.data(), ID_MAPTYPE);
	
	switch (mapType) {
	case UVMAP_EXPLICIT: /*fprintf(pStream,"%s\n", ID_MAPTYPE_EXP);*/ break;
	case UVMAP_SPHERE_ENV: /*fprintf(pStream,"%s\n", ID_MAPTYPE_SPH);*/ break;
	case UVMAP_CYL_ENV:  /*fprintf(pStream,"%s\n", ID_MAPTYPE_CYL);*/ break;
	case UVMAP_SHRINK_ENV: /*fprintf(pStream,"%s\n", ID_MAPTYPE_SHR);*/ break;
	case UVMAP_SCREEN_ENV: /*fprintf(pStream,"%s\n", ID_MAPTYPE_SCR);*/ break;
	}
	
/*	fprintf(pStream,"%s%s %s\n", indent.data(), ID_U_OFFSET, Format(uvGen->GetUOffs(t)));
	fprintf(pStream,"%s%s %s\n", indent.data(), ID_V_OFFSET, Format(uvGen->GetVOffs(t)));
	fprintf(pStream,"%s%s %s\n", indent.data(), ID_U_TILING, Format(uvGen->GetUScl(t)));
	fprintf(pStream,"%s%s %s\n", indent.data(), ID_V_TILING, Format(uvGen->GetVScl(t)));
	fprintf(pStream,"%s%s %s\n", indent.data(), ID_ANGLE, Format(uvGen->GetAng(t)));
	fprintf(pStream,"%s%s %s\n", indent.data(), ID_BLUR, Format(uvGen->GetBlur(t)));
	fprintf(pStream,"%s%s %s\n", indent.data(), ID_BLUR_OFFSET, Format(uvGen->GetBlurOffs(t)));
	fprintf(pStream,"%s%s %s\n", indent.data(), ID_NOISE_AMT, Format(uvGen->GetNoiseAmt(t)));
	fprintf(pStream,"%s%s %s\n", indent.data(), ID_NOISE_SIZE, Format(uvGen->GetNoiseSize(t)));
	fprintf(pStream,"%s%s %d\n", indent.data(), ID_NOISE_LEVEL, uvGen->GetNoiseLev(t));
	fprintf(pStream,"%s%s %s\n", indent.data(), ID_NOISE_PHASE, Format(uvGen->GetNoisePhs(t)));*/
}

/****************************************************************************

  Face Mapped Material functions
  
****************************************************************************/

BOOL MFExp::CheckForAndExportFaceMap(Mtl* mtl, Mesh* mesh, int indentLevel)
{
	if (!mtl || !mesh) {
		return FALSE;
	}
	
	ULONG matreq = mtl->Requirements(-1);
	
	// Are we using face mapping?
	if (!(matreq & MTLREQ_FACEMAP)) {
		return FALSE;
	}
	
	TSTR indent = GetIndent(indentLevel+1);
	
	// OK, we have a FaceMap situation here...
	
//	fprintf(pStream, "%s%s {\n", indent.data(), ID_MESH_FACEMAPLIST);
	for (int i=0; i<mesh->getNumFaces(); i++) {
		Point3 tv[3];
		Face* f = &mesh->faces[i];
		make_face_uv(f, tv);
/*		fprintf(pStream, "%s\t%s %d {\n", indent.data(), ID_MESH_FACEMAP, i);
		fprintf(pStream, "%s\t\t%s\t%d\t%d\t%d\n", indent.data(), ID_MESH_FACEVERT, (int)tv[0].x, (int)tv[0].y, (int)tv[0].z);
		fprintf(pStream, "%s\t\t%s\t%d\t%d\t%d\n", indent.data(), ID_MESH_FACEVERT, (int)tv[1].x, (int)tv[1].y, (int)tv[1].z);
		fprintf(pStream, "%s\t\t%s\t%d\t%d\t%d\n", indent.data(), ID_MESH_FACEVERT, (int)tv[2].x, (int)tv[2].y, (int)tv[2].z);
		fprintf(pStream, "%s\t}\n", indent.data());*/
	}
//	fprintf(pStream, "%s}\n", indent.data());
	
	return TRUE;
}


/****************************************************************************

  Misc Utility functions
  
****************************************************************************/

// Return an indentation string
TSTR MFExp::GetIndent(int indentLevel)
{
	TSTR indentString = "";
	for (int i=0; i<indentLevel; i++) {
		indentString += "\t";
	}
	
	return indentString;
}

// Determine is the node has negative scaling.
// This is used for mirrored objects for example. They have a negative scale factor
// so when calculating the normal we should take the vertices counter clockwise.
// If we don't compensate for this the objects will be 'inverted'.
BOOL MFExp::TMNegParity(Matrix3 &m)
{
	return (DotProd(CrossProd(m.GetRow(0),m.GetRow(1)),m.GetRow(2))<0.0)?1:0;
}

// Return a pointer to a TriObject given an INode or return NULL
// if the node cannot be converted to a TriObject
TriObject* MFExp::GetTriObjectFromNode(INode *node, TimeValue t, int &deleteIt)
{
	deleteIt = FALSE;
	Object *obj = node->EvalWorldState(t).obj;
	if (obj->CanConvertToType(Class_ID(TRIOBJ_CLASS_ID, 0))) { 
		TriObject *tri = (TriObject *) obj->ConvertToType(t, 
			Class_ID(TRIOBJ_CLASS_ID, 0));
		// Note that the TriObject should only be deleted
		// if the pointer to it is not equal to the object
		// pointer that called ConvertToType()
		if (obj != tri) deleteIt = TRUE;
		return tri;
	}
	else {
		return NULL;
	}
}

// Print out a transformation matrix in different ways.
// Apart from exporting the full matrix we also decompose
// the matrix and export the components.
void MFExp::DumpMatrix3(Matrix3* m, int indentLevel)
{
	Point3 row;
	TSTR indent = GetIndent(indentLevel);
	
	// Dump the whole Matrix
	row = m->GetRow(0);
//	fprintf(pStream,"%s%s %s\n", indent.data(), ID_TM_ROW0, Format(row));
	row = m->GetRow(1);
//	fprintf(pStream,"%s%s %s\n", indent.data(), ID_TM_ROW1, Format(row));
	row = m->GetRow(2);
//	fprintf(pStream,"%s%s %s\n", indent.data(), ID_TM_ROW2, Format(row));
	row = m->GetRow(3);
//	fprintf(pStream,"%s%s %s\n", indent.data(), ID_TM_ROW3, Format(row));
	
	// Decompose the matrix and dump the contents
	AffineParts ap;
	float rotAngle;
	Point3 rotAxis;
	float scaleAxAngle;
	Point3 scaleAxis;
	
	decomp_affine(*m, &ap);

	// Quaternions are dumped as angle axis.
	AngAxisFromQ(ap.q, &rotAngle, rotAxis);
	AngAxisFromQ(ap.u, &scaleAxAngle, scaleAxis);

/*	fprintf(pStream,"%s%s %s\n", indent.data(), ID_TM_POS, Format(ap.t));
	fprintf(pStream,"%s%s %s\n", indent.data(), ID_TM_ROTAXIS, Format(rotAxis));
	fprintf(pStream,"%s%s %s\n", indent.data(), ID_TM_ROTANGLE, Format(rotAngle));
	fprintf(pStream,"%s%s %s\n", indent.data(), ID_TM_SCALE, Format(ap.k));
	fprintf(pStream,"%s%s %s\n", indent.data(), ID_TM_SCALEAXIS, Format(scaleAxis));
	fprintf(pStream,"%s%s %s\n", indent.data(), ID_TM_SCALEAXISANG, Format(scaleAxAngle));*/
}

// From the SDK
// How to calculate UV's for face mapped materials.
static Point3 basic_tva[3] = { 
	Point3(0.0,0.0,0.0),Point3(1.0,0.0,0.0),Point3(1.0,1.0,0.0)
};
static Point3 basic_tvb[3] = { 
	Point3(1.0,1.0,0.0),Point3(0.0,1.0,0.0),Point3(0.0,0.0,0.0)
};
static int nextpt[3] = {1,2,0};
static int prevpt[3] = {2,0,1};

void MFExp::make_face_uv(Face *f, Point3 *tv)
{
	int na,nhid,i;
	Point3 *basetv;
	/* make the invisible edge be 2->0 */
	nhid = 2;
	if (!(f->flags&EDGE_A))  nhid=0;
	else if (!(f->flags&EDGE_B)) nhid = 1;
	else if (!(f->flags&EDGE_C)) nhid = 2;
	na = 2-nhid;
	basetv = (f->v[prevpt[nhid]]<f->v[nhid]) ? basic_tva : basic_tvb; 
	for (i=0; i<3; i++) {  
		tv[i] = basetv[na];
		na = nextpt[na];
	}
}


/****************************************************************************

  String manipulation functions
  
****************************************************************************/

#define CTL_CHARS  31
#define SINGLE_QUOTE 39

// Replace some characters we don't care for.
TCHAR* MFExp::FixupName(TCHAR* name)
{
	static char buffer[256];
	TCHAR* cPtr;
	
    _tcscpy(buffer, name);
    cPtr = buffer;
	
    while(*cPtr) {
		if (*cPtr == '"')
			*cPtr = SINGLE_QUOTE;
        else if (*cPtr <= CTL_CHARS)
			*cPtr = _T('_');
        cPtr++;
    }
	
	return buffer;
}

// International settings in Windows could cause a number to be written
// with a "," instead of a ".".
// To compensate for this we need to convert all , to . in order to make the
// format consistent.
void MFExp::CommaScan(TCHAR* buf)
{
    for(; *buf; buf++) if (*buf == ',') *buf = '.';
}

TSTR MFExp::Format(int value)
{
	TCHAR buf[50];
	
	sprintf(buf, _T("%d"), value);
	return buf;
}


TSTR MFExp::Format(float value)
{
	TCHAR buf[40];
	
	sprintf(buf, szFmtStr, value);
	CommaScan(buf);
	return TSTR(buf);
}

TSTR MFExp::Format(Point3 value)
{
	TCHAR buf[120];
	TCHAR fmt[120];
	
	sprintf(fmt, "%s\t%s\t%s", szFmtStr, szFmtStr, szFmtStr);
	sprintf(buf, fmt, value.x, value.y, value.z);

	CommaScan(buf);
	return buf;
}

TSTR MFExp::Format(Color value)
{
	TCHAR buf[120];
	TCHAR fmt[120];
	
	sprintf(fmt, "%s\t%s\t%s", szFmtStr, szFmtStr, szFmtStr);
	sprintf(buf, fmt, value.r, value.g, value.b);

	CommaScan(buf);
	return buf;
}

TSTR MFExp::Format(AngAxis value)
{
	TCHAR buf[160];
	TCHAR fmt[160];
	
	sprintf(fmt, "%s\t%s\t%s\t%s", szFmtStr, szFmtStr, szFmtStr, szFmtStr);
	sprintf(buf, fmt, value.axis.x, value.axis.y, value.axis.z, value.angle);

	CommaScan(buf);
	return buf;
}


TSTR MFExp::Format(Quat value)
{
	// A Quat is converted to an AngAxis before output.
	
	Point3 axis;
	float angle;
	AngAxisFromQ(value, &angle, axis);
	
	return Format(AngAxis(axis, angle));
}

TSTR MFExp::Format(ScaleValue value)
{
	TCHAR buf[280];
	
	sprintf(buf, "%s %s", Format(value.s), Format(value.q));
	CommaScan(buf);
	return buf;
}
