#ifndef WATCHER_FORWARDING_TYPES_HPP
#define WATCHER_FORWARDING_TYPES_HPP

#include <utility>

BW_BEGIN_NAMESPACE

enum ComponentType
{
	CELL_APP,
	BASE_APP,
	SERVICE_APP,
};

// At present, ComponentID needs to handle CellAppID, BaseAppID and ServiceAppID
typedef int32 ComponentID;
typedef BW::vector<ComponentID> ComponentIDList;
typedef std::pair<ComponentType,ComponentID> Component;
typedef std::pair<Mercury::Address,Component> AddressPair;
typedef BW::vector<AddressPair> AddressList;

BW_END_NAMESPACE

#endif
