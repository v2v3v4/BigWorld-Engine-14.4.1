#ifndef PYMODEL_HPP
#define PYMODEL_HPP

#include <memory>

#include "pyscript/pyobject_plus.hpp"
#include "pyscript/script_math.hpp"
#include "pyscript/stl_to_py.hpp"

#include "py_attachment.hpp"
#include "action_queue.hpp"

#include "cstdmf/bgtask_manager.hpp"
#include "cstdmf/dogwatch.hpp"

#include "duplo/ik_constraint_system.hpp"

#include "math/boundbox.hpp"
#include "math/matrix.hpp"

#include "moo/reload.hpp"

namespace BW
{
	class PyMaterialFashion;
	class PyTransformFashion;
}

BW_BEGIN_NAMESPACE

namespace Moo
{
	class Node;
	typedef SmartPointer<Node> NodePtr;
}

class Attachment;
class IKConstraintSystem;
class SuperModel;
class PyFxSound;
class SoundSource;
class Motor;

class PySound;
class PySoundList;

class PyModelNode;
typedef BW::vector<PyModelNode*>	PyModelNodes;

class PyModel;
typedef ScriptObjectPtr<PyModel> PyModelPtr;

/**
 *	This class properly aligns an attachment whose hardpoint node is
 *	animated (either directly or indirectly by parents moving)
 */
class ModelAligner : public TransformFashion
{
public:
	ModelAligner( SuperModel * pSuperModel, Moo::NodePtr pNode );

	const Matrix & staticInverse() const	{ return staticInverse_; }

	/**
	 *	ModelAligner always requires models to re-calculate their
	 *	transform state after this fashion is applied.
	 *	@return true
	 */
	bool needsRedress() const { return true; }

private:
	virtual void dress( SuperModel & superModel, Matrix& world );

	Moo::NodePtr	pNode_;
	Matrix			staticInverse_;
};

typedef SmartPointer<ModelAligner> ModelAlignerPtr;

/**
 *	Helper class to create pyModel for scheduled model loading
 */
class ScheduledModelLoader : public BackgroundTask
{
public:
	ScheduledModelLoader( PyObject * bgLoadCallback,
			const BW::vector< BW::string > & modelNames );

	virtual void doBackgroundTask( TaskManager & mgr );
	virtual void doMainThreadTask( TaskManager & mgr );

private:
	PyObjectPtr pBgLoadCallback_;
	SuperModel * pSuperModel_;
	BW::vector< BW::string >	modelNames_;
};

/*~ class BigWorld.PyModel
 *
 *	PyModels are renderable objects which can be moved around the world,
 *  animated, dyed (have materials programatically assigned) and attached to.
 *
 *	PyModels are constructed using the BigWorld.Model function, which takes the
 *  resource id of one or more .model files. Each of these individual models is
 *  loaded, and combined into a supermodel, which is the PyModel.
 *
 *	For example, to load a body and a head model into one supermodel:
 *
 *	@{
 *	model = BigWorld.Model( "models/body.model", "models/head.model" )
 *	@}
 *
 *	Models which are attached to entities can have Motors.  These are objects
 *  which control how the model moves in response to its parent entity moving.
 *	The default Motor for a PyModel is an ActionMatcher which also takes care
 *  of synchronising animations with the movement of the model (for example,
 *  playing Idle when it stands still, and Walk when it moves at walking
 *  speed).
 *
 *	In order to animate a PyModel, you play actions on it.  You do this by
 *	calling an ActionQueuer object.  These are available as named attributes from the
 *  PyModel, as well as by using the action function to look up the name.
 *
 *	For example:
 *	@{
 *	ac1 = model.Walk
 *	ac2 = model.action("Walk")
 *	@}
 *	both obtain the same ActionQueuer object.  The action can be played by
 *  calling the object, as follows:
 *	@{
 *	model.Walk()
 *	@}
 *
 *	Actions are defined in the .model file.
 *
 *	A PyModel can also have one or more instances of PyDye.  These are
 *  available as named attributes, in exactly the same way that actions are.
 *	The PyDye can be assigned a string which names a particular tint, which
 *  will change the material which is applied to a particular piece of geometry
 *  on the model.
 *
 *	For example:
 *	@{
 *	# Apply the Red tint (which is defined in the .model file) to the Chest
 *  # PyDye.
 *	model.Chest = "Red"
 *	@}
 *
 *	In addition, a PyModel can have hardpoints.  A hardpoint is a node
 *	that has a prefix of "HP_" on its name.  This is defined in the .model file.
 *	A PyAttachment can be attached to a hardpoint by assigning it to an attribute
 *	with that hardpoints name.
 *
 *	For example, if there were a node named HP_RightHand:
 *	@{
 *	# attach the sword model to the biped's right hand
 *	biped.RightHand = sword
 *	@}
 */
