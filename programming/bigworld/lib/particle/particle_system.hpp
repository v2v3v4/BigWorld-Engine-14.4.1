#ifndef PARTICLE_SYSTEM_HPP
#define PARTICLE_SYSTEM_HPP

// Standard MF Library Headers.
#include "math/matrix_liason.hpp"
#include "moo/moo_math.hpp"
#include "moo/node.hpp"
#include "romp/romp_collider.hpp"
#include "particles.hpp"
#include "particle/actions/particle_system_action.hpp"
#include "particle/renderers/particle_system_renderer.hpp"
#include "math/boundbox.hpp"
#include "cstdmf/bw_set.hpp"


BW_BEGIN_NAMESPACE

// Forward Class Declarations.
class ParticleSystem;
typedef SmartPointer<ParticleSystem> ParticleSystemPtr;


/**
 *	This class is the abstract picture of the particles that is invariant
 *	to a renderer.
 */
class ParticleSystem : public ReferenceCount
{
public:

	static const BW::StringRef VERSION_STRING;
	static const float LOD_INFINITE;

	/// @name Constructor(s) and Destructor.
	//@{
	ParticleSystem( int initialCapacity = 100 );
	~ParticleSystem();
	//@}

	/// A collection of Actions for a Particle System.
	typedef BW::vector<ParticleSystemActionPtr> Actions;

	/// @name Direct Accessors to Particles.
	//@{
	Particles::iterator begin( void );
	Particles::const_iterator begin( void ) const;

	Particles::iterator end( void );
	Particles::const_iterator end( void ) const;

	const ParticlesPtr& particles() const	{ return particles_; }
	//@}

	// Return the resources required for a ParticleSystem
	static void prerequisites( DataSectionPtr pSection, BW::set<BW::string>& output );

	// Create a deep copy of the particle system.
	ParticleSystem * clone() const;

	/// @name methods to make us look like a py attachment 
	//@{
	virtual bool tick( float dTime );
	bool isLodVisible( float lod ) const;
	void draw( Moo::DrawContext& drawContext, const Matrix & worldTransform, float lod );
	void localBoundingBox( BoundingBox & bb ) const;		
	void localVisibilityBoundingBox( BoundingBox & vbb ) const;
	void worldBoundingBox( BoundingBox & bb, const Matrix& world ) const;	
	void worldVisibilityBoundingBox( BoundingBox & vbb ) const;
	bool attach( MatrixLiaison * pOwnWorld );
	void detach();	
	void tossed( bool isOutside );
	void enterWorld();
	void leaveWorld();
	void move( float dTime );
	//@}

	void update( float dTime );


	/// @name Accessors to Particle System Properties.
	//@{
	int capacity( void ) const;
	void capacity( int number );

	int size( void ) const;

    float windFactor( void ) const;
    void windFactor( float ratio );

	bool enabled( void ) const;
    void enabled( bool state );

	ParticleSystemRendererPtr pRenderer( void ) const;
	void pRenderer( ParticleSystemRendererPtr pNewRenderer );

	Matrix worldTransform( void ) const;
	Matrix objectToWorld( void ) const;

	const BoundingBox & boundingBox() const;

	const BW::string& name( void ) const { return name_; }
	void name( const BW::StringRef & newName ) { name_.assign( newName.data(), newName.size() ); }

	bool isStatic( void ) const { return static_; }
	void isStatic( bool s );

	uint32 id( void ) const { return id_; }

	bool forcingSave( void ) const { return forcingSave_; }

	void addFlareID( uint32 id ) { flareIDs_.insert( id ); }
	void removeFlareID( uint32 id ) { flareIDs_.erase( id ); }

	static uint32 getUniqueParticleID( Particles::iterator p, const ParticleSystem &ps );

	/// force particle system to update next time it's ticked.
	void setDoUpdate()	{ doUpdate_ = true; }
	void setFirstUpdate()	{ firstUpdate_ = true; }

	size_t sizeInBytes() const;
	//@}


	/// @name General Operations on Particle Systems.
	//@{
	bool load( DataSectionPtr pDS,const BW::StringRef & name = BW::StringRef(),
				bool transient = false, StringBuilder* pError = NULL );
	bool load( const BW::StringRef & filename,
			const BW::StringRef & directory = "particles/",
			StringBuilder* pError = NULL );		

	bool save( const BW::StringRef & filename,
		const BW::StringRef & directory = "/particles/",
        bool transient = false );   // state saving in xml format
	bool save( DataSectionPtr pDS,
		const BW::StringRef & name = BW::StringRef(), bool transient = false );

