#ifndef GUI_MANAGER_HPP__
#define GUI_MANAGER_HPP__

#include "cstdmf/bw_string.hpp"
#include "cstdmf/bw_map.hpp"
#include "cstdmf/bw_set.hpp"
#include "cstdmf/init_singleton.hpp"
#include "cstdmf/guard.hpp"
#include "resmgr/datasection.hpp"
#include "gui_forward.hpp"
#include "gui_item.hpp"
#include "gui_functor.hpp"
#include "gui_functor_cpp.hpp"
#include "gui_functor_option.hpp"
#include "gui_functor_watcher.hpp"
#include "gui_functor_python.hpp"
#include "gui_functor_datasection.hpp"
#include "gui_bitmap.hpp"


BW_BEGIN_NAMESPACE
BEGIN_GUI_NAMESPACE

class Subscriber : public SafeReferenceCount
{
	BW::string root_;
	ItemPtr rootItem_;
public:
	Subscriber( const BW::string& root );
	virtual ~Subscriber(){}
	const BW::string& root() const;
	ItemPtr rootItem();
	void rootItem( ItemPtr item );
	virtual void changed( ItemPtr item ) = 0;
};

typedef SmartPointer<Subscriber> SubscriberPtr;


class Manager : public InitSingleton<Manager>, public Item
{
	BW::map<BW::string,BW::set<SubscriberPtr> > subscribers_;
public:
	Manager();
	virtual ~Manager();

	// Methods from InitSingleton
	/*virtual*/ bool doInit();
	/*virtual*/ bool doFini();

	using Item::add;
	void add( SubscriberPtr subscriber );
	void remove( SubscriberPtr subscriber );
	virtual ItemPtr operator()( const BW::string& path );

	void act( unsigned short commandID );
	void update( unsigned short commandID );
	void update();
	void changed( ItemPtr item );

	static bool resolveFileName( const BW::string& fileName );

	FunctorManager& functors() { return functorManager_; }

	CppFunctor& cppFunctor() { return cppFunctor_; }
	OptionFunctor& optionFunctor() { return optionFunctor_; }
	PythonFunctor& pythonFunctor() { return pythonFunctor_; }
	DataSectionFunctor& dataSectionFunctor() { return dataSectionFunctor_; }

	BitmapManager& bitmaps() { return bitmapManager_; }

private:
	Manager( const DataSectionPtr section );

	FunctorManager functorManager_;

	CppFunctor cppFunctor_;
	OptionFunctor optionFunctor_;
	WatcherFunctor watcherFunctor_;
	PythonFunctor pythonFunctor_;
	DataSectionFunctor dataSectionFunctor_;

	BitmapManager bitmapManager_;
};

END_GUI_NAMESPACE
BW_END_NAMESPACE

#endif//GUI_MANAGER_HPP__
