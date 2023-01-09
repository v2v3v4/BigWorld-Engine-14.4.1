#ifndef META_PARTICLE_SYSTEM_HPP
#define META_PARTICLE_SYSTEM_HPP

// Standard MF Library Headers.
#include "particle_system.hpp"
#include "cstdmf/smartpointer.hpp"
#include "particle.hpp"
#include "math/boundbox.hpp"

#ifdef EDITOR_ENABLED
#include "gizmo/meta_data.hpp"
#endif//EDITOR_ENABLED

#include "model/forward_declarations.hpp"

BW_BEGIN_NAMESPACE

class MetaParticleSystem;
typedef SmartPointer<MetaParticleSystem> MetaParticleSystemPtr;


/**
 *	This class is a container of a number of ParticleSystems
 *
 *	The MetaParticleSystem can contain several ParticleSystem objects, 
 *	allowing them to function as a single object.
 */
class MetaParticleSystem : public ReferenceCount
{
public:
	/// A collection of ParticleSystem
	typedef BW::vector<ParticleSystemPtr> ParticleSystems;

	/// @name Constructor(s) and Destructor.
	//@{
	MetaParticleSystem();
	~MetaParticleSystem();
	//@}

    // Create a deep copy (the particle systems are also cloned) of the 
    // metaparticle system.
    MetaParticleSystemPtr clone() const;	

	// Return the resources required for a MetaParticleSystem
	static void prerequisites( DataSectionPtr pSection, BW::set<BW::string>& output );
	static bool isParticleSystem( const BW::string& file );

	/// @name General Operations on Particle Systems.
	//@{
	bool tick( float dTime );
	bool isLodVisible( float lod ) const;
	void draw( Moo::DrawContext& drawContext, const Matrix & worldTransform, float lod );
	void localBoundingBox( BoundingBox & bb ) const;
	void localVisibilityBoundingBox( BoundingBox & bb ) const;
	void worldBoundingBox( BoundingBox & bb, const Matrix & world ) const;
	void worldVisibilityBoundingBox( BoundingBox & bb ) const;
	bool attach( MatrixLiaison * pOwnWorld );
	void detach();	
	void tossed( bool isOutside );
	void enterWorld();
	void leaveWorld();
	void move( float dTime );

	const Matrix & worldTransform( void );

	bool load( DataSectionPtr pDS, const BW::StringRef & filename, 
				StringBuilder* pError = NULL );
	bool load( const BW::StringRef & filename, const BW::StringRef & directory,
				StringBuilder* pError = NULL );

	void save( const BW::StringRef & filename, const BW::StringRef & directory,
		bool transient = false );
	void reSerialise( DataSectionPtr sect, bool load = true, 
		bool transient = false );

	void addSystem( const ParticleSystemPtr & newSystem );
    void insertSystem( size_t idx, const ParticleSystemPtr & newSystem );
	void removeSystem( const ParticleSystemPtr & system );
	void removeAllSystems();

	ParticleSystemPtr system( const BW::StringRef & name );
	ParticleSystemPtr systemFromIndex( size_t index );
	ParticleSystemPtr getFirstSystem() { return (systems_.at(0)); }
	ParticleSystems & systemSet() { return systems_; }

	int size( void );
	int capacity( void );

	void spawn( int num=1 );	// force all the sources to create their force particle set
	void clear();				// delegate clear to all particle systems

	void isStatic( bool s );
	void setDoUpdate();
	void setFirstUpdate();

	size_t sizeInBytes() const;
	//@}


	/// @name for the editor
	//@{
	void transformBoundingBox( const Matrix & trans );
	//@}

	// Gets the largest duration of a MetaParticleSystem -1 
	// if it goes on forever.
	float	duration() const;

	// Returns the number of particlesystems in the metaparticlesystem
	uint32 nSystems() const;

#ifdef EDITOR_ENABLED
	MetaData::MetaData&				metaData()			{	return metaData_;	}
	const MetaData::MetaData&		metaData()	const	{	return metaData_;	}
	bool							populateMetaData( DataSectionPtr ds );
#endif//EDITOR_ENABLED

	void helperModelName( const BW::string &name ) { helperModelName_ = name; }
	const BW::string &helperModelName() const { return helperModelName_; }
	void helperModelCenterOnHardPoint(uint idx) { helperModelCenterOnHardPoint_ = idx; }
	uint helperModelCenterOnHardPoint() const { return helperModelCenterOnHardPoint_; }

	static void getHardPointTransforms( ModelPtr model, BW::vector<BW::string> &helperModelHardPointNames, BW::vector< Matrix > &helperModelHardPointTransforms );

private:
	static void collectVisualNodes( const Moo::NodePtr & node,
		BW::vector< Moo::NodePtr > & nodes );

	ParticleSystems		systems_;

	/// Variables to make it look like this is a PyAttachment.  It is
	/// only used when we are wrapped by PyMetaParticleSystem
	MatrixLiaison	* pOwnWorld_;
	bool			attached_;
	bool			inWorld_;

#ifdef EDITOR_ENABLED
	MetaData::MetaData		metaData_;
#endif//EDITOR_ENABLED

	BW::string					helperModelName_;
	uint						helperModelCenterOnHardPoint_;
};

typedef SmartPointer<MetaParticleSystem>	MetaParticleSystemPtr;

/// Class which keeps a cache of loaded particle systems
class MetaParticleSystemCache
{
public:

	MetaParticleSystemCache();

	/// Load a MetaParticleSystem. If it is already loaded a cloned copy will
	/// be returned.
	MetaParticleSystemPtr load( const StringRef & filename );

	/// Clear all cached MetaParticleSystems
	void clear();

	// shutdown cache system, no further loads will be cached
	void shutdown();

private:

	MetaParticleSystemPtr loadInternal( const StringRef & filename ) const;

	typedef BW::StringRefUnorderedMap< MetaParticleSystemPtr > Cache;
	Cache cache_;
	SimpleMutex mutex_;
	size_t cacheHit_;
	size_t cacheMiss_;
	bool enabled_;
};

BW_END_NAMESPACE

#endif // META_PARTICLE_SYSTEM_HPP
/* meta_particle_system.hpp */