/**
 *	A PyModel is a Scene Node with an animation player,
 *	an action queue and the ability to attach models to hardPoints.
 *
 *	A PyModel provides script access to a SuperModel.
 *
 *	PyModels do a lot of other cools things too.
 *
 * PyModelBase extends Aligned, so we don't have to
 */
class PyModel :
	public PyAttachment,
	/*
	*	Note: PyModel is always listening if pSuperModel_ has 
	*	been reloaded, if you are pulling info out from the pSuperModel_
	*	then please update it again in onReloaderReloaded which is 
	*	called when pSuperModel_ is reloaded so that related data
	*	will need be update again.and you might want to do something
	*	in onReloaderPriorReloaded which happen right before the pSuperModel_ reloaded.
	*/
	public ReloadListener
{
	Py_Header( PyModel, PyAttachment )

public:
	~PyModel();

	// PyAttachment overrides
	void tick( float dTime );
	bool needsUpdate() const;
#if ENABLE_TRANSFORM_VALIDATION
	bool isTransformFrameDirty( uint32 frameTimestamp ) const;
	bool isVisibleTransformFrameDirty( uint32 frameTimestamp ) const;
#endif // ENABLE_TRANSFORM_VALIDATION
	virtual void updateAnimations( const Matrix & worldTransform,
		float lod );
	void draw( Moo::DrawContext& drawContext, const Matrix & worldTransform );

	virtual void tossed( bool outside );

	bool visible() const			{ return visible_; }
	void visible( bool v )			{ visible_ = v; }

	bool visibleAttachments() const			{ return visibleAttachments_; }
	void visibleAttachments( bool va )		{ visibleAttachments_ = va; }

	bool outsideOnly() const		{ return outsideOnly_; }
	void outsideOnly( bool oo )		{ outsideOnly_ = oo; }

	float moveScale() const			{ return moveScale_; }
	void moveScale( float m )		{ moveScale_ = m; }

	float actionScale() const		{ return actionScale_; }
	void actionScale( float a )		{ actionScale_ = a; }

	float height() const; 
	void height( float h ); 

	float yaw() const; 
	void yaw(float yaw); 

	virtual void onReloaderReloaded( Reloader* pReloader );
	virtual void generateTextureUsage( Moo::ModelTextureUsageGroup & usageGroup ) const;
	virtual void textureUsageWorldBox( BoundingBox & bb, const Matrix & world )  const;

	// PyObjectPlus overrides
	ScriptObject pyGetAttribute( const ScriptString & attrObj );
	bool pySetAttribute( const ScriptString & attrObj,
		const ScriptObject & value );
	bool pyDelAttribute( const ScriptString & attrObj );

	// Script related methods
	PY_FACTORY_DECLARE()

	PY_METHOD_DECLARE( py_action );

	PyObject* getSound( const BW::string& s );
	PY_AUTO_METHOD_DECLARE( RETOWN, getSound, ARG( BW::string, END ) );

	PyObject* playSound( const BW::string& s );
	PY_AUTO_METHOD_DECLARE( RETOWN, playSound, ARG( BW::string, END )  );

	bool stopSoundsOnDestroy() const { return cleanupSounds_; }
	void stopSoundsOnDestroy( bool enable ){ cleanupSounds_ = enable; }
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( bool,
		stopSoundsOnDestroy, stopSoundsOnDestroy );

#if FMOD_SUPPORT
	bool attachSound( PySound *pySound );
	bool updateSound( PySound *pySound );
	bool detachSound( PySound *pySound );
	PySoundList &sounds();

protected:
	// Sound-related helper methods
	PySound* createSoundInternal( const BW::string& s );
	void cleanupSounds();
#endif // FMOD_SUPPORT

public:

	PY_METHOD_DECLARE( py_addMotor )
	PY_METHOD_DECLARE( py_delMotor )
	PyObject * pyGet_motors();
	int pySet_motors( PyObject * value );

	// This function is for a hack to keep entity collisions working...
	Motor * motorZero() { return motors_.size() > 0 ? motors_[0] : NULL; }

	//  Attributes
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( bool, visible, visible )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( bool, visibleAttachments, visibleAttachments )
	PY_RW_ATTRIBUTE_DECLARE( moveAttachments_, moveAttachments )
	PY_RW_ATTRIBUTE_DECLARE( independentLod_, independentLod )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( bool, outsideOnly, outsideOnly )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, moveScale, moveScale )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, actionScale, actionScale )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, height, height ) 
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, yaw, yaw ) 

	PyObject * pyGet_sources();
	PY_RO_ATTRIBUTE_SET( sources )

	PyObject * pyGet_position();
	int pySet_position( PyObject * value );

	PyObject * pyGet_scale();
	int pySet_scale( PyObject * value );

	PY_RO_ATTRIBUTE_DECLARE( unitRotation_, unitRotation )

	void straighten();
	void rotate( float angle, const Vector3 & vAxis,
		const Vector3 & vCentre = Vector3( 0.f, 0.f, 0.f ) );
	void alignTriangle( const Vector3 & vertex0, const Vector3 & vertex1,
		const Vector3 & vertex2, bool randomYaw = true );
	Vector3 reflectOffTriangle( const Vector3 & vertex0,
		const Vector3 & vertex1,
		const Vector3 & vertex2,
		const Vector3 & fwd );
	void zoomExtents( bool upperFrontFlushness, float xzMutiplier = 1.f );

	PY_AUTO_METHOD_DECLARE( RETVOID, straighten, END )
	PY_AUTO_METHOD_DECLARE( RETVOID, rotate, ARG( float, ARG( Vector3,
		OPTARG( Vector3, Vector3( 0.f, 0.f, 0.f ), END ) ) ) )
	PY_AUTO_METHOD_DECLARE( RETVOID, alignTriangle, ARG( Vector3, ARG( Vector3,
		ARG( Vector3, OPTARG( bool, true, END ) ) ) ) )
	PY_AUTO_METHOD_DECLARE( RETDATA, reflectOffTriangle, ARG( Vector3, ARG( Vector3,
		ARG( Vector3, ARG( Vector3, END ) ) ) ) )
	PY_AUTO_METHOD_DECLARE( RETVOID, zoomExtents, ARG( bool, OPTARG( float, 1.f, END ) ) )

	PY_RO_ATTRIBUTE_DECLARE( actionQueue_.queueState(), queue )

	PyObject * pyGet_bounds();
	PY_RO_ATTRIBUTE_SET( bounds )

	PY_RW_ATTRIBUTE_DECLARE( tossCallback_, tossCallback )

	PyObject * node( const BW::string & nodeName, MatrixProviderPtr local = NULL );
	PY_AUTO_METHOD_DECLARE( RETOWN, node, ARG( BW::string, OPTARG( MatrixProviderPtr, NULL, END ) ) );

	PY_RO_ATTRIBUTE_DECLARE( SmartPointer<PyObject>( this->node(""), true ), root );

	bool cue( const BW::string & identifier, SmartPointer<PyObject> function );
	PY_AUTO_METHOD_DECLARE( RETOK, cue, ARG( BW::string,
		ARG( SmartPointer<PyObject>, END ) ) )

	PY_METHOD_DECLARE( py_saveActionQueue )
	PY_METHOD_DECLARE( py_restoreActionQueue )

	// accessors
	SuperModel *		pSuperModel() const;

	ActionQueue &		actionQueue( void );

	// action queue methods
	void				unitTransform( const Matrix& transform );

	// action matcher methods
	void				unitOffset( const Vector3& offset );
	void				unitRotation( float yaw );
	float				unitRotation();

	// entity method (called from Entity's tick)
	void				move( float dTime );

	void				enterWorld();
	void				leaveWorld();

	virtual void		localBoundingBox( BoundingBox & bb, bool skinny = false );
	virtual void		localVisibilityBox( BoundingBox & bb, bool skinny = false );
	virtual void		worldBoundingBox( BoundingBox & bb, const Matrix& world, bool skinny = false );
	virtual void		worldVisibilityBox( BoundingBox & vbb, const Matrix& world, bool skinny = false );

	void				shouldDrawSkeleton( bool drawIt )
									{ shouldDrawSkeleton_ = drawIt; }

	void				setDebugString( const char* id,
							const BW::string& str )
							{
								if (pDebugInfo_ == NULL)
								{
									pDebugInfo_ = new DebugStrings;
								}
								(*pDebugInfo_)[id] = str;
							}

	BW::string			name() const;

	const Matrix &		initialItinerantContext() const
								{ return initialItinerantContext_; }
	const Matrix &		initialItinerantContextInverse() const
								{ return initialItinerantContextInverse_; }

	bool origin( const BW::string & nodeName );
	PY_AUTO_METHOD_DECLARE( RETOK, origin, ARG( BW::string, END ) )

	static PyModel *pCurrent() { return s_pCurrent_; }
	static void pCurrent( PyModel *pModel ){ s_pCurrent_ = pModel; }
	
	void worldTransform( const Matrix & worldTransform );
	const Matrix & worldTransform() const;

	SmartPointer<IKConstraintSystem> getIKConstraintSystemFromData() const;
	PY_AUTO_METHOD_DECLARE(
		RETDATA, getIKConstraintSystemFromData, END );
	
	static PyModelPtr createTestPyModel(
		std::auto_ptr<SuperModel> superModel );

	virtual const PyModel * getPyModel() const { return this; };

