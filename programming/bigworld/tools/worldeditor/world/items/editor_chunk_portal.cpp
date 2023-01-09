#include "pch.hpp"
#include "worldeditor/world/items/editor_chunk_portal.hpp"
#include "worldeditor/world/world_manager.hpp"
#include "worldeditor/editor/item_editor.hpp"
#include "worldeditor/editor/item_properties.hpp"
#include "worldeditor/project/project_module.hpp"
#include "worldeditor/misc/selection_filter.hpp"
#include "worldeditor/misc/options_helper.hpp"
#include "chunk/chunk_space.hpp"
#include "chunk/chunk_manager.hpp"
#include "chunk/chunk_obstacle.hpp"
#include "chunk/chunk_model_obstacle.hpp"
#include "chunk/chunk_model.hpp"
#include "chunk/geometry_mapping.hpp"
#include "chunk/umbra_chunk_item.hpp"
#include "scene/scene_object.hpp"
#include "physics2/worldtri.hpp"
#include "moo/render_context.hpp"
#include "moo/effect_constant_value.hpp"
#include "moo/effect_visual_context.hpp"
#include "appmgr/options.hpp"
#include "appmgr/module_manager.hpp"
#include "resmgr/xml_section.hpp"
#include "resmgr/string_provider.hpp"
#include "resmgr/auto_config.hpp"
#include "resmgr/resource_cache.hpp"
#include "common/material_utility.hpp"
#include <sstream>


BW_BEGIN_NAMESPACE

static AutoConfigString s_portalSelectionFx( "selectionfx/portal" );


namespace
{

class ResourcesHolder
{
public:
	static void acquirePortalMaterials( Moo::EffectMaterialPtr & portalMat,
										Moo::EffectMaterialPtr & portalSelectMat )
	{
		BW_GUARD;

		SimpleMutexHolder smh( s_mutex_ );

		++s_acquireCount_;
		if (s_acquireCount_ == 1)
		{
			s_portalMat_ = new Moo::EffectMaterial();
			s_portalMat_->load( BWResource::openSection( "resources/materials/editor_chunk_portal.mfm" ));
			MaterialUtility::viewTechnique( s_portalMat_.getObject(), "editorChunkPortal" );
			s_portalSelectMat_ = new Moo::EffectMaterial();
			s_portalSelectMat_->initFromEffect( s_portalSelectionFx );
		}

		portalMat = s_portalMat_;
		portalSelectMat = s_portalSelectMat_;
	}

	static void releasePortalMaterials()
	{
		BW_GUARD;

		SimpleMutexHolder smh( s_mutex_ );

		--s_acquireCount_;
		if (s_acquireCount_ == 0)
		{
			s_portalMat_ = NULL;
			s_portalSelectMat_ = NULL;
		}
	}

private:
	static SimpleMutex s_mutex_;
	static int s_acquireCount_;
	static Moo::EffectMaterialPtr s_portalMat_;
	static Moo::EffectMaterialPtr s_portalSelectMat_;
};

/*static*/ SimpleMutex ResourcesHolder::s_mutex_;
/*static*/ int ResourcesHolder::s_acquireCount_ = 0;
/*static*/ Moo::EffectMaterialPtr ResourcesHolder::s_portalMat_;
/*static*/ Moo::EffectMaterialPtr ResourcesHolder::s_portalSelectMat_;


class PointProxy : public StringProxy
{
	BW::string value_;
public:
	PointProxy( const BW::string& value )
		: value_( value )
	{}
	virtual BW::string get() const
	{
		return value_;
	}
	virtual void set( BW::string value, bool transient,
							bool addBarrier )
	{}
};


class PortalPointsProxy : public ArrayProxy
{
private:
	typedef BW::vector<GeneralProperty*> Properties;
	typedef ChunkBoundary::V2Vector V2Vector;

	Properties properties_;
	ChunkBoundary::Portal* pPortal_;

public:
	PortalPointsProxy( ChunkBoundary::Portal* pPortal ) :
		pPortal_( pPortal )
	{}

	~PortalPointsProxy()
	{}

	virtual bool readOnly()	{	return true;	}

	virtual BW::string asString() const
	{
		BW_GUARD;

		BW::string ret;

		for (V2Vector::iterator iter = pPortal_->points.begin();
			iter != pPortal_->points.end(); ++iter)
		{
			if (!ret.empty())
			{
				ret += ", ";
			}

			ret += iter->desc();
		}

		return ret;
	}

	virtual void elect( GeneralProperty* parent )
	{
		BW_GUARD;

		int index = 0;

		for (V2Vector::iterator iter = pPortal_->points.begin();
			iter != pPortal_->points.end(); ++iter)
		{
			++index;

			STATIC_LOCALISE_NAME( s_pointName, "COMMON/EDITOR_VIEWS/POINT_NAME" );

			properties_.push_back( new StaticTextProperty(
				s_pointName, new PointProxy( iter->desc() ) ) );

			properties_.back()->setGroup(
				parent->name()[0] ? parent->getGroup().str() + parent->name().c_str() :
				parent->getGroup() );
			properties_.back()->elect();
		}
	}

