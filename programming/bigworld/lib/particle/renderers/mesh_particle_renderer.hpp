#ifndef MESH_PARTICLE_RENDERER_HPP
#define MESH_PARTICLE_RENDERER_HPP

#include "base_mesh_particle_renderer.hpp"
#include "mesh_particle_sorter.hpp"
#include "moo/vertex_formats.hpp"
#include "moo/device_callback.hpp"
#include "moo/effect_material.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This class displays the particle system with each particle sharing a mesh.
 */
class MeshParticleRenderer :
	public BaseMeshParticleRenderer,
	/*
	*	Note: VisualCompound is always listening if it's pVisual_ has 
	*	been reloaded, if you are pulling info out from the visual
	*	then please update it again in onReloaderReloaded which is 
	*	called when the visual is reloaded so that related data
	*	will need be update again.
	*/
	public ReloadListener 
{
public:
	/// @name Constructor(s) and Destructor.
	//@{
	MeshParticleRenderer();
	~MeshParticleRenderer();
	//@}

	static void prerequisites(
		DataSectionPtr pSection,
		BW::set<BW::string>& output );


	/// An enumeration of the possible material effects.
	enum MaterialFX
	{
		FX_ADDITIVE,		
		FX_BLENDED,		
		FX_SOLID,		
		FX_MAX	// keep this element at the end
	};


	/// An enumeration of the possible sort types.
	enum SortType
	{
		SORT_NONE,
		SORT_QUICK,
		SORT_ACCURATE
	};


	/// @name Mesh particle renderer methods.
	//@{
	static bool isSuitableVisual( const BW::StringRef & v );
#ifdef EDITOR_ENABLED
	static bool quickCheckSuitableVisual( const BW::StringRef & v );
#endif
	virtual void visual( const BW::StringRef & v );
	virtual const BW::string& visual() const	{ return visualName_; }    

	MaterialFX materialFX() const		{ return materialFX_; }
	void materialFX( MaterialFX newMaterialFX );

    SortType sortType() const			{ return sortType_; }
    void sortType( SortType newSortType ) { sortType_ = newSortType; }

	bool doubleSided() const		{ return doubleSided_; }
	void doubleSided( bool s );
	//@}


	///	@name Renderer Overrides.
	//@{
	virtual void draw( Moo::DrawContext& drawContext,
		const Matrix & worldTransform,
		Particles::iterator beg,
		Particles::iterator end,
		const BoundingBox & bb );

	virtual void realDraw( const Matrix & worldTransform,
		Particles::iterator beg,
		Particles::iterator end )	{};

	virtual bool isMeshStyle() const	{ return true; }
	virtual ParticlesPtr createParticleContainer()
	{
		return new FixedIndexParticles;
	}

	virtual MeshParticleRenderer * clone() const;
	//@}


	// type of renderer
	virtual const BW::string & nameID() const { return nameID_; }
	static const BW::string nameID_;

	virtual size_t sizeInBytes() const { return sizeof(MeshParticleRenderer); }

	virtual void onReloaderReloaded( Reloader* pReloader );


protected:
	void loadInternal( DataSectionPtr pSect );
	void saveInternal( DataSectionPtr pSect ) const;

	template < typename Serialiser >
	void serialise( const Serialiser & ) const;

private:
	void drawOptimised(
		Moo::DrawContext& drawContext,
		const Matrix& worldTransform,
		Particles::iterator beg,
		Particles::iterator end,
		const BoundingBox & inbb,
		const bool isSorting );

	void drawSorted(
		Moo::DrawContext& drawContext,
		const Matrix& worldTransform,
		Particles::iterator beg,
		Particles::iterator end,
		const BoundingBox & inbb );

	bool beginDraw( const BoundingBox & inbb, const Matrix& worldTransform);
	void draw( Moo::DrawContext& drawContext, const BoundingBox& bb, int primGroupIndex = 0 );
	void endDraw(const BoundingBox & inbb);

    void fixMaterial();

	BW::string			visualName_;
	BW::string			textureName_;
	MeshParticleSorter	sorter_;	
	SortType    		sortType_;
	MaterialFX			materialFX_;
	bool				doubleSided_;
	bool				materialNeedsUpdate_;

	Moo::VerticesPtr	verts_;
	Moo::PrimitivePtr	prim_;
	Moo::Visual::PrimitiveGroup* primGroup_;
	Moo::ComplexEffectMaterialPtr material_;

	void updateMaterial();
	void pullInfoFromVisual();
};


