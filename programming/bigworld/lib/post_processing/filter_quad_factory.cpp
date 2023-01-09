#include "pch.hpp"
#include "filter_quad_factory.hpp"


BW_BEGIN_NAMESPACE

BW_SINGLETON_STORAGE( PostProcessing::FilterQuadCreators );

namespace PostProcessing
{

void FilterQuadFactory::registerType( const BW::string& label, FilterQuadCreator creator )
{
	static PostProcessing::FilterQuadCreators s_creators;
	INFO_MSG( "Registering factory for %s\n", label.c_str() );
	FilterQuadCreators::instance().insert( std::make_pair(label, creator) );
}


/**
 *	This method loads the given section assuming it is a filter quad
 */
/*static*/FilterQuad* FilterQuadFactory::loadItem( DataSectionPtr pSection )
{
	BW_GUARD;

	FilterQuadCreators::iterator found = FilterQuadCreators::instance().find( pSection->sectionName() );
	if (found != FilterQuadCreators::instance().end())
		return (*found->second)( pSection );

	return NULL;
}


FilterQuad* FilterQuadFactory::create( DataSectionPtr pDS )
{
	return creator_(pDS);
}


}	//namespace PostProcessing

BW_END_NAMESPACE

// filter_quad_factory.cpp