	virtual void expel( GeneralProperty* parent )
	{
		BW_GUARD;

		for (Properties::iterator iter = properties_.begin();
			iter != properties_.end(); ++iter)
		{
			(*iter)->expel();
			(*iter)->deleteSelf();
		}

		properties_.clear();
	}

	virtual void select( GeneralProperty* )
	{}

	virtual bool addItem()
	{
		return false;
	}

	virtual void delItem( int index )
	{
	}

	virtual bool delItems()
	{
		return false;
	}
};

} // anonymous namespace


// -----------------------------------------------------------------------------
// Section: EditorPortalObstacle
// -----------------------------------------------------------------------------

/**
 *	This class is the obstacle that a ChunkPortal puts in the collision scene
 */
class EditorPortalObstacle : public ChunkObstacle
{
public:
	EditorPortalObstacle( EditorChunkPortalPtr cpp );
	~EditorPortalObstacle() {}

	virtual bool collide( const Vector3 & source, const Vector3 & extent,
		CollisionState & state ) const;
	virtual bool collide( const WorldTriangle & source, const Vector3 & extent,
		CollisionState & state ) const;

	void buildTriangles();

private:
	EditorChunkPortalPtr	cpp_;
	BoundingBox				bb_;

	mutable BW::vector<WorldTriangle>	ltris_;
};


/**
 *	Constructor
 *  @param	cpp	The EditorChunkPortalPtr that will be used to create the obstacle
 */
EditorPortalObstacle::EditorPortalObstacle( EditorChunkPortalPtr cpp ) :
	ChunkObstacle( cpp->chunk()->transform(), &bb_, cpp ),
	cpp_( cpp )
{
	BW_GUARD;

	// now calculate our bb. fortunately the ChunkObstacle constructor
	// doesn't do anything with it except store it.
	const ChunkBoundary::Portal * pPortal = cpp_->pPortal_;
	MF_ASSERT(pPortal);

	// extend 10cm into the chunk (the normal is always normalised)
	Vector3 ptExtra = pPortal->plane.normal() * 0.10f;

	// build up the bb from the portal points
	for (uint i = 0; i < pPortal->points.size(); i++)
	{
		Vector3 pt =
			pPortal->uAxis * pPortal->points[i][0] +
			pPortal->vAxis * pPortal->points[i][1] +
			pPortal->origin;
		if (!i)
			bb_ = BoundingBox( pt, pt );
		else
			bb_.addBounds( pt );
		bb_.addBounds( pt + ptExtra );
	}

	// and figure out the triangles (a similar process)
	this->buildTriangles();
}


/**
 *	Build the 'world' triangles to collide with
 */
void EditorPortalObstacle::buildTriangles()
{
	BW_GUARD;

	ltris_.clear();

	const ChunkBoundary::Portal * pPortal = cpp_->pPortal_;
	MF_ASSERT(pPortal);

	// extend 5cm into the chunk
	Vector3 ptExOri = pPortal->origin + pPortal->plane.normal() * 0.05f;

	// build the wt's from the points
	Vector3 pto, pta, ptb(0.f,0.f,0.f);
	for (uint i = 0; i < pPortal->points.size(); i++)
	{
		// shuffle and find the next pt
		pta = ptb;
		ptb =
			pPortal->uAxis * pPortal->points[i][0] +
			pPortal->vAxis * pPortal->points[i][1] +
			ptExOri;

		// stop if we don't have enough for a triangle
		if (i < 2)
		{
			// start all triangles from pt 0.
			if (i == 0) pto = ptb;
			continue;
		}

		// make a triangle then
		ltris_.push_back( WorldTriangle( pto, pta, ptb ) );

		// make a second tri to test the portal both ways 
		// (for mouse selection which is all this is used for)
		ltris_.push_back( WorldTriangle( ptb, pta, pto ) );
	}
}


/**
 *	Collision test with an extruded point
 *	@param	source	the source point of the collision ray
 *	@param	extent	specifies how long the ray extends from its source point
 *	@param	state	the CollisionState object used to return result of collision
 *	@return	true if find any collision, false if find none
 */
bool EditorPortalObstacle::collide( const Vector3 & source, const Vector3 & extent,
	CollisionState & state ) const
{
	BW_GUARD;

	const ChunkBoundary::Portal * pPortal = cpp_->pPortal_;
	MF_ASSERT(pPortal);

	// see if we let anyone through
	//if (pPortal->permissive) return false;

	// check if selectable (unbound and belongo to an indoor chunk)
	if (pChunk()->isOutsideChunk())
	{
		return false;
	}

	// ok, see if they collide then
	// (chances are very high if they're in the bb!)
	Vector3 tranl = extent - source;
	for (uint i = 0; i < ltris_.size(); i++)
	{
		// see if it intersects (both ways)
		float rd = 1.0f;
		if (!ltris_[i].intersects( source, tranl, rd ) ) continue;

		// see how far we really travelled (handles scaling, etc.)
		float ndist = state.sTravel_ + (state.eTravel_-state.sTravel_) * rd;

		if (state.onlyLess_ && ndist > state.dist_) continue;
		if (state.onlyMore_ && ndist < state.dist_) continue;
		state.dist_ = ndist;

		// call the callback function
		ltris_[i].flags( uint8( cpp_->triFlags_ ) );

		CollisionObstacle ob( &transform_, &transformInverse_,
			this->pItem()->sceneObject() );
		int say = state.cc_.visit( ob, ltris_[i], state.dist_ );

		// see if any other collisions are wanted
		if (!say) return true;

		// some are wanted ... see if it's only one side
		state.onlyLess_ = !(say & 2);
        state.onlyMore_ = !(say & 1);
	}

	return false;
}

