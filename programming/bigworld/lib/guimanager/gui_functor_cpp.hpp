#ifndef GUI_FUNCTOR_CPP_HPP__
#define GUI_FUNCTOR_CPP_HPP__

#include "gui_functor.hpp"

BW_BEGIN_NAMESPACE
BEGIN_GUI_NAMESPACE

static const char GUI_DELIMITER = '|';

struct Textor
{
	virtual BW::string text( ItemPtr item ) = 0;
};

struct Updater
{
	virtual unsigned int update( ItemPtr item ) = 0;
};

struct Importer
{
	virtual DataSectionPtr import( ItemPtr item ) = 0;
};

struct Action
{
	virtual bool act( ItemPtr item ) = 0;
};

class CppFunctor : public Functor
{
	BW::map< BW::string, Textor* > textors_;
	BW::map< BW::string, Updater* > updaters_;
	BW::map< BW::string, Importer* > importers_;
	BW::map< BW::string, Action* > actions_;
public:
	void set( const BW::string& name, Textor* textor );
	void set( const BW::string& name, Updater* updater );
	void set( const BW::string& name, Importer* importer );
	void set( const BW::string& name, Action* action );
	void remove( Textor* textor );
	void remove( Updater* updater );
	void remove( Importer* importer );
	void remove( Action* action );

	virtual const BW::string& name() const;
	virtual bool text( const BW::string& textor, ItemPtr item, BW::string& result );
	virtual bool update( const BW::string& updater, ItemPtr item, unsigned int& result );
	virtual DataSectionPtr import( const BW::string& importer, ItemPtr item );
	virtual bool act( const BW::string& action, ItemPtr item, bool& result );
};

END_GUI_NAMESPACE
BW_END_NAMESPACE

#endif//GUI_FUNCTOR_CPP_HPP__
