#include "pch.hpp"

#include "sound_manager.hpp"
#if FMOD_SUPPORT


#include "sound_occluder.hpp"
#include "physics2/bsp.hpp"
#include "model/model.hpp"
#include "model/super_model.hpp"
#include "moo/effect_constant_value.hpp"
#include "terrain/terrain_height_map.hpp"
#include "resmgr/auto_config.hpp"
#include "moo/geometrics.hpp"


BW_BEGIN_NAMESPACE

static void drawFMODGeometry( FMOD::Geometry* geometry )
{
	FMOD_RESULT result;

	// Get number of polygons
	int numPolygons = 0;
	result = geometry->getNumPolygons( &numPolygons );
	if (!SoundManager::FMOD_ErrCheck( result, "drawFMODGeometry: Couldn't get number of polygons." ))
	{
		return;
	}

	if (numPolygons == 0)
	{
		return;
	}

	BW::vector<Moo::VertexXYZL> verts;

	Vector3 trans, left, forward, up;
	geometry->getPosition( (FMOD_VECTOR*)(&trans) );
	geometry->getRotation( (FMOD_VECTOR*)(&forward), (FMOD_VECTOR*)(&up) );

	left.crossProduct(up, forward);

	Matrix transform;
	transform.column( 0, Vector4(left.x, left.y, left.z, 0.0f) );
	transform.column( 1, Vector4(up.x, up.y, up.z, 0.0f) );
	transform.column( 2, Vector4(forward.x, forward.y, forward.z, 0.0f) );
	transform.column( 3, Vector4(trans.x, trans.y, trans.z, 1.0) );


	//set the transforms
	Moo::rc().push();

	Moo::rc().device()->SetTransform( D3DTS_WORLD, &transform );
	Moo::rc().device()->SetTransform( D3DTS_VIEW, &Moo::rc().view() );
	Moo::rc().device()->SetTransform( D3DTS_PROJECTION, &Moo::rc().projection() );

	Moo::rc().setPixelShader( NULL );
	Moo::rc().setVertexShader( NULL );
	Moo::rc().setFVF( Moo::VertexXYZL::fvf() );
	Moo::rc().setRenderState( D3DRS_ALPHATESTENABLE, FALSE );
	Moo::rc().setRenderState( D3DRS_ALPHABLENDENABLE, FALSE );
	Moo::rc().setRenderState( D3DRS_LIGHTING, FALSE );
	Moo::rc().setRenderState( D3DRS_ZWRITEENABLE, TRUE );
	Moo::rc().setRenderState( D3DRS_ZENABLE, D3DZB_TRUE );
	Moo::rc().setRenderState( D3DRS_ZFUNC, D3DCMP_LESSEQUAL );
	Moo::FogHelper::pInstance()->fogEnable(false);

	Moo::rc().setTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_SELECTARG1 );
	Moo::rc().setTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_DIFFUSE );
	Moo::rc().setTextureStageState( 0, D3DTSS_ALPHAOP, D3DTOP_DISABLE );
	Moo::rc().setTextureStageState( 1, D3DTSS_ALPHAOP, D3DTOP_DISABLE );
	Moo::rc().setTextureStageState( 1, D3DTSS_COLOROP, D3DTOP_DISABLE );

	// Go through and build us a mesh to draw with.
	for (int ipoly = 0; ipoly < numPolygons; ipoly++)
	{
		int numVerts = 0;
		geometry->getPolygonNumVertices( ipoly, &numVerts );
		if (numVerts < 3)
		{
			WARNING_MSG( "drawFMODGeometry: degenerate polygon found.\n" );
			continue;
		}

		// We need a triangle list so do a very basic triangulation (fan style).
		for (int ivert = 0; ivert < numVerts; ivert+=1)
		{
			FMOD_VECTOR fmv;
			geometry->getPolygonVertex(ipoly, ivert, &fmv );

			Moo::VertexXYZL v;
			v.pos_.set( fmv.x, fmv.y, fmv.z );
			//INFO_MSG( "Poly: %d, Vert: %d, %f %f %f\n", ipoly, ivert, v.pos_.x, v.pos_.y, v.pos_.z );
			v.colour_ = D3DCOLOR_RGBA(ivert == 0 ? 255 : 0, 255, 128, 255 );
			verts.push_back( v );
		}

		Moo::rc().drawPrimitiveUP( D3DPT_TRIANGLEFAN, numVerts-2, &verts[0], sizeof( Moo::VertexXYZL ) );

		verts.resize(0);
	}

	Moo::rc().pop();
}

SoundOccluder::SoundOccluder()
{
}

SoundOccluder::SoundOccluder( SuperModel * pSuperModel )
{
    BW_GUARD;
    construct( pSuperModel );
}

SoundOccluder::~SoundOccluder()
{
    BW_GUARD;
    for( unsigned int i = 0; i < geometries_.size(); i++ )
    {
        FMOD_RESULT result = geometries_[i]->release();
        SoundManager::FMOD_ErrCheck( result, "SoundOccluder::~SoundOccluder: Couldn't release geometry." );
    }
}