	void addParticle( const Particle &particle, bool isMesh );	
	Particles::iterator removeParticle( Particles::iterator particle );

	void addAction( ParticleSystemActionPtr pAction );
    void insertAction( size_t idx, ParticleSystemActionPtr pAction );
	void removeAction( int actionTypeID );
	void removeAction( ParticleSystemActionPtr pAction );
	ParticleSystemActionPtr pAction( int actionTypeID );
    ParticleSystemActionPtr pAction( int actionTypeID,
		const BW::StringRef & str );
	Actions & actionSet() { return actions_; }

	void clear( void );

	RompColliderPtr groundSpecifier( void ) const;
	void groundSpecifier( RompColliderPtr pGS );

	void transformBoundingBox(const Matrix& trans)
	{
		if( !( boundingBox_ == BoundingBox::s_insideOut_ ) )
			boundingBox_.transformBy(trans);
	}
	//@}


	void spawn( int num = 1 );	// force all the sources to create their force particle set

	void explicitPosition( const Vector3& );
	const Vector3& explicitPosition() const;

	void explicitDirection( const Vector3& );
	const Vector3& explicitDirection() const;

	float fixedFrameRate() const	{ return fixedFrameRate_; }
	void fixedFrameRate( float f )	{ fixedFrameRate_ = f; }

	void localOffset( const Vector3 & offset ) { localOffset_ = offset; }
	Vector3 localOffset() const { return localOffset_; }

	void maxLod(float maxLod);
	float maxLod() const;

	// Gets the duration of ParticleSystem, -1 if it goes on forever.
	float duration() const;

	// Predict position of the particle.
	void predictPosition( const Particle & particle, float dt,
		Vector3 & retPos ) const;
	bool isLocal() const;   ///< Does the particle system use local space particles? 
protected:
    void updateBoundingBox();


private:

	template < typename Serialiser >
	void serialise( const Serialiser & ) const;

	// required for serialisation
	void explicitTransform( bool );
	bool explicitTransform() const;

	/// @name General Particle System Properties.
	//@{
	float windFactor_;	    ///< Effectiveness of wind against the particles.
	bool enabled_;			///< Whether or not the particle system should be drawn as a preview
	bool doUpdate_;			///< Should we execute update next tick.
	bool firstUpdate_;		///< Refresh the source incase undo/redo operation.
	ParticlesPtr particles_;///< The set of particles.
	Actions actions_;		///< The set of behaviours.
	RompColliderPtr pGS_;	///< Ground specifier for the Particle System.
	float	fixedFrameRate_;///< Can simulate a fixed frame rate for particle systems.
	float	framesLeftOver_;///< Used by the fixed frame rate stuff.	
	Vector3 windEffect_;	///< Cached per-frame wind vector to apply during movement.

	uint32	id_;			///< UniqueID of the particle systema
	bool forcingSave_;		///< So we do not remove flares from list of stored flares.

	BW::set<size_t> flareIDs_;	
	//@}

	/// Variables to make it look like this is a PyAttachment
	MatrixLiaison	* pOwnWorld_;
	bool			attached_;
	bool			inWorld_;


	/// The object that dictates the drawing and appearance of the particles.
	ParticleSystemRendererPtr pRenderer_;

	bool	explicitTransform_;
	Vector3	explicitPosition_;
	Vector3 explicitDirection_;

	Vector3 localOffset_;	// used by the meta particle system to position particle systems

	float	maxLod_;

	int counter_;
	static int s_counter_;
	BoundingBox	boundingBox_;
	BoundingBox vizBox_;
#ifdef EDITOR_ENABLED
	BoundingBox	originalVizBox_;
#endif//EDITOR_ENABLED

	BW::string name_;	

	bool loadInternal( DataSectionPtr pSect, bool transient,
						StringBuilder* pError = NULL );
	bool saveInternal( DataSectionPtr pSect, bool transient );

	void forceFullBoundingBoxCalculation();	//please only use during save
	void clearBoundingBox();

	bool static_;		// use to determine whether static (unmoving) in the world
						// in which case a different bounding box is used

	static uint32 	s_idCounter_;
};


typedef SmartPointer<ParticleSystem>	ParticleSystemPtr;


#ifdef CODE_INLINE
#include "particle_system.ipp"
#endif

BW_END_NAMESPACE

#endif // PARTICLE_SYSTEM_HPP

/* particle_system.hpp */