private:
	explicit PyModel(
		SuperModel * pSuperModel, PyTypeObject* pType = &PyModel::s_type_ );
	PyModel( const PyModel & );
	PyModel & operator=( const PyModel & );

	class ScopedPyModelWatchHolder
	{
	public:
		ScopedPyModelWatchHolder( DogWatch& pyModelWatch );
		~ScopedPyModelWatchHolder();
	private:
		DogWatch& pyModelWatch_;
		static int running_;
	};

	class ScopedCurrentModelHolder
	{
	public:
		ScopedCurrentModelHolder( PyModel& pyModel );
		~ScopedCurrentModelHolder();
	private:
		PyModel& pyModel_;
	};

	void pullInfoFromSuperModel( bool visualOnly = false );

	void calculateBoundingBoxes();
	void localBoundingBoxInternal( BoundingBox& bb ) const;

	bool isLodVisible() const;
	bool animateAsAttachment();
	void updateAsAttachment( const Matrix& thisTransform,
		TransformFashion * pCouplingFashion,
		float lod );
	void updateSuperModelAsAttachment( const Matrix& thisTransform,
		TransformFashion * pCouplingFashion,
		float lod );
	void collectTransformFashions(
		TmpTransforms& preTransformFashions,
		TmpTransforms& postTransformFashions,
		TransformFashion* pCouplingFashion );
	void updateDummyNodeAsAttachment( const Matrix & thisWorld,
		float lod );

	void updateAttachments( const Matrix & world,
		float lod );

	void drawAsAttachment( Moo::DrawContext& drawContext,
							const Matrix& thisTransform );
	void drawSuperModelAsAttachment( Moo::DrawContext& drawContext,
									const Matrix& thisTransform );
	void drawDummyModelAsAttachment( Moo::DrawContext& drawContext );

	void drawAttachments( Moo::DrawContext& drawContext, const Matrix& attachmentsTransform );

	void detach();

	PyModelNode * accessNode( Moo::NodePtr pNode, MatrixProviderPtr = NULL );

	BW::PyMaterialFashion * getMaterial( const char * attr );
	void setMaterial( const char * attr,
		PyMaterialFashion * pPyMaterialFashion );
	bool removeMaterial( const char * attr );
		PyTransformFashion * getTransform( const char * attr );
	void setTransform( const char * attr,
		PyTransformFashion * pPyTransformFashion );
	bool removeTransform( const char* attr );

	SuperModel			*pSuperModel_;
	SmartPointer<PyModelNode> dummyModelNode_;
	ActionQueue			actionQueue_;

	BoundingBox			localBoundingBox_;
	BoundingBox			localVisibilityBox_;
	float				height_;
	bool				visible_;
	bool				visibleAttachments_;
	bool				moveAttachments_;
	bool				independentLod_;
	bool				outsideOnly_;
	bool				dirty_;

	float				moveScale_;
	float				actionScale_;

	// PyFashions
	typedef
	StringHashMap< SmartPointer<PyTransformFashion> > PyTransformFashions;
	PyTransformFashions prePyTransformFashionsMap_;
	PyTransformFashions postPyTransformFashionsMap_;

	typedef
	StringHashMap< SmartPointer<PyMaterialFashion> > PyMaterialFashions;
	PyMaterialFashions pyMaterialFashionsMap_;

	// Fashions
	ModelAlignerPtr		pCouplingFashion_;
	MaterialFashionVector materialFashions_;

	PyModelNodes		knownNodes_;

	Matrix				unitTransform_;
	Vector3				unitOffset_;
	float				unitRotation_;
	bool				transformModsDirty_;

	void				drawSkeleton();
	void				drawNodeLabels( Moo::DrawContext& drawContext );

	void				drawDebugInfo( Moo::DrawContext& drawContext );

	bool				shouldDrawSkeleton_;

	typedef StringHashMap<BW::string> DebugStrings;
	DebugStrings *		pDebugInfo_;