/**
 *	Collision test with an extruded triangle
 *	@param	source	the source triangle of the collision ray
 *	@param	extent	specifies how long the ray extends from its source triangle
 *	@param	state	the CollisionState object used to return result of collision
 *	@return	true if find any collision, false if find none
 */
bool EditorPortalObstacle::collide( const WorldTriangle & source, const Vector3 & extent,
	CollisionState & state ) const
{
	BW_GUARD;

	const ChunkBoundary::Portal * pPortal = cpp_->pPortal_;
	MF_ASSERT(pPortal);

	// see if we let anyone through
	//if (pPortal->permissive) return false;

	// check if selectable (unbound and belongo to an indoor chunk)
	if (pChunk()->isOutsideChunk())
	{
		return false;
	}

	// ok, see if they collide then
	// (chances are very high if they're in the bb!)
	const Vector3 tranl = extent - source.v0();
	for (uint i = 0; i < ltris_.size(); i++)
	{
		// see if it intersects (both ways)
		if (!ltris_[i].intersects( source, tranl ) ) continue;

		// see how far we really travelled
		float ndist = state.sTravel_;

		if (state.onlyLess_ && ndist > state.dist_) continue;
		if (state.onlyMore_ && ndist < state.dist_) continue;
		state.dist_ = ndist;

		// call the callback function
		ltris_[i].flags( uint8( cpp_->triFlags_ ) );

		CollisionObstacle ob( &transform_, &transformInverse_,
			this->pItem()->sceneObject() );
		int say = state.cc_.visit( ob, ltris_[i], state.dist_ );

		// see if any other collisions are wanted
		if (!say) return true;

		// some are wanted ... see if it's only one side
		state.onlyLess_ = !(say & 2);
        state.onlyMore_ = !(say & 1);
	}

	return false;
}



// -----------------------------------------------------------------------------
// Section: namespace Script
// -----------------------------------------------------------------------------

/**
 *	This is a special converter to represent a chunk pointer in python.
 *	It handles 'heaven' and 'earth' pseudo-pointers too.
 *	@param	pChunk	a chunk pointer contained in the portal object
 *	@return	a Python String identifier of pChunk
 */
PyObject * Script::getData( const Chunk * pChunk )
{
	BW_GUARD;

	if (uintptr(pChunk) > ChunkBoundary::Portal::LAST_SPECIAL )
	{
		BW::string fullid = pChunk->identifier() + "@" + pChunk->mapping()->name();
		return PyUnicode_DecodeUTF8( fullid.c_str(), fullid.length(), NULL );
	}

	switch (uintptr(pChunk))
	{
	case 0:
		Py_RETURN_NONE;
	case ChunkBoundary::Portal::HEAVEN:
		return PyUnicode_FromWideChar( L"heaven", 6 );
	case ChunkBoundary::Portal::EARTH:
		return PyUnicode_FromWideChar( L"earth", 5 );
	case ChunkBoundary::Portal::INVASIVE:
		return PyUnicode_FromWideChar( L"invasive", 7 );
	case ChunkBoundary::Portal::EXTERN:
		return PyUnicode_FromWideChar( L"extern", 6 );
	}

	return PyUnicode_FromWideChar( L"unknown_special", 15 );
}


// -----------------------------------------------------------------------------
// Section: EditorChunkPortal
// -----------------------------------------------------------------------------

PY_TYPEOBJECT( EditorChunkPortal )

PY_BEGIN_METHODS( EditorChunkPortal )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( EditorChunkPortal )
	PY_ATTRIBUTE( home )//"The chunk this portal belongs to"
	PY_ATTRIBUTE( triFlags )//"The triangle flag of this portal"
	PY_ATTRIBUTE( internal )//"This is set to true if this portal is an interal portal"
	PY_ATTRIBUTE( permissive )//"This is set to true if this portal is a permissive portal"
	PY_ATTRIBUTE( chunk )//"The chunk in the otherside of the portal"
	PY_ATTRIBUTE( points )//"The points of the portal"
	PY_ATTRIBUTE( uAxis )//"The uAxis of the portal"
	PY_ATTRIBUTE( vAxis )//"The vAxis of the portal"
	PY_ATTRIBUTE( origin )//"The origin of the portal"
	PY_ATTRIBUTE( lcentre )//"The center of the portal in local coordinate"
	PY_ATTRIBUTE( centre )//"The center of the portal in world coordinate"
	PY_ATTRIBUTE( plane_n )//"The normal of the plane that contains the portal"
	PY_ATTRIBUTE( plane_d )//"The d of the plane that contains the portal"
	PY_ATTRIBUTE( label )//"The label of the portal"
PY_END_ATTRIBUTES()



