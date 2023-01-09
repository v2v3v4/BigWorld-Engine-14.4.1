#ifndef GUI_FUNCTOR_HPP__
#define GUI_FUNCTOR_HPP__

#include "gui_forward.hpp"
#include "gui_item.hpp"
#include "resmgr/datasection.hpp"
#include "cstdmf/bw_map.hpp"
#include "cstdmf/bw_string.hpp"

BW_BEGIN_NAMESPACE
BEGIN_GUI_NAMESPACE

class Functor
{
public:
	virtual ~Functor();
	virtual const BW::string& name() const = 0;
	virtual bool text( const BW::string& textor, ItemPtr item, BW::string& result ) = 0;
	virtual bool update( const BW::string& updater, ItemPtr item, unsigned int& result ) = 0;
	virtual DataSectionPtr import( const BW::string& importer, ItemPtr item ) { return NULL; }
	virtual bool act( const BW::string& action, ItemPtr item, bool& result ) = 0;
};

class FunctorManager
{
	BW::map< BW::string, Functor* >& functors();
public:
	FunctorManager();
	~FunctorManager();
	BW::string text( const BW::string& textor, ItemPtr item );
	unsigned int update( const BW::string& updater, ItemPtr item );
	DataSectionPtr import( const BW::string& importer, ItemPtr item );
	bool act( const BW::string& action, ItemPtr item );

	void registerFunctor( Functor* functor );
	void unregisterFunctor( Functor* functor );

	static void trim( BW::string& str );
};

END_GUI_NAMESPACE
BW_END_NAMESPACE

#endif//GUI_FUNCTOR_HPP__
