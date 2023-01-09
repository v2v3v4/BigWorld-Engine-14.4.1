#ifndef MATRIX_SWARM_PSA_HPP
#define MATRIX_SWARM_PSA_HPP

#include "particle_system_action.hpp"


BW_BEGIN_NAMESPACE

typedef BW::vector< MatrixProviderPtr > Matrices;

/**
 *	This class causes particles to "swarm" around a list of targets. This
 *	ParticleSystemAction causes the particle to have is position transformed
 *	by the matrix of one of the targets rather than the source of the
 *	particle system. The particles are divided evenly among the targets.
 */
class MatrixSwarmPSA : public ParticleSystemAction
{
public:
	MatrixSwarmPSA();
	~MatrixSwarmPSA();

    ParticleSystemActionPtr clone() const;

	///	 Accessors to MatrixSwarmPSA properties.
	//@{
	Matrices & targets( void );
	//@}

	///	@name Overrides to the Particle System Action Interface.
	//@{
	virtual void execute( ParticleSystem & particleSystem, float dTime );
	virtual void lateExecute( ParticleSystem & particleSystem, float dTime );
	virtual int typeID() const;
	virtual const BW::string & nameID() const;

	virtual size_t sizeInBytes() const { return sizeof(MatrixSwarmPSA) + sizeof(BW::vector< MatrixProvider * >) * targets_.capacity() + sizeof(PySTLSequenceHolder<Matrices>); }
	//@}

	static const BW::string nameID_;

protected:
	virtual void loadInternal( DataSectionPtr pSect );
	virtual void saveInternal( DataSectionPtr pSect ) const;

private:
	///	@name Auxiliary Variables for the MatrixSwarmPSA.
	//@{
	static int typeID_;			///< TypeID of the MatrixSwarmPSA.
	Matrices						targets_;
	//@}
};

typedef SmartPointer<MatrixSwarmPSA> MatrixSwarmPSAPtr;


/*~ class Pixie.PyMatrixSwarmPSA
 *
 *	PyMatrixSwarmPSA causes particles to "swarm" around a list of targets. This
 *	PyParticleSystemAction causes the particle to have is position transformed
 *	by the matrix of one of the targets rather than the source of the
 *	particle system. The particles are divided evenly among the targets.
 *
 *	A new PyMatrixSwarmPSA is created using Pixie.MatrixSwarmPSA function.
 */
class PyMatrixSwarmPSA : public PyParticleSystemAction
{
	Py_Header( PyMatrixSwarmPSA, PyParticleSystemAction )
public:
	PyMatrixSwarmPSA( MatrixSwarmPSAPtr pAction, PyTypeObject *pType = &s_type_ );

	int typeID( void ) const;

	///	@name Python Interface to the PyMatrixSwarmPSA.
	//@{
	PY_FACTORY_DECLARE()

	PY_RW_ATTRIBUTE_DECLARE( targetHolders_, targets )
	//@}
private:
	MatrixSwarmPSAPtr pAction_;

	PySTLSequenceHolder<Matrices> targetHolders_;
};

PY_SCRIPT_CONVERTERS_DECLARE( PyMatrixSwarmPSA )


void MatrixProviderPtr_release( MatrixProvider *&pCamera );
/*
 *	Wrapper to let the C++ vector of matrices work as a vector/list in Python.
 */
namespace PySTLObjectAid
{
	/// Releaser is same for all PyObject's
	template <> struct Releaser< MatrixProvider * >
	{
		static void release( MatrixProvider *&pMatrix )
        {
        	MatrixProviderPtr_release( pMatrix );
        }
	};
}


#ifdef CODE_INLINE
#include "matrix_swarm_psa.ipp"
#endif

BW_END_NAMESPACE

#endif
/* matrix_swarm_psa.hpp */