/**
 *	Constructor.
 *	@param	pPortal	the portal used to create the EditorChunkPortal object
 *	@param	pType	the pointer to PyTypeObject object that will be passed to parent object's ctor
 */
EditorChunkPortal::EditorChunkPortal( ChunkBoundary::Portal * pPortal, PyTypeObject * pType ):
	PyObjectPlus( pType ),
	pPortal_( pPortal ),
	triFlags_( 0 ),
	xform_( Matrix::identity )
{
	this->wantFlags_ = WantFlags( this->wantFlags_ | WANTS_DRAW );
}


/**
 *	Destructor.
 */
EditorChunkPortal::~EditorChunkPortal()
{
	if (portalMat_ != NULL)
	{
		BW_GUARD;

		ResourcesHolder::releasePortalMaterials();
	}
}


/**
 *	Get the points that form the boundary of this chunk
 *	@return	a tuple contains the points
 */
PyObject * EditorChunkPortal::pyGet_points()
{
	BW_GUARD;

	Py_ssize_t sz = pPortal_->points.size();
	PyObject * pTuple = PyTuple_New( sz );

	for (Py_ssize_t i = 0; i < sz; i++)
	{
		PyTuple_SetItem( pTuple, i, Script::getData( pPortal_->points[i] ) );
	}

	return pTuple;
}


class ProjSetter : public Moo::EffectConstantValue
{
public:
	bool operator()(ID3DXEffect* pEffect, D3DXHANDLE constantHandle)
	{
		BW_GUARD;

		pEffect->SetMatrix( constantHandle, &Moo::rc().projection() );
		return true;
	}
};

/**
 *	This class is the DrawItem used to draw the portals in World Editor
 */
class PortalDrawItem : public Moo::DrawContext::UserDrawItem
{
public:
	/**
	 *	Constructor
	 *	@param	points	4 vectors for the rectangle of the portal
	 *	@param	pMaterial	material used to draw the portal
	 *	@param	colour	the colour used to draw the portal
	 */
	PortalDrawItem( const BW::vector<Vector3>& points, Moo::EffectMaterialPtr pMaterial, uint32 colour )
	: pMaterial_( pMaterial ),
	  colour_( colour ),
	  points_( points )
	{
		BW_GUARD;

		pEffectConstantValue_ = Moo::rc().effectVisualContext().getMapping( "Projection" );
		setter_ = new ProjSetter;
	}

	/**
	 *	The overridden draw method
	 */
	void draw()
	{
		BW_GUARD;

		*pEffectConstantValue_ = setter_;
		Moo::rc().setFVF( Moo::VertexXYZL::fvf() );

		BW::vector<Moo::VertexXYZL> pVerts( points_.size() );
		for (unsigned int i = 0; i < points_.size(); i++)
		{
			pVerts[i].colour_ = colour_;
			pVerts[i].pos_ = points_[i];
		}

		pMaterial_->begin();
		for ( uint32 i=0; i<pMaterial_->numPasses(); i++ )
		{
			pMaterial_->beginPass(i);   
			Moo::rc().drawPrimitiveUP( D3DPT_TRIANGLEFAN, (UINT)pVerts.size() - 2, &pVerts[0], sizeof( Moo::VertexXYZL ) );
			pMaterial_->endPass();
		}
		pMaterial_->end();
	}
	/**
	 *	The overridden fini method
	 */
	void fini()
	{
		BW_GUARD;

		delete this;
	}
private:
	BW::vector<Vector3> points_;
	Moo::EffectMaterialPtr pMaterial_;
	Moo::Colour colour_;
	Moo::EffectConstantValuePtr* pEffectConstantValue_;
	SmartPointer<ProjSetter> setter_;
};

static bool s_drawHeavenAndEarth = false;
/**
 *	overridden edShouldDraw method
 */
bool EditorChunkPortal::edShouldDraw()
{
	BW_GUARD;

	if( !ChunkItem::edShouldDraw() )
		return false;
	if( Chunk::hideIndoorChunks_ )
		return false;

	// see if we should draw
	if (Moo::rc().frameTimestamp() != s_settingsMark_)
	{
		s_drawAlways_ = Options::getOptionBool(
			"render/drawChunkPortals", s_drawAlways_ ) &&
			OptionsScenery::visible();

		bool drawFlagOn = Options::getOptionInt(
			"render/misc/drawHeavenAndEarth", 0 ) != 0;
		drawFlagOn &= !!OptionsMisc::visible();
		bool projectModule = ProjectModule::currentInstance() == ModuleManager::instance().currentModule();
		s_drawHeavenAndEarth = (drawFlagOn && !projectModule);

		s_settingsMark_ = Moo::rc().frameTimestamp();
	}
	return s_drawAlways_;
}

/**
 *	Draw method to debug portal states
 */