bool SoundOccluder::construct( SuperModel * pSuperModel )
{
#if 1 //construct main model
    if (pSuperModel != NULL)
        return construct( &*pSuperModel->topModel(0) );

    return false;
#else //construct all models
    bool ok = true;
    for (int i = 0;
         i < pSuperModel->nModels();
         i++)
    {
        // construct each model in this super model.
        ok &= construct( pSuperModel->curModel(i) );
    }

    return ok;
#endif
}

bool SoundOccluder::construct( Model * pModel )
{
    BW_GUARD;
	if (!pModel)
        return false;

    Moo::Visual *pVisual = &*pModel->getVisual();

    Moo::Visual::RenderSetVector& renderSets = pVisual->renderSets();

    bool ok = true;
    if (renderSets.size() && renderSets[0].geometry_.size())
	{
		Moo::Visual::Geometry& geometry = renderSets[0].geometry_[0];

		IF_NOT_MF_ASSERT_DEV( geometry.vertices_ && geometry.primitives_ )
		{
			return false;
		}

        // Iterate over the primitive groups and collect the triangles
		BW::vector<uint32> indices;
		for (uint32 i = 0; i < geometry.primitiveGroups_.size(); i++)
		{
			Moo::Visual::PrimitiveGroup& pg = geometry.primitiveGroups_[i];

			if (!pg.material_.exists())
			{
				continue;
			}

            Moo::EffectMaterial *material = &*pg.material_->baseMaterial();
			// If the primiative group has no occlusion values ignore.
            float directOcclusion = 0.f;
            bool occluder = material->floatProperty(directOcclusion, "DirectOcclusion");
            float reverbOcclusion = 0.f;
            occluder &= material->floatProperty(reverbOcclusion, "ReverbOcclusion");
            bool doubleSided = false;
            material->boolProperty(doubleSided, "doubleSided");

            float tempTest; material->floatProperty( tempTest, "selfIllumination" );

			if ( occluder && (directOcclusion != 0.f || reverbOcclusion != 0.f ))
			{
				const Moo::PrimitiveGroup& primGroup = geometry.primitives_->primitiveGroup(pg.groupIndex_);
                if (geometry.primitives_->indices().format() == D3DFMT_INDEX16)
			    {
				    const uint16* pInd = (const uint16*)geometry.primitives_->indices().indices();
				    indices.insert( indices.end(), pInd + primGroup.startIndex_, 
					    pInd + primGroup.startIndex_ + primGroup.nPrimitives_ * 3);
			    }
			    else
			    {
				    const uint32* pInd = (const uint32*)geometry.primitives_->indices().indices();
				    indices.insert( indices.end(), pInd + primGroup.startIndex_, 
					    pInd + primGroup.startIndex_ + primGroup.nPrimitives_ * 3);
			    }

                if (indices.size())
			    {                  
                    // Create a new FMOD::Geometry pointer
                    FMOD::Geometry *pGeometry = NULL;

					BW::vector<Vector3> verts;
					geometry.vertices_->vertexPositionsVB( verts );
					MF_ASSERT( indices.size() <= INT_MAX );
                    int numIndices = ( int )indices.size() ;
                    int numTris = numIndices / 3;
                    int numVerts = numTris * 3;//verts.size();

                    FMOD::System *pSystem;
                    FMOD_RESULT result = SoundManager::instance().getEventSystemObject()->getSystemObject(&pSystem);
                    if (!SoundManager::FMOD_ErrCheck( result, "SoundOccluder::construct Couldn't get FMOD::System handle" ))
                        return false;
                    result = pSystem->createGeometry( numTris, numVerts, &pGeometry );
                    if (!SoundManager::FMOD_ErrCheck( result, "SoundOccluder::SoundOccluder Couldn't create geometry object." ))
                        return false;                    

                    for (int i = 0; i < numIndices; ++i)
                    {
                        FMOD_VECTOR vertices[3];
                        vertices[0] = *(( FMOD_VECTOR *) &(verts[indices[i]]));
                        vertices[1] = *(( FMOD_VECTOR *) &(verts[indices[++i]]));
                        vertices[2] = *(( FMOD_VECTOR *) &(verts[indices[++i]]));

                        int index; //TODO: remove 'index' (replace with NULL if possible)
                        FMOD_RESULT result = pGeometry->addPolygon( directOcclusion, reverbOcclusion, doubleSided, 3, vertices, &index ); 
                        ok &= SoundManager::FMOD_ErrCheck( result, "SoundOccluder::SoundOccluder" );
                    }

                    geometries_.push_back( pGeometry );
			    }
            }
        }
    }

    return setActive( true );
}

