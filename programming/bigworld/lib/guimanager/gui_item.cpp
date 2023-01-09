#include "pch.hpp"
#include "gui_item.hpp"
#include "gui_manager.hpp"


BW_BEGIN_NAMESPACE
BEGIN_GUI_NAMESPACE


//-----------------------------------------------------------------------------
// Section: GUI::InputDevice
//-----------------------------------------------------------------------------

InputDevice::~InputDevice(){}


//-----------------------------------------------------------------------------
// Section: GUI::Item
//-----------------------------------------------------------------------------

BW::set<unsigned short>& Item::unusedCommands()
{
	BW_GUARD;

	static BW::set<unsigned short> unusedCommands;
	static bool firstTime = true;
	if (firstTime)
	{
		firstTime = false;
		for (unsigned short i = GUI_COMMAND_START; i < GUI_COMMAND_END; ++i)
		{
			unusedCommands.insert( i );
		}
	}
	return unusedCommands;
}

BW::map<BW::string, ItemTypePtr>& Item::types()
{
	static BW::map<BW::string, ItemTypePtr> types;
	return types;
}

Item::Item( const BW::string& type, const BW::string& name, const BW::string& displayName,
	const BW::string& description,	const BW::string& icon, const BW::string& shortcutKey,
	const BW::string& action, const BW::string& updater, const BW::string& imports, unsigned int commandID )
	:type_( type ), name_( name ), displayName_( displayName ), description_( description ),
	icon_( icon ), shortcutKey_( shortcutKey ), action_( action ), updater_( updater ),
	imports_( imports ), commandID_( commandID )
{
	BW_GUARD;

	if( commandID_ == GUI_COMMAND_END )
	{
		if( unusedCommands().empty() )
			throw 1;
		commandID_ = *unusedCommands().begin();
	}
	if( unusedCommands().find( commandID_ ) != unusedCommands().end() )
		unusedCommands().erase( unusedCommands().find( commandID_ ) );
}

Item::Item( DataSectionPtr section )
{
	BW_GUARD;

	if( !section )
		throw 1;
	type_ = section->readString( "type" );
	name_ = section->readString( "name" );
	displayName_ = section->readString( "displayName" );
	description_ = section->readString( "description" );
	icon_ = section->readString( "icon" );
	shortcutKey_ = section->readString( "shortcut" );
	action_ = section->readString( "action" );
	updater_ = section->readString( "updater" );
	commandID_ = (unsigned short)section->readInt( "commandID", GUI_COMMAND_END );
	if( commandID_ == GUI_COMMAND_END )
	{
		if( unusedCommands().empty() )
			throw 1;
		commandID_ = *unusedCommands().begin();
	}
	if( unusedCommands().find( commandID_ ) != unusedCommands().end() )
		unusedCommands().erase( unusedCommands().find( commandID_ ) );

	for( int i = 0; i < section->countChildren(); ++i )
	{
		values_[ section->childSectionName( i ) ] =
			section->readString( section->childSectionName( i ) );
	}

	BW::vector<DataSectionPtr> items;
	section->openSections( "item", items );
	for( BW::vector<DataSectionPtr>::iterator iter = items.begin(); iter != items.end(); ++iter )
		add( new Item( *iter ) );
	items.clear();
	section->openSections( "import", items );
	if( ! items.empty() )
		throw 1;// TODO: implement import
}

Item::~Item()
{
	BW_GUARD;

	if( commandID_ >= GUI_COMMAND_START && commandID_ < GUI_COMMAND_END )
		unusedCommands().insert( commandID_ );
}

void Item::add( ItemPtr item )
{
	BW_GUARD;

	if( isAncestor( item.getObject() ) )
		throw 1;
	subitems_.push_back( item );
	item->addParent( this );
}

void Item::add( const DataSectionPtr section )
{
	BW_GUARD;

	add( new Item( section ) );
}

void Item::insert( BW::vector<ItemPtr>::size_type index, ItemPtr item )
{
	BW_GUARD;

	if( index >= num() )
		index = num();
	BW::vector<ItemPtr>::iterator iter = subitems_.begin();
	std::advance( iter, index );
	subitems_.insert( iter, item );
}