void EditorChunkPortal::draw( Moo::DrawContext& drawContext )
{
	BW_GUARD;

	if( !edShouldDraw() )
		return;

	if (portalMat_ == NULL)
	{
		ResourcesHolder::acquirePortalMaterials( portalMat_, portalSelectMat_ );
	}

	// figure out what colour to draw it in
	uint32 colour = 0;
	if ( WorldManager::instance().isItemSelected(this) )
	{
		// portal is selected
		colour = 0xff008800;
	}
	else if (pPortal_->pChunk == NULL)
	{
		colour = 0xff000077;
	}
	else if (pPortal_->isExtern())
	{
		colour = 0xff888800;
	}
	else if (!pPortal_->label.empty())
	{
		colour = pPortal_->permissive ? 0xff003300 : 0xff550000;
	}
	// Only draw heaven for outside chunks
	else if (pPortal_->isHeaven() && s_drawHeavenAndEarth)
	{
		colour = 0xffaa00aa;
	}
	else if(chunk() && !chunk()->isOutsideChunk() && SelectionFilter::canSelect( this ))
	{
		colour = 0xff000077;
	}
	else if (pPortal_->isEarth() && s_drawHeavenAndEarth)
	{
		colour = 0xffff0000;
	}

	if (colour == 0) return;

	if (WorldManager::instance().drawSelection())
	{
		if (!SelectionFilter::canSelect( this ))
		{
			return;
		}

		BW::vector<Vector3> points( pPortal_->points.size() );
		for (unsigned int i = 0; i < pPortal_->points.size(); i++)
		{
			points[i] = Vector3( pPortal_->uAxis * pPortal_->points[i][0] +
				pPortal_->vAxis * pPortal_->points[i][1] + pPortal_->origin );
		}

		WorldManager::instance().registerDrawSelectionItem( this );

		Moo::rc().setFVF( Moo::VertexXYZ::fvf() );
		Moo::rc().device()->SetTransform( D3DTS_WORLD, &pChunk_->transform() );

		BW::vector<Moo::VertexXYZ> pVerts( points.size() );

		for( unsigned int i = 0; i < points.size(); ++i )
			pVerts[i].pos_ = points[i];

		portalSelectMat_->begin();
		for ( uint32 i=0; i<portalSelectMat_->numPasses(); i++ )
		{
			portalSelectMat_->beginPass(i);
			Moo::rc().drawPrimitiveUP( D3DPT_TRIANGLEFAN, (UINT)pVerts.size() - 2, &pVerts[0], sizeof( Moo::VertexXYZ ) );
			portalSelectMat_->endPass();
		}
		portalSelectMat_->end();

		Moo::rc().device()->SetTransform( D3DTS_WORLD, &Moo::rc().world() );
	}
	else
	{
		// get the transformation matrix
		Matrix tran;
		tran.multiply( Moo::rc().world(), Moo::rc().view() );

		// transform all the points
		BW::vector<Vector3> points( pPortal_->points.size() );
		for (unsigned int i = 0; i < pPortal_->points.size(); i++)
		{
			// project the point straight into clip space
			tran.applyPoint( points[i], Vector3(
				pPortal_->uAxis * pPortal_->points[i][0] +
				pPortal_->vAxis * pPortal_->points[i][1] +
				pPortal_->origin ) );
		}

		float distance = 0.f;
		for( BW::vector<Vector3>::iterator iter = points.begin();
			iter != points.end(); ++iter )
		{
			distance += iter->z;
		}

		distance /= points.size();
		drawContext.drawUserItem(  new PortalDrawItem( points, portalMat_.getObject(), colour ),
			Moo::DrawContext::TRANSPARENT_CHANNEL_MASK, distance );
	}
}


/**
 *	overridden toss method
 */
void EditorChunkPortal::toss( Chunk * pChunk )
{
	BW_GUARD;

	if (pChunk_ != NULL)
	{
		ChunkPyCache::instance( *pChunk_ ).del( pPortal_->label );
		ChunkModelObstacle::instance( *pChunk_ ).delObstacles( this );
	}

	this->ChunkItem::toss( pChunk );

	if (pChunk_ != NULL)
	{
		ChunkPyCache::instance( *pChunk_ ).add( pPortal_->label, this );
		ChunkModelObstacle::instance( *pChunk_ ).addObstacle(
			new EditorPortalObstacle( this ) );
		this->syncInit();
	}
}

/**
 *	Overridden edEdit method
 */
