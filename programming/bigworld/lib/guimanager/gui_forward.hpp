#ifndef GUI_FORWARD_HPP__
#define GUI_FORWARD_HPP__

#ifndef GUI_COMMAND_START
#define GUI_COMMAND_START	( 40000 )
#endif// GUI_COMMAND_START

#ifndef GUI_COMMAND_END
#define GUI_COMMAND_END	( 50000 )
#endif// GUI_COMMAND_END

#define BEGIN_GUI_NAMESPACE	namespace GUI	{
#define END_GUI_NAMESPACE	}
#define USING_GUI_NAMEAPCE using namespace GUI;

#include "cstdmf/smartpointer.hpp"

BW_BEGIN_NAMESPACE
BEGIN_GUI_NAMESPACE

class Item;
typedef SmartPointer<Item> ItemPtr;


class ItemType;
typedef SmartPointer<ItemType> ItemTypePtr;


class Manager;
class Menu;
class Toolbar;


END_GUI_NAMESPACE
BW_END_NAMESPACE

#endif//GUI_FORWARD_HPP__
