#include "pch.hpp"
#include "phase_factory.hpp"


BW_BEGIN_NAMESPACE

BW_SINGLETON_STORAGE( PostProcessing::PhaseCreators );

namespace PostProcessing
{

void PhaseFactory::registerType( const BW::string& label, PhaseCreator creator )
{
	static PostProcessing::PhaseCreators s_creators;
	INFO_MSG( "Registering factory for %s\n", label.c_str() );
	PhaseCreators::instance().insert( std::make_pair(label, creator) );
}


/**
 *	This method loads the given section assuming it is a phase
 */
/*static*/Phase* PhaseFactory::loadItem( DataSectionPtr pSection )
{
	BW_GUARD;
	
	PhaseCreators::iterator found = PhaseCreators::instance().find( pSection->sectionName() );
	if (found != PhaseCreators::instance().end())
		return (*found->second)( pSection );

	return NULL;
}


Phase* PhaseFactory::create( DataSectionPtr pDS )
{
	return creator_(pDS);
}


}	//namespace PostProcessing

BW_END_NAMESPACE

// phase_factory.cpp