bool EditorChunkPortal::edEdit( GeneralEditor & editor )
{
	BW_GUARD;

	if (this->edFrozen())
		return false;

	if (!edCommonEdit( editor ))
	{
		return false;
	}

	MatrixProxy * pMP = new ChunkItemMatrix( this );

	static Name s_position_x( LocaliseStaticUTF8(L"WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_CHUNK_PORTAL/POSITION" )
		+ LocaliseUTF8(L"COMMON/EDITOR_VIEWS/X_NAME" ) );
	static Name s_position_y( LocaliseStaticUTF8(L"WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_CHUNK_PORTAL/POSITION" )
		+ LocaliseUTF8(L"COMMON/EDITOR_VIEWS/X_NAME" ) );
	static Name s_position_z( LocaliseStaticUTF8(L"WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_CHUNK_PORTAL/POSITION" )
		+ LocaliseUTF8(L"COMMON/EDITOR_VIEWS/X_NAME" ) );

	STATIC_LOCALISE_NAME( s_normal, "COMMON/EDITOR_VIEWS/NORMAL_NAME" );
	STATIC_LOCALISE_NAME( s_d, "COMMON/EDITOR_VIEWS/D_NAME" );
	STATIC_LOCALISE_NAME( s_uaxis, "COMMON/EDITOR_VIEWS/UAXIS_NAME" );
	STATIC_LOCALISE_NAME( s_localCentre, "COMMON/EDITOR_VIEWS/LOCAL_CENTRE_NAME" );
	STATIC_LOCALISE_NAME( s_points, "COMMON/EDITOR_VIEWS/POINTS_NAME" );
	STATIC_LOCALISE_NAME( s_extern, "COMMON/EDITOR_VIEWS/EXTERN" );
	STATIC_LOCALISE_NAME( s_otherChunk, "COMMON/EDITOR_VIEWS/OTHER_CHUNK" );

	editor.addProperty( new StaticTextProperty( s_position_x,
		new AccessorDataProxy< EditorChunkPortal, StringProxy >(
			this, "position.x", 
			&EditorChunkPortal::getX, 
			NULL ) ) );

	editor.addProperty( new StaticTextProperty( s_position_y,
		new AccessorDataProxy< EditorChunkPortal, StringProxy >(
			this, "position.y", 
			&EditorChunkPortal::getY, 
			NULL ) ) );

	editor.addProperty( new StaticTextProperty( s_position_z,
		new AccessorDataProxy< EditorChunkPortal, StringProxy >(
			this, "position.z", 
			&EditorChunkPortal::getZ, 
			NULL ) ) );

	editor.addProperty( new StaticTextProperty( s_normal,
		new AccessorDataProxy< EditorChunkPortal, StringProxy >(
			this, "normal", 
			&EditorChunkPortal::getNormal, 
			NULL ) ) );

	editor.addProperty( new StaticTextProperty( s_d,
		new AccessorDataProxy< EditorChunkPortal, StringProxy >(
			this, "d", 
			&EditorChunkPortal::getD, 
			NULL ) ) );

	editor.addProperty( new StaticTextProperty( s_uaxis,
		new AccessorDataProxy< EditorChunkPortal, StringProxy >(
			this, "uaxis", 
			&EditorChunkPortal::getUAxis, 
			NULL ) ) );

	editor.addProperty( new StaticTextProperty( s_localCentre,
		new AccessorDataProxy< EditorChunkPortal, StringProxy >(
			this, "local centre", 
			&EditorChunkPortal::getLocalCentre,
			NULL ) ) );

	editor.addProperty( new ArrayProperty( s_points,
		new PortalPointsProxy( pPortal_ ) ) );

	editor.addProperty( new StaticTextProperty( s_extern, 
		new AccessorDataProxy< EditorChunkPortal, StringProxy >(
			this, "extern", 
			&EditorChunkPortal::getExtern, 
			NULL ) ) );

	editor.addProperty( new StaticTextProperty( s_otherChunk, 
		new ConstantChunkNameProxy<EditorChunkPortal>( 
			this, &EditorChunkPortal::otherChunk ) ) );

	return true;
}

/**
 *	Overridden edBounds method
 */
void EditorChunkPortal::edBounds( BoundingBox& bbRet ) const
{
	BW_GUARD;

	const BW::vector<Vector2> & points = pPortal_->points;
	bbRet = BoundingBox::s_insideOut_;

	// first find the average in portal space again
	Vector2 avg( 0.f, 0.f );
	for (uint i = 0; i < points.size(); i++)
		avg += points[i];
	avg /= float( points.size() );

	// now build up the bounding box (also in portal space)
	for (uint i = 0; i < points.size(); i++)
	{
		const Vector2 & apt = points[i];
		bbRet.addBounds( Vector3( apt.x - avg.x, apt.y - avg.y, 0.f ) );
	}

	// and add a bit of depth
	bbRet.addBounds( Vector3( 0.f, 0.f, 0.2f ) );
}

/**
 *	Overridden edTransform method
 */
const Matrix & EditorChunkPortal::edTransform()
{
	BW_GUARD;

	//xform_.setTranslate( pPortal_->lcentre );
	xform_[0] = pPortal_->uAxis;
	xform_[1] = pPortal_->vAxis;
	xform_[2] = pPortal_->plane.normal();
	xform_[3] = pPortal_->lcentre;	// note: not pPortal->origin
	// we use the portal space centre for the matrix translation
	// instead of the origin ... causes a bit of trouble in edBounds
	// above, but overall it keeps everything a bit saner.
	return xform_;
}

/**
 *	need a section name for the selection filter
 *	@return	a static DataSectionPtr created solely for the use in SelectionFilter
 */
DataSectionPtr EditorChunkPortal::pOwnSect()
{
	static DataSectionPtr sect = new XMLSection( "portal" );

	return sect;
}

/**
 *	need a section name for the selection filter
 *	@return	a static DataSectionPtr created solely for the use in SelectionFilter
 */
const DataSectionPtr EditorChunkPortal::pOwnSect()	const
{
	static DataSectionPtr sect = new XMLSection( "portal" );

	return sect;
}

/**
 *	Overridden edSave method
 */
bool EditorChunkPortal::edSave( DataSectionPtr pSection )
{
	// do nothing, portals are saved with the chunk
	return true;
}

/**
 *	Return "True" if this portal is an external one, otherwise return "False"
 *	@return	"True" if it is external, "False" if not
 */
