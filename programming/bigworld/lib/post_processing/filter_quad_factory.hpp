#ifndef FILTER_QUAD_FACTORY_HPP
#define FILTER_QUAD_FACTORY_HPP

#include "cstdmf/singleton.hpp"
#include "resmgr/datasection.hpp"
#include "filter_quad.hpp"


BW_BEGIN_NAMESPACE

namespace PostProcessing
{

/**
 *	This macro is used to declare a class as a filter quad. It is used to
 *	implement the factory functionality. It should appear in the declaration of
 *	the class.
 *
 *	Classes using this macro should implement the load method and also use
 *	the IMPLEMENT_FILTER_QUAD macro.
 */
#define DECLARE_FILTER_QUAD_COMMON( CLASS )									\
	static FilterQuad* create( DataSectionPtr pSection );					\
	static FilterQuadRegistrar s_registrar;

#ifndef EDITOR_ENABLED

	#define DECLARE_FILTER_QUAD( CLASS ) DECLARE_FILTER_QUAD_COMMON( CLASS )

#else // EDITOR_ENABLED

	#define DECLARE_FILTER_QUAD( CLASS )									\
		DECLARE_FILTER_QUAD_COMMON( CLASS )									\
		public:																\
		const char * creatorName() const { return #CLASS; }

#endif // EDITOR_ENABLED


/**
 *	This macro is used to implement some of the filter quad
 *  functionality of a class that has used the DECLARE_FILTER_QUAD macro.
 */
#define IMPLEMENT_FILTER_QUAD( CLASS, LABEL )											\
																						\
	FilterQuad* CLASS::create( DataSectionPtr pDS )										\
	{																					\
		CLASS * pFilterQuad = new CLASS();												\
																						\
		if (pFilterQuad->load(pDS))														\
		{																				\
			return pFilterQuad;															\
		}																				\
																						\
		bw_safe_delete(pFilterQuad);													\
		return NULL;																	\
	};																					\
																						\
	FilterQuadRegistrar CLASS::s_registrar( #CLASS, CLASS::create );


typedef FilterQuad* (*FilterQuadCreator)( DataSectionPtr pSection );


class FilterQuadCreators : public BW::map<BW::string,FilterQuadCreator>, public Singleton<FilterQuadCreators>
{
};


/**
 *	This class implements a single filter quad factory and static storage of all
 *	filter quad factories.
 */
class FilterQuadFactory
{
public:
	static void registerType( const BW::string& label, FilterQuadCreator creator = NULL );
	FilterQuad* create( DataSectionPtr );	
	FilterQuadCreator creator_;

	static FilterQuad* loadItem( DataSectionPtr );
};


class FilterQuadRegistrar
{
public:
	FilterQuadRegistrar( const BW::string& label, FilterQuadCreator c )
	{
		FilterQuadFactory::registerType( label, c );
	};
};


}	//namespace PostProcessing

BW_END_NAMESPACE

#endif	//FILTER_QUAD_FACTORY_HPP
