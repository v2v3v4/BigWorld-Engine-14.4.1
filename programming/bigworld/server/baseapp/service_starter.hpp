#ifndef SERVICE_STARTER_HPP
#define SERVICE_STARTER_HPP

#include "network/machine_guard.hpp"
#include "entity_type.hpp"

BW_BEGIN_NAMESPACE

class EntityCreator;

class ServiceStarter : public MachineGuardMessage::ReplyHandler
{
public:
	ServiceStarter();
	bool init();
	bool finishInit( int layoutId );

	bool run( EntityCreator & entityCreator ) const;

private:
	bool onTagsMessage( TagsMessage &tm, uint32 addr );

	bool shouldStartService( const EntityTypePtr & pType ) const;

	void readFragmentsFromLayout( DataSectionPtr pLayout, Tags &fragments );

	Tags tags_;
	bool hasTags_;
};

BW_END_NAMESPACE

#endif // SERVICE_STARTER_HPP