BW::string EditorChunkPortal::getExtern() const
{
	BW_GUARD;

	return pPortal_->isExtern() ? "True" : "False";
}

/**
 *	Return "True" if this portal is an invasive one, otherwise return "False"
 *	@return	"True" if it is invasive, "False" if not
 */
bool EditorChunkPortal::getInvasive() const
{
	BW_GUARD;

	return pPortal_->isInvasive();
}

/**
 *	Return the portal's label
 *	@return the label of the portal
 */
BW::string EditorChunkPortal::getLabel() const
{
	BW_GUARD;

	return pPortal_->label;
}

/**
 *	Set the portal's label
 *	@param	v	the new label
 *	@return	true is successful, otherwise false
 */
bool EditorChunkPortal::setLabel( const BW::string & v )
{
	pPortal_->label = v;
	return true;
}

/**
 *	Return x component of the portal's position as an BW::string
 *	@return x component of the portal's position as an BW::string
 */
BW::string EditorChunkPortal::getX() const
{
	BW_GUARD;

	Vector3 position = pPortal_->lcentre;

	if (chunk() != NULL)
	{
		position = chunk()->transform().applyPoint( position );
	}

	BW::ostringstream oss;
	oss << position.x;
	return oss.str();
}

/**
 *	Return y component of the portal's position as an BW::string
 *	@return y component of the portal's position as an BW::string
 */
BW::string EditorChunkPortal::getY() const
{
	BW_GUARD;

	Vector3 position = pPortal_->lcentre;

	if (chunk() != NULL)
	{
		position = chunk()->transform().applyPoint( position );
	}

	BW::ostringstream oss;
	oss << position.y;
	return oss.str();
}

/**
 *	Return z component of the portal's position as an BW::string
 *	@return z component of the portal's position as an BW::string
 */
BW::string EditorChunkPortal::getZ() const
{
	BW_GUARD;

	Vector3 position = pPortal_->lcentre;

	if (chunk() != NULL)
	{
		position = chunk()->transform().applyPoint( position );
	}

	BW::ostringstream oss;
	oss << position.z;
	return oss.str();
}

/**
 *	Return the normal of the portal's boundary as an BW::string
 *	@Return the normal of the portal's boundary as an BW::string
 */
BW::string EditorChunkPortal::getNormal() const
{
	return pPortal_->plane.normal().desc();
}

/**
 *	Return the d of the portal's boundary as an BW::string
 *	@Return the d of the portal's boundary as an BW::string
 */
BW::string EditorChunkPortal::getD() const
{
	BW::stringstream ss;

	ss << pPortal_->plane.d();

	return ss.str();
}

/**
 *	Return the uaxis of the portal's boundary as an BW::string
 *	@Return the uaxis of the portal's boundary as an BW::string
 */
BW::string EditorChunkPortal::getUAxis() const
{
	return pPortal_->uAxis.desc();
}

/**
 *	Return the local centre of the portal's boundary as an BW::string
 *	@Return the local centre of the portal's boundary as an BW::string
 */
BW::string EditorChunkPortal::getLocalCentre() const
{
	return pPortal_->lcentre.desc();
}

/**
 *	Return the chunk in the otherside of the portal
 *	@return	the chunk in the otherside of the portal
 */
Chunk * EditorChunkPortal::otherChunk() const
{
	return pPortal_->pChunk;
}

void EditorChunkPortal::syncInit()
{
	#if UMBRA_ENABLE
	BW_GUARD;

	bw_safe_delete(pUmbraDrawItem_);
	// Grab the visibility bounding box	
	BoundingBox bb = BoundingBox::s_insideOut_;
	for (unsigned int i = 0; i < pPortal_->points.size(); i++)
	{
		Vector3 p( pPortal_->uAxis * pPortal_->points[i][0] +
			pPortal_->vAxis * pPortal_->points[i][1] + pPortal_->origin );
		bb.addBounds(p);
	}

	// Set up object transforms
	Matrix m = pChunk_->transform();

	// Create the umbra chunk item
	UmbraChunkItem* pUmbraChunkItem = new UmbraChunkItem();
	pUmbraChunkItem->init( this, bb, m, pChunk_->getUmbraCell());
	pUmbraDrawItem_ = pUmbraChunkItem;

	this->updateUmbraLenders();
	#endif
}

// static initialisers
uint32 EditorChunkPortal::s_settingsMark_ = -16;
bool EditorChunkPortal::s_drawAlways_ = true;



// -----------------------------------------------------------------------------
// Section: ChunkPyCache
// -----------------------------------------------------------------------------


/**
 *	Constructor
 *	@param	chunk	the chunk used to create the cache
 */
ChunkPyCache::ChunkPyCache( Chunk & chunk ) :
	chunk_( chunk ),
	bound_( false )
{
}

/**
 *	Destructor
 */
ChunkPyCache::~ChunkPyCache()
{
}


/**
 *	Add this python object to our list of exposed items for this chunk
 *	@param	name	the label of the portal
 *	@param	pObject	pointer to an EditorChunkPortal object
 */
void ChunkPyCache::add( const BW::string & name, PyObject * pObject )
{
	BW_GUARD;
	// this is safe when overwriting
	exposed_[ name ] = pObject;
}

