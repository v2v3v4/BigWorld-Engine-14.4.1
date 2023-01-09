#ifndef NODE_CLAMP_PSA_HPP
#define NODE_CLAMP_PSA_HPP

#include "particle_system_action.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This action clamps the particle to a node. The node clamp PSA effectively
 *	moves the particle by the same displacement done to the node in the same
 *	update.
 */
class NodeClampPSA : public ParticleSystemAction
{
public:
	///	@name Constructor(s) and Destructor.
	//@{
	NodeClampPSA();	
	//@}

    ParticleSystemActionPtr clone() const;

	///	@name Overrides to the Particle System Action Interface.
	//@{
	virtual void execute( ParticleSystem & particleSystem, float dTime );
	virtual void lateExecute( ParticleSystem &particleSystem, float dTime );
	virtual int typeID() const;
	virtual const BW::string & nameID() const;

	virtual size_t sizeInBytes() const { return sizeof(NodeClampPSA); }
	//@}

	///	 Accessors to NodeClampPSA properties.
	//@{
	bool fullyClamp() const;
	void fullyClamp( bool f );
	//@}

	static const BW::string nameID_;

protected:
	virtual void loadInternal( DataSectionPtr pSect );
	virtual void saveInternal( DataSectionPtr pSect ) const;

private:

	template < typename Serialiser >
	void serialise( const Serialiser & ) const;

	///	@name Auxiliary Variables for the NodeClampPSA.
	//@{
	static int typeID_;			///< TypeID of the NodeClampPSA.

	Vector3 lastPositionOfPS_;	///< Previous position of the particle system.
	bool firstUpdate_;			///< If true, need to ignore lastPositionOfPS.
	bool fullyClamp_;			///< If true( default ), does not retain relative positions.
	//@}
};

typedef SmartPointer<NodeClampPSA> NodeClampPSAPtr;


/*~ class Pixie.PyNodeClampPSA
 *
 *	PyNodeClampPSA is a PyParticleSystemAction that locks a particle to the Node
 *	the particle system is attached to. Any motion or change in position of the
 *	Node is also appied to the particles in the particle system.
 *
 *	A new PyNodeClampPSA is created using Pixie.NodeClampPSA function.
 */
class PyNodeClampPSA : public PyParticleSystemAction
{
	Py_Header( PyNodeClampPSA, PyParticleSystemAction )
public:
	PyNodeClampPSA( NodeClampPSAPtr pAction, PyTypeObject *pType = &s_type_ );

	int typeID( void ) const;

	///	 Accessors to PyNodeClampPSA properties.
	//@{
	bool fullyClamp() const;
	void fullyClamp( bool f );
	//@}

	///	@name Python Interface to the PyCollidePSA.
	//@{
	PY_FACTORY_DECLARE()

	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( bool, fullyClamp, fullyClamp )

	PY_RO_ATTRIBUTE_DECLARE( typeID(), typeID )
	//@}
private:
	NodeClampPSAPtr pAction_;
};

PY_SCRIPT_CONVERTERS_DECLARE( PyNodeClampPSA )


#ifdef CODE_INLINE
#include "node_clamp_psa.ipp"
#endif

BW_END_NAMESPACE

#endif
/* node_clamp_psa.hpp */
