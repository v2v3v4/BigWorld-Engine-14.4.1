#ifndef GUI_ITEM_HPP__
#define GUI_ITEM_HPP__


#include "gui_forward.hpp"
#include "cstdmf/smartpointer.hpp"
#include "cstdmf/guard.hpp"
#include "resmgr/datasection.hpp"
#include "cstdmf/bw_string.hpp"
#include "cstdmf/bw_map.hpp"
#include "cstdmf/bw_vector.hpp"
#include "cstdmf/bw_set.hpp"

BW_BEGIN_NAMESPACE
BEGIN_GUI_NAMESPACE


class InputDevice
{
public:
	virtual ~InputDevice();
	virtual bool isKeyDown( const BW::string& key ) = 0;
};


// the framework uses a java style view so not eveything is necessarily consted
class Item : public SafeReferenceCount
{
	BW::string type_;
	BW::string name_;
	BW::string displayName_;
	BW::string description_;
	BW::string icon_;
	BW::string shortcutKey_;
	BW::string action_;
	BW::string updater_;
	BW::string imports_;
	BW::map<BW::string,BW::string> values_;
	BW::vector<Item*> parents_;// no smarter pointer here, remove cycle
	BW::vector<ItemPtr> subitems_;
	unsigned short commandID_;
	static BW::set<unsigned short>& unusedCommands();
	static BW::map<BW::string, ItemTypePtr>& types();
public:
	Item( const BW::string& type, const BW::string& name, const BW::string& displayName,
		const BW::string& description,	const BW::string& icon, const BW::string& shortcutKey,
		const BW::string& action, const BW::string& updater, const BW::string& imports,
		unsigned int commandID = GUI_COMMAND_END );
	Item( DataSectionPtr section );
	virtual ~Item();
	void add( ItemPtr item );
	void add( const DataSectionPtr section );
	void insert( BW::vector<ItemPtr>::size_type index, ItemPtr item );
	void remove( BW::vector<ItemPtr>::size_type index );
	void remove( ItemPtr item );
	void remove( const BW::string& name );

	BW::vector<ItemPtr>::size_type num() const;
	ItemPtr operator[]( BW::vector<ItemPtr>::size_type index );
	const ItemPtr operator[]( BW::vector<ItemPtr>::size_type index ) const;

	BW::vector<Item*>::size_type parentNum();
	Item* parent( BW::vector<Item*>::size_type index );
	void addParent( Item* parent );
	bool isAncestor( Item* item ) const;
	BW::string pathTo( Item* item );

	const BW::string& type() const;
	const BW::string& name() const;
	BW::string displayName();
	BW::string description();
	const BW::string& action() const;
	const BW::string& updater() const;
	const BW::string& shortcutKey() const;
	unsigned short commandID() const;

	bool exist( const BW::string& name );
	BW::string operator[]( const BW::string& name );
	void set( const BW::string& name, const BW::string& value );

	unsigned int update();
	bool act();
	bool processInput( InputDevice* inputDevice );
	void changed();

	virtual ItemPtr operator()( const BW::string& path );

	ItemPtr findByCommandID( unsigned short commandID );
	ItemPtr findByName( const BW::string& name );

	static void registerType( ItemTypePtr itemType );
	static void unregisterType( ItemTypePtr itemType );

	static void staticInit();
};

class ItemType : public SafeReferenceCount
{
public:
	virtual ~ItemType();
	virtual const BW::string& name() const = 0;
	virtual bool act( ItemPtr item ) = 0;
	virtual unsigned int update( ItemPtr item ) = 0;
	virtual void shortcutPressed( ItemPtr item ) = 0;
};

class BasicItemType : public ItemType
{
	BW::string name_;
public:
	BasicItemType( const BW::string& typeName );
	virtual ~BasicItemType();
	virtual const BW::string& name() const;
	virtual bool act( ItemPtr item );
	virtual unsigned int update( ItemPtr item );
	virtual void shortcutPressed( ItemPtr item );
};


END_GUI_NAMESPACE
BW_END_NAMESPACE


#endif // GUI_ITEM_HPP__