typedef SmartPointer<MeshParticleRenderer> MeshParticleRendererPtr;


/*~ class Pixie.PyMeshParticleRenderer
 *	MeshParticleRenderer is a ParticleSystemRenderer which renders each particle
 *	as a mesh object.  MeshParticles are exported from 3D Studio Max using
 *	mesh particle option in Visual Exporter.  A MeshParticle .visual file contains
 *	up to 15 unique simple pieces of mesh.  The mesh particle renderer will use each
 *	one of these pieces as a single particle.
 *
 *  From BigWorld 1.8 and on, the MeshParticleRenderer can not render standard
 *	visuals.  You should use VisualParticleRenderer instead.
 */
class PyMeshParticleRenderer : public PyParticleSystemRenderer
{
	Py_Header( PyMeshParticleRenderer, PyParticleSystemRenderer )

public:
	/// @name Constructor(s) and Destructor.
	//@{
	PyMeshParticleRenderer( MeshParticleRendererPtr pR, PyTypeObject *pType = &s_type_ );	
	//@}

	/// An enumeration of the possible material effects.
	enum MaterialFX
	{
		FX_ADDITIVE,		
		FX_BLENDED,		
		FX_SOLID,		
		FX_MAX	// keep this element at the end
	};


	/// An enumeration of the possible sort types.
	enum SortType
	{
		SORT_NONE,
		SORT_QUICK,
		SORT_ACCURATE
	};

	/// @name Mesh particle renderer methods.
	//@{	
	virtual void visual( const BW::string& v )	{ pR_->visual(v); }
	virtual const BW::string& visual() const	{ return pR_->visual(); }    

	MaterialFX materialFX() const		{ return (MaterialFX)pR_->materialFX(); }
	void materialFX( MaterialFX newMaterialFX );

	SortType sortType() const			{ return (SortType)pR_->sortType(); }
    void sortType( SortType newSortType );

	const bool doubleSided() const		{ return pR_->doubleSided(); }
	void doubleSided( bool s )			{ pR_->doubleSided(s); }
	//@}


	// Python Wrappers for handling Emumerations in Python.
	PY_BEGIN_ENUM_MAP( MaterialFX, FX_ )
		PY_ENUM_VALUE( FX_ADDITIVE )
		PY_ENUM_VALUE( FX_SOLID )		
		PY_ENUM_VALUE( FX_BLENDED )		
	PY_END_ENUM_MAP()

	PY_BEGIN_ENUM_MAP( SortType, SORT_ )
		PY_ENUM_VALUE( SORT_NONE )
		PY_ENUM_VALUE( SORT_QUICK )		
		PY_ENUM_VALUE( SORT_ACCURATE )		
	PY_END_ENUM_MAP()
	//@}


	///	@name Python Interface to the MeshParticleRenderer.
	//@{
	PY_FACTORY_DECLARE()

	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( BW::string, visual, visual )
	PY_DEFERRED_ATTRIBUTE_DECLARE( materialFX )
    PY_DEFERRED_ATTRIBUTE_DECLARE( sortType )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( bool, doubleSided, doubleSided )	
	//@}
private:
	MeshParticleRendererPtr	pR_;
};


PY_ENUM_CONVERTERS_DECLARE( PyMeshParticleRenderer::MaterialFX )
PY_ENUM_CONVERTERS_DECLARE( PyMeshParticleRenderer::SortType )

BW_END_NAMESPACE

#endif // MESH_PARTICLE_RENDERER_HPP