bool SoundOccluder::construct( const Terrain::TerrainHeightMap& map, 
							   float blockSize,
							   float directOcclusion, float reverbOcclusion )
{
	BW_GUARD;

    FMOD::EventSystem *eventSystem = SoundManager::instance().getEventSystemObject();
    if (eventSystem == NULL)
        return false;

    if ((directOcclusion == 0.0f) && (reverbOcclusion == 0.0f))
        return false;

    const bool  doubleSided     = false;

	// We set our mesh size to be power of two, as we assume that
	// the height map is power of 2 + 1
	const uint32 MESH_SIZE = std::min(uint32(16), map.blocksWidth() );

	const uint32 VERTICES_SIZE = MESH_SIZE + 1;

	// This is the distance between heights in the terrain mesh
    float step = blockSize / (MESH_SIZE);


	// reserve enough data for our vertices
	BW::vector<Vector3> verts;
    verts.reserve( VERTICES_SIZE * VERTICES_SIZE );

    uint32 numQuads = MESH_SIZE * MESH_SIZE;
    uint32 numVerts = numQuads * 4;
	// Iterate over our heights and initialise our vertices with an aliased
	// sample
	float zPos = 0.f;
	for (uint32 z = 0; z < map.verticesHeight(); 
			z += (map.blocksHeight() / MESH_SIZE) )
	{
		float xPos = 0.f;
		for (uint32 x = 0; x < map.verticesWidth(); 
				x += (map.blocksWidth() / MESH_SIZE))
		{
			float h = map.heightAt( int(x), int(z) );
			verts.push_back( Vector3( xPos, h, zPos));
			xPos += step;
		}
		zPos += step;
	}

    FMOD::Geometry *pGeometry = NULL;
    FMOD::System *pSystem;
    FMOD_RESULT result = eventSystem->getSystemObject(&pSystem);
    if (!SoundManager::FMOD_ErrCheck( result, "SoundOccluder::construct Couldn't get FMOD::System handle" ))
        return false;
    result = pSystem->createGeometry( numQuads, numVerts, &pGeometry );
    if (!SoundManager::FMOD_ErrCheck( result, "SoundOccluder::SoundOccluder Couldn't create geometry object." ))
        return false;                    

    bool ok = true;

    for ( uint32 z = 0; z < MESH_SIZE; ++z)
    {
        for ( uint32 x = 0; x < MESH_SIZE; ++x)
        {
            uint32 index0 = z * MESH_SIZE + x;
            uint32 index1 = index0 + 1;
            uint32 index2 = index1 + MESH_SIZE;
            uint32 index3 = index2 - 1;

            FMOD_VECTOR vertices[4];
            vertices[0] = *(( FMOD_VECTOR *) &(verts[index3]));
            vertices[1] = *(( FMOD_VECTOR *) &(verts[index2]));
            vertices[2] = *(( FMOD_VECTOR *) &(verts[index1]));
            vertices[3] = *(( FMOD_VECTOR *) &(verts[index0]));

            int index; //TODO: remove 'index' (replace with NULL if possible)
            FMOD_RESULT result = 
                pGeometry->addPolygon( directOcclusion, 
                                       reverbOcclusion, 
                                       doubleSided, 4, vertices, &index ); 
            ok &= SoundManager::FMOD_ErrCheck( result, "SoundOccluder::SoundOccluder" );
        }
    }

    geometries_.push_back( pGeometry );

    ok &= setActive(true);
    return ok;
}

bool SoundOccluder::setActive( bool active )
{
    BW_GUARD;
    bool ok = true;
    for( unsigned int i = 0; i < geometries_.size(); i++ )
    {
        FMOD_RESULT result = geometries_[i]->setActive( active );
        ok &= SoundManager::FMOD_ErrCheck( result, "SoundOccluder::setActive" );
    }
    return ok;
}

bool SoundOccluder::update( const Vector3& position, const Vector3& forward, const Vector3& up )
{
    BW_GUARD;
    bool ok = true;
    for( unsigned int i = 0; i < geometries_.size(); i++ )
    {
        FMOD_RESULT result = geometries_[i]->setPosition( (FMOD_VECTOR*)&position );
        SoundManager::FMOD_ErrCheck(result, "SoundOccluder::update couldn't set geometry position");

        if ( forward != Vector3::zero() )
        {
            MF_ASSERT( up != Vector3::zero() );
            MF_ASSERT( fabs(forward.dotProduct( up )) < 1.00e-10f );
            FMOD_RESULT result = geometries_[i]->setRotation( (FMOD_VECTOR*)&forward , (FMOD_VECTOR*)&up);
            SoundManager::FMOD_ErrCheck(result, "SoundOccluder::update couldn't set geometry rotation");
        }
    }
    return ok;
}

bool SoundOccluder::update( const Matrix& transform )
{
	BW_GUARD;
	return this->update( transform.applyToOrigin(), 
						 transform.applyToUnitAxisVector(2), 
						 transform.applyToUnitAxisVector(1) );
}

void SoundOccluder::debugDraw()
{
	BW_GUARD;
	for( unsigned int i = 0; i < geometries_.size(); i++ )
	{
		drawFMODGeometry( geometries_[i] );
	}	
}

BW_END_NAMESPACE

#endif //FMOD_SUPPORT