void Item::remove( BW::vector<ItemPtr>::size_type index )
{
	BW_GUARD;

	if( index < num() )
	{
		BW::vector<ItemPtr>::iterator iter = subitems_.begin();
		std::advance( iter, index );
		subitems_.erase( iter );
	}
}

void Item::remove( ItemPtr item )
{
	BW_GUARD;

	BW::vector<ItemPtr>::iterator iter = std::remove(
		subitems_.begin(), subitems_.end(), item );
	subitems_.erase( iter, subitems_.end() );
}

void Item::remove( const BW::string& name )
{
	BW_GUARD;

	for (BW::vector<ItemPtr>::iterator iter = subitems_.begin();
		iter != subitems_.end();)
	{
		if ((*iter)->name() == name)
		{
			iter = subitems_.erase( iter );
		}
		else
		{
			++iter;
		}
	}
}

BW::vector<ItemPtr>::size_type Item::num() const
{
	return subitems_.size();
}

ItemPtr Item::operator[]( BW::vector<ItemPtr>::size_type index )
{
	BW_GUARD;

	return subitems_.at( index );
}

const ItemPtr Item::operator[]( BW::vector<ItemPtr>::size_type index ) const
{
	BW_GUARD;

	return subitems_.at( index );
}

BW::vector<Item*>::size_type Item::parentNum()
{
	return parents_.size();
}

Item* Item::parent( BW::vector<Item*>::size_type index )
{
	BW_GUARD;

	return parents_.at( index );
}

void Item::addParent( Item* parent )
{
	BW_GUARD;

	parents_.push_back( parent );
}

bool Item::isAncestor( Item* item ) const
{
	BW_GUARD;

	for( BW::vector<Item*>::const_iterator iter = parents_.begin();
		iter != parents_.end(); ++iter )
	{
		if( *iter == item || ( *iter )->isAncestor( item ) )
			return true;
	}
	return false;
}

BW::string Item::pathTo( Item* item )
{
	BW_GUARD;

	if( item == this )
		return "";
	for( BW::vector<Item*>::const_iterator iter = parents_.begin();
		iter != parents_.end(); ++iter )
	{
		if( item == NULL || ( *iter )->isAncestor( item ) )
			return ( *iter )->pathTo( item ) + "/" + name();
	}
	throw 1;
	return "";
}

const BW::string& Item::type() const
{
	return type_;
}

const BW::string& Item::name() const
{
	return name_;
}

BW::string Item::displayName()
{
	BW_GUARD;

	return displayName_.empty() ? name_ : Manager::instance().functors().text( displayName_, this );
}

BW::string Item::description()
{
	BW_GUARD;

	return description_.empty() ? displayName() : Manager::instance().functors().text( description_, this );
}

const BW::string& Item::action() const
{
	return action_;
}

const BW::string& Item::updater() const
{
	return updater_;
}

const BW::string& Item::shortcutKey() const
{
	return shortcutKey_;
}

unsigned short Item::commandID() const
{
	return commandID_;
}

bool Item::exist( const BW::string& name )
{
	BW_GUARD;

	return values_.find( name ) != values_.end();
}

BW::string Item::operator[]( const BW::string& name )
{
	BW_GUARD;

	if( values_.find( name ) != values_.end() )
		return values_[ name ];
	return "";
}

void Item::set( const BW::string& name, const BW::string& value )
{
	BW_GUARD;

	if( !value.empty() )
		values_[ name ] = value;
	else if( values_.find( name ) != values_.end() )
		values_.erase( values_.find( name ) );
}

unsigned int Item::update()
{
	BW_GUARD;

	if( types().find( type() ) != types().end() )
		return types()[ type() ]->update( this );
	return 1;// otherwise it is disabled
}

bool Item::processInput( InputDevice* inputDevice )
{
	BW_GUARD;

	if( inputDevice->isKeyDown( shortcutKey() ) )
	{
		if( types().find( type() ) != types().end() )
			types()[ type() ]->shortcutPressed( this );
		return true;
	}
	for( unsigned int i = 0; i < num(); ++i )
		if( operator[]( i )->processInput( inputDevice ) )
		{
			changed();
			return true;
		}
	return false;
}

bool Item::act()
{
	BW_GUARD;

	if( types().find( type() ) != types().end() )
		return types()[ type() ]->act( this );
	return false;
}

