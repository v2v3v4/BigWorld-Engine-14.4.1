#include "pch.hpp"
#include "gui_manager.hpp"
#include "gui_toolbar.hpp"
#include "gui_input_handler.hpp"
#include "resmgr/bwresource.hpp"
#include <stack>


BW_BEGIN_NAMESPACE
BW_INIT_SINGLETON_STORAGE( GUI::Manager )
BEGIN_GUI_NAMESPACE


//-----------------------------------------------------------------------------
// Section: GUI::Subscriber
//-----------------------------------------------------------------------------

Subscriber::Subscriber( const BW::string& root ) : root_( root )
{}

const BW::string& Subscriber::root() const
{
	return root_;
}

ItemPtr Subscriber::rootItem()
{
	return rootItem_;
}

void Subscriber::rootItem( ItemPtr item )
{
	rootItem_ = item;
}


//-----------------------------------------------------------------------------
// Section: GUI::Manager
//-----------------------------------------------------------------------------

Manager::Manager() : Item( "", "", "", "", "", "", "", "", "" )
{
	BW_GUARD;

	Item::staticInit();
	Toolbar::staticInit();
	functorManager_.registerFunctor( &cppFunctor_ );
	functorManager_.registerFunctor( &optionFunctor_ );
	functorManager_.registerFunctor( &watcherFunctor_ );
	functorManager_.registerFunctor( &pythonFunctor_ );
	functorManager_.registerFunctor( &dataSectionFunctor_ );
}


Manager::Manager( DataSectionPtr section ) : Item( section )
{
	BW_GUARD;

	functorManager_.registerFunctor( &cppFunctor_ );
	functorManager_.registerFunctor( &optionFunctor_ );
	functorManager_.registerFunctor( &watcherFunctor_ );
	functorManager_.registerFunctor( &pythonFunctor_ );
	functorManager_.registerFunctor( &dataSectionFunctor_ );
}


Manager::~Manager()
{
	BW_GUARD;

	functorManager_.unregisterFunctor( &cppFunctor_ );
	functorManager_.unregisterFunctor( &optionFunctor_ );
	functorManager_.unregisterFunctor( &watcherFunctor_ );
	functorManager_.unregisterFunctor( &pythonFunctor_ );
	functorManager_.unregisterFunctor( &dataSectionFunctor_ );
}


bool Manager::doInit()
{
	BW_GUARD;

	// This is a smartpointer class, so inc the ref while the singleton needs
	// to be alive.
	incRef();

	Win32InputDevice::install();

	return true;
}


bool Manager::doFini()
{
	BW_GUARD;

	Win32InputDevice::fini();

	// Decrement the reference of the singleton. This will destroy the instance
	// and will set pInstance to NULL in the base InitSingleton class, which is
	// valid (InitSingleton does a delete pInstance(), which is save with NULL.
	decRef();
	// At this stage, the pInstance must be NULL. Assert otherwise.
	MF_ASSERT( pInstance() == NULL );
	return true;
}


void Manager::add( SubscriberPtr subscriber )
{
	BW_GUARD;

	ItemPtr item = operator()( subscriber->root() );

	if (item)
	{
		subscribers_[ subscriber->root() ].insert( subscriber );
		subscriber->rootItem( item );
		subscriber->changed( item );
	}
}

void Manager::remove( SubscriberPtr subscriber )
{
	BW_GUARD;

	if( subscribers_.find( subscriber->root() ) != subscribers_.end() )
	{
		if( subscribers_[ subscriber->root() ].find( subscriber ) !=
			subscribers_[ subscriber->root() ].end() )
			subscribers_[ subscriber->root() ].erase(
				subscribers_[ subscriber->root() ].find( subscriber ) );
		if( subscribers_[ subscriber->root() ].empty() )
			subscribers_.erase( subscribers_.find( subscriber->root() ) );
	}
}

ItemPtr Manager::operator()( const BW::string& path )
{
	BW_GUARD;

	if( path.size() && path[ 0 ] == '/' )
		return Item::operator()( path.c_str() + 1 );
	return Item::operator()( path );
}

void Manager::act( unsigned short commandID )
{
	BW_GUARD;

	ItemPtr item = findByCommandID( commandID );
	if( item )
	{
		item->act();
		for( BW::map<BW::string,BW::set<SubscriberPtr> >::iterator iter = subscribers_.begin();
			iter != subscribers_.end(); ++iter )
		{
			for( BW::set<SubscriberPtr>::iterator siter = iter->second.begin();
				siter != iter->second.end(); ++siter )
				( *siter )->changed( ( *siter )->rootItem() );
		}
	}
}

void Manager::update( unsigned short commandID )
{
	BW_GUARD;

	ItemPtr item = findByCommandID( commandID );
	if( item )
	{
		for( BW::map<BW::string,BW::set<SubscriberPtr> >::iterator iter = subscribers_.begin();
			iter != subscribers_.end(); ++iter )
		{
			for( BW::set<SubscriberPtr>::iterator siter = iter->second.begin();
				siter != iter->second.end(); ++siter )
				( *siter )->changed( item );
		}
	}
}

void Manager::update()
{
	BW_GUARD;

	for( BW::map<BW::string,BW::set<SubscriberPtr> >::iterator iter = subscribers_.begin();
		iter != subscribers_.end(); ++iter )
	{
		for( BW::set<SubscriberPtr>::iterator siter = iter->second.begin();
			siter != iter->second.end(); ++siter )
			( *siter )->changed( ( *siter )->rootItem() );
	}
}

void Manager::changed( ItemPtr item )
{
	BW_GUARD;

	for( BW::map<BW::string,BW::set<SubscriberPtr> >::iterator iter = subscribers_.begin();
		iter != subscribers_.end(); ++iter )
	{
		for( BW::set<SubscriberPtr>::iterator siter = iter->second.begin();
			siter != iter->second.end(); ++siter )
			( *siter )->changed( item );
	}
}

bool Manager::resolveFileName( const BW::string& fileName )
{
	BW_GUARD;

	return BWResource::fileExists( fileName );
}


END_GUI_NAMESPACE
BW_END_NAMESPACE

