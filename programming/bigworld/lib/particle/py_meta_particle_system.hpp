#ifndef PY_META_PARTICLE_SYSTEM_HPP
#define PY_META_PARTICLE_SYSTEM_HPP

#include "meta_particle_system.hpp"
#include "cstdmf/bgtask_manager.hpp"
#include "duplo/py_attachment.hpp"


BW_BEGIN_NAMESPACE

/*~ class Pixie.PyMetaParticleSystem
 *
 *	The PyMetaParticleSystem can contain several PyParticleSystem objects, 
 *	allowing them to function as a single object. As a PyAttachment, 
 *	the PyMetaParticleSystem can be attached to other BigWorld objects.
 *
 *	A new PyMetaParticleSystem is created using Pixie.MetaParticleSystem
 *	function.
 */

/**
 *	This class is the python wrapper for MetaParticleSystem.
 */
class PyMetaParticleSystem : public PyAttachment
{
	Py_Header( PyMetaParticleSystem, PyAttachment )
public:
	PyMetaParticleSystem( MetaParticleSystemPtr pSystem, PyTypeObject *pType = &s_type_ );

	/// @name create - this method creates either Meta or standard Particle systems,
	///	as such it is only particularly useful for python scripts.
	//@{
	static PyObject* create( const BW::string& filename );
	PY_AUTO_MODULE_STATIC_METHOD_DECLARE( RETOWN, create, ARG( BW::string, END ) )
	//@}

	/// @name createBG - this method creates a Meta or 
	/// Particle systems in the background thread
	//@{
	static PyObject* createBG( const BW::string& filename, PyObjectPtr callback );
	PY_AUTO_MODULE_STATIC_METHOD_DECLARE( RETOWN, createBG, ARG( BW::string, ARG( PyObjectPtr, END ) ) )
	//@}

	MetaParticleSystemPtr	pSystem() { return pSystem_; }

	/// @name PyAttachment overrides
	//@{
	virtual void tick( float dTime );
	virtual bool isLodVisible() const;
	virtual bool needsUpdate() const;
#if ENABLE_TRANSFORM_VALIDATION
	virtual bool isTransformFrameDirty( uint32 frameTimestamp ) const;
	virtual bool isVisibleTransformFrameDirty( uint32 frameTimestamp ) const;
#endif // ENABLE_TRANSFORM_VALIDATION
	virtual void updateAnimations( const Matrix & worldTransform,
		float lod );
	virtual void draw( Moo::DrawContext& drawContext, const Matrix & worldTransform );

	virtual void localBoundingBox( BoundingBox & bb, bool skinny = false );
	virtual void localVisibilityBox( BoundingBox & vbb, bool skinny = false );
	virtual void worldBoundingBox( BoundingBox & bb, const Matrix& world, bool skinny = false );
	virtual void worldVisibilityBox( BoundingBox & vbb, const Matrix& world, bool skinny = false );

	virtual bool attach( MatrixLiaison * pOwnWorld );
	virtual void detach();

	virtual void tossed( bool outside );

	virtual void enterWorld();
	virtual void leaveWorld();

	virtual void move( float dTime );
	//@}

	float duration() const;
	uint32 nSystems() const;

	/// @name The Python Interface to the Particle System.
	//@{
	PY_FACTORY_DECLARE()
	PY_METHOD_DECLARE( py_update )
	PY_METHOD_DECLARE( py_render )
	PY_METHOD_DECLARE( py_load )
	PY_METHOD_DECLARE( py_save )
	PY_METHOD_DECLARE( py_addSystem )
	PY_METHOD_DECLARE( py_removeSystem )
	PY_METHOD_DECLARE( py_removeAllSystems )
	PY_METHOD_DECLARE( py_system )
	PY_METHOD_DECLARE( py_force )
	PY_METHOD_DECLARE( py_clear )
	PY_AUTO_METHOD_DECLARE( RETDATA, duration, END )
	PY_AUTO_METHOD_DECLARE( RETDATA, nSystems, END )
	//@}
private:
	MetaParticleSystemPtr	pSystem_;
	bool dirty_;
	float lod_;
};


/**
 *	This class implements background loading of particle systems.
 */
class MPSBackgroundTask : public BackgroundTask
{
public:
	MPSBackgroundTask( const BW::string& filename, PyObject * callback );
	void doBackgroundTask( TaskManager & mgr );
	void doMainThreadTask( TaskManager & mgr );
private:
	BW::string				filename_;
	MetaParticleSystemPtr	pSystem_;
	SmartPointer<PyObject>	pCallback_;
};


typedef SmartPointer<MPSBackgroundTask> MPSBackgroundTaskPtr;

BW_END_NAMESPACE

#endif //PY_META_PARTICLE_SYSTEM_HPP
