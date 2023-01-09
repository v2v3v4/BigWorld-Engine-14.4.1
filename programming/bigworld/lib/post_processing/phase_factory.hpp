#ifndef PHASE_FACTORY_HPP
#define PHASE_FACTORY_HPP

#include "cstdmf/singleton.hpp"
#include "resmgr/datasection.hpp"
#include "phase.hpp"


BW_BEGIN_NAMESPACE

namespace PostProcessing
{

/**
 *	This macro is used to declare a class as a phase. It is used to
 *	implement the factory functionality. It should appear in the declaration of
 *	the class.
 *
 *	Classes using this macro should implement the load method and also use
 *	the IMPLEMENT_PHASE macro.
 */
#define DECLARE_PHASE( CLASS )															\
	static Phase* create( DataSectionPtr pSection );									\
	static PhaseRegistrar s_registrar;



/**
 *	This macro is used to implement some of the phase functionality of a
 *	class that has used the DECLARE_PHASE macro.
 */
#define IMPLEMENT_PHASE( CLASS, LABEL )													\
																						\
	Phase* CLASS::create( DataSectionPtr pDS )											\
	{																					\
		CLASS * pPhase = new CLASS();													\
																						\
		if (pPhase->load(pDS))															\
		{																				\
			return pPhase;																\
		}																				\
																						\
		bw_safe_delete(pPhase);															\
		return NULL;																	\
	};																					\
																						\
	PhaseRegistrar CLASS::s_registrar( #CLASS, CLASS::create );


typedef Phase* (*PhaseCreator)( DataSectionPtr pSection );


class PhaseCreators : public BW::map<BW::string,PhaseCreator>, public Singleton<PhaseCreators>
{
};


/**
 *	This class implements a single phase factory and static storage of all
 *	phase factories.
 */
class PhaseFactory
{
public:
	static void registerType( const BW::string& label, PhaseCreator creator = NULL );
	Phase* create( DataSectionPtr );	
	PhaseCreator creator_;

	static Phase* loadItem( DataSectionPtr );
};


class PhaseRegistrar
{
public:
	PhaseRegistrar( const BW::string& label, PhaseCreator c )
	{
		PhaseFactory::registerType( label, c );
	};
};


}	//namespace PostProcessing

BW_END_NAMESPACE

#endif	//PHASE_FACTORY_HPP