void Item::changed()
{
	BW_GUARD;

	Manager::instance().changed( this );
}

ItemPtr Item::operator()( const BW::string& path )
{
	BW_GUARD;

	BW::string current, rest;
	if( path.find( '/' ) != path.npos )
	{
		current = path.substr( 0, path.find( '/' ) );
		rest = path.substr( path.find( '/' ) + 1 );
	}
	else
		current = path;

	for( BW::vector<ItemPtr>::iterator iter = subitems_.begin();
		iter != subitems_.end(); ++iter )
	{
		if( (*iter)->name() == current )
			return rest.empty() ? (*iter) : (*iter)->operator()( rest );
	}
	return NULL;
}

ItemPtr Item::findByCommandID( unsigned short commandID )
{
	BW_GUARD;

	if( commandID == commandID_ )
		return this;
	for( unsigned int i = 0; i < num(); ++i )
	{
		ItemPtr result = ( *this )[ i ]->findByCommandID( commandID );
		if( result )
			return result;
	}
	return NULL;
}

ItemPtr Item::findByName( const BW::string& name )
{
	BW_GUARD;

	if( name == name_ )
		return this;

	for( unsigned int i = 0; i < num(); ++i )
	{
		ItemPtr result = ( *this )[ i ]->findByName( name );
		if( result )
			return result;
	}
	return NULL;
}

void Item::registerType( ItemTypePtr itemType )
{
	BW_GUARD;

	types()[ itemType->name() ] = itemType;
}

void Item::unregisterType( ItemTypePtr itemType )
{
	BW_GUARD;

	if( types().find( itemType->name() ) != types().end() &&
		types()[ itemType->name() ] == itemType )
		types().erase( types().find( itemType->name() ) );
}

void Item::staticInit()
{
	BW_GUARD;

	new BasicItemType( "ACTION" );
	class ChoiceItemType : public BasicItemType
	{
	public:
		ChoiceItemType( const BW::string& typeName ) : BasicItemType( typeName )
		{}
		virtual void shortcutPressed( ItemPtr item )
		{
			if( item->update() )
			{
				for( unsigned int i = 0; i < item->num(); ++i )
					if( ( *item )[ i ]->update() )
					{
						( *item )[ ( i + 1 ) % item->num() ]->act();
						break;
					}
			}
		}
	};
	new ChoiceItemType( "CHOICE" );
	new ChoiceItemType( "EXPANDED_CHOICE" );
	class ToggleItemType : public BasicItemType
	{
	public:
		ToggleItemType() : BasicItemType( "TOGGLE" )
		{}
		virtual bool act( ItemPtr item )
		{
			return ( *item )[ 0 ]->update()	?
				( *item )[ 0 ]->act():
				( *item )[ 1 ]->act();
		}
	};
	new ToggleItemType();
	class ChildItemType : public BasicItemType
	{
	public:
		ChildItemType() : BasicItemType( "CHILD" )
		{}
		virtual bool act( ItemPtr item )
		{
			bool enabled = false;
			for( unsigned int i = 0; i < item->parentNum(); ++i )
			{
				Item* parent = item->parent( i );
				enabled = enabled || parent->update();
			}
			if( enabled && !item->action().empty() )
				return Manager::instance().functors().act( item->action(), item );
			return false;
		}
	};
	new ChildItemType();
}

ItemType::~ItemType()
{}


//-----------------------------------------------------------------------------
// Section: GUI::BasicItemType
//-----------------------------------------------------------------------------

BasicItemType::BasicItemType( const BW::string& typeName ) : name_( typeName )
{
	BW_GUARD;

	Item::registerType( this );
}

BasicItemType::~BasicItemType()
{
}

const BW::string& BasicItemType::name() const
{
	return name_;
}

bool BasicItemType::act( ItemPtr item )
{
	BW_GUARD;

	return ( !item->update() || item->action().empty() ) ? false : Manager::instance().functors().act( item->action(), item );
}

unsigned int BasicItemType::update( ItemPtr item )
{
	BW_GUARD;

	return item->updater().empty() ? 1 : Manager::instance().functors().update( item->updater(), item );
}

void BasicItemType::shortcutPressed( ItemPtr item )
{
	BW_GUARD;

	item->act();
}


END_GUI_NAMESPACE
BW_END_NAMESPACE