/**
 *	Remove this python object from our list of exposed items for this chunk
 *	@param	name	the label of the portal
 */
void ChunkPyCache::del( const BW::string & name )
{
	BW_GUARD;
	// this is safe when absent
	exposed_.erase( name );
}


/**
 *	Get the python object with the given name from this chunk
 *	@param	name	the label of the portal to get
 *	@return	a pointer to EditorChunkPortal object, or NULL if not found
 */
SmartPointer<PyObject> ChunkPyCache::get( const BW::string & name )
{
	BW_GUARD;
	NamedPyObjects::iterator found = exposed_.find( name );
	return found != exposed_.end() ? found->second : NULL;
}


/**
 *	Static method to get the given chunk inhabitant
 */
/*
PyObject * ChunkPyCache::chunkInhabitant( const BW::string & label,
	const BW::string & chunk, const BW::string & space )
{
	ChunkManager & cm = ChunkManager::instance();

	// look up the space
	ChunkSpacePtr pSpace = NULL;
	if (space.empty())
	{
		pSpace = cm.cameraSpace();
	}
	else
	{
		pSpace = cm.spaceFromName( space );
		if (!pSpace)
		{
			PyErr_Format( PyExc_ValueError, "BigWorld.chunkInhabitant(): "
				"space '%s' not found", space.c_str() );
			return NULL;
		}
	}

	// look up the chunk
	Chunk * pChunk = pSpace->findChunk( chunk );
	if (pChunk == NULL)
	{
		PyErr_Format( PyExc_ValueError, "BigWorld.chunkInhabitant(): "
			"chunk '%s' not found", chunk.c_str() );
		return NULL;
	}

	// look up the inhabitant
	SmartPointer<PyObject> spPyObj =
		ChunkPyCache::instance( *pChunk ).get( label );
	if (!spPyObj)
	{
		PyErr_Format( PyExc_ValueError, "BigWorld.chunkInhabitant(): "
			"no inhabitant with label '%s' found in chunk '%s'",
			label.c_str(), chunk.c_str() );
		return NULL;
	}

	// and return it!
	PyObject * pPyObj = spPyObj.getObject();
	Py_INCREF( pPyObj );
	return pPyObj;
}
PY_MODULE_STATIC_METHOD( ChunkPyCache, chunkInhabitant, BigWorld )
*/


/**
 *	Static method to find a chunk from a point
 */
/*
PyObject * ChunkPyCache::findChunkFromPoint( const Vector3 & point,
	const BW::string & space )
{
	ChunkManager & cm = ChunkManager::instance();

	// look up the space
	ChunkSpace * pSpace = NULL;
	if (space.empty())
	{
		pSpace = cm.cameraSpace();
	}
	else
	{
		pSpace = cm.spaceFromName( space );
		if (!pSpace)
		{
			PyErr_Format( PyExc_ValueError, "BigWorld.findChunkFromPoint(): "
				"space '%s' not found", space.c_str() );
			return NULL;
		}
	}

	// ask it to find the chunk
	Chunk * pChunk = pSpace->findChunkFromPoint( point );
	if (!pChunk)
	{
		char buf[1024];
		bw_snprintf( buf, sizeof(buf), "BigWorld.chunkInhabitant(): "
			"chunk at (%f,%f,%f) not found", point.x, point.y, point.z );
		PyErr_SetString( PyExc_ValueError, buf );
		return NULL;
	}

	// return the chunk identifier
	return Script::getData( pChunk->identifier() );
}
PY_MODULE_STATIC_METHOD( ChunkPyCache, findChunkFromPoint, BigWorld )
*/


/**
 *	overridden bind method
 */
void ChunkPyCache::bind( bool isUnbind )
{
	BW_GUARD;

	// only do this once
	if (bound_) return;
	bound_ = true;

	// go through all portals and create items for named ones,
	// whether bound or not.
	ChunkBoundaries::iterator			bit;
	ChunkBoundary::Portals::iterator	pit;

	// go through all our joint boundaries
	for (bit = chunk_.joints().begin(); bit != chunk_.joints().end(); bit++)
	{
		// go through all their bound portals
		for (pit = (*bit)->boundPortals_.begin();
			pit != (*bit)->boundPortals_.end();
			pit++)
		{
			if (!(*pit)->internal && !(*pit)->isExtern())
				chunk_.addStaticItem( ChunkItemPtr(new EditorChunkPortal( *pit ), true) );
		}

		// go through all their unbound portals too
		for (pit = (*bit)->unboundPortals_.begin();
			pit != (*bit)->unboundPortals_.end();
			pit++)
		{
			if (!(*pit)->internal && !(*pit)->isExtern())
				chunk_.addStaticItem( ChunkItemPtr(new EditorChunkPortal( *pit ), true) );
		}
	}
}

/**
 *	overridden touch method
 */
void ChunkPyCache::touch( Chunk & chunk )
{
	BW_GUARD;

	// make us exist in this chunk
	ChunkPyCache::instance( chunk );
}


/// Static instance accessor initialiser
ChunkCache::Instance<ChunkPyCache> ChunkPyCache::instance;

BW_END_NAMESPACE

// editor_chunk_portal.cpp