#if FMOD_SUPPORT
	PySoundList			*pSounds_;
#endif
	bool				cleanupSounds_;

	typedef BW::vector<Motor*>	Motors;
	Motors				motors_;

	Matrix				initialItinerantContext_;
	Matrix				initialItinerantContextInverse_;

	SmartPointer<PyObject>	tossCallback_;

	static PyModel *s_pCurrent_;

	friend class ScheduledModelLoader;

	// Cache for frame transforms
	struct FrameTransformCache
	{
#if ENABLE_TRANSFORM_VALIDATION
		uint32 transformFrameTimestamp_;
#endif // ENABLE_TRANSFORM_VALIDATION

		Matrix attachmentsTransform_;
		bool didAnimate_;
		Matrix thisTransform_;
		TransformFashion * thisFrameCouplingFashion_;
	};
	FrameTransformCache frameTransformCache_;

public:
	static bool			drawSkeletons_;
	static bool			drawSkeletonWithCoordAxes_;
	static bool			drawNodeLabels_;
	static void			debugStuff( float dTime );
};

#ifndef PyModel_CONVERTERS
	PY_SCRIPT_CONVERTERS_DECLARE( PyModel )
	#define PyModel_CONVERTERS
#endif


#ifdef CODE_INLINE
#include "pymodel.ipp"
#endif

BW_END_NAMESPACE


#endif // PYMODEL_HPP
