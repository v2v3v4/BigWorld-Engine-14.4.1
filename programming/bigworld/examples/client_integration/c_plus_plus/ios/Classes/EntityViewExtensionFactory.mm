#include "EntityViewExtensionFactory.hpp"

#include "Classes/EntityViews/AvatarEntityViewExtension.hpp"
#include "Classes/EntityViews/CreatureEntityViewExtension.hpp"
#include "Classes/EntityViews/DoorEntityViewExtension.hpp"
#include "Classes/EntityViews/DroppedItemEntityViewExtension.hpp"
#include "Classes/EntityViews/FlockEntityViewExtension.hpp"
#include "Classes/EntityViews/GenericEntityViewExtension.hpp"
#include "Classes/EntityViews/GuardEntityViewExtension.hpp"
#include "Classes/EntityViews/GuardSpawnerEntityViewExtension.hpp"
#include "Classes/EntityViews/MerchantEntityViewExtension.hpp"
#include "Classes/EntityViews/MovingPlatformEntityViewExtension.hpp"
#include "Classes/EntityViews/SeatEntityViewExtension.hpp"

#include "Entities/Account.hpp"
#include "Entities/AlertBeastTrigger.hpp"
#include "Entities/Avatar.hpp"
#include "Entities/Beast.hpp"
#include "Entities/Building.hpp"
#include "Entities/Button.hpp"
#include "Entities/Creature.hpp"
#include "Entities/Door.hpp"
#include "Entities/DroppedItem.hpp"
#include "Entities/DustDevil.hpp"
#include "Entities/Effect.hpp"
#include "Entities/Flock.hpp"
#include "Entities/Fog.hpp"
#include "Entities/Guard.hpp"
#include "Entities/GuardRallyPoint.hpp"
#include "Entities/GuardSpawner.hpp"
#include "Entities/IndoorMapInfo.hpp"
#include "Entities/Info.hpp"
#include "Entities/Landmark.hpp"
#include "Entities/LightGuard.hpp"
#include "Entities/MenuScreenAvatar.hpp"
#include "Entities/Merchant.hpp"
#include "Entities/MovingPlatform.hpp"
#include "Entities/PointOfInterest.hpp"
#include "Entities/RandomNavigator.hpp"
#include "Entities/Reverb.hpp"
#include "Entities/Ripper.hpp"
#include "Entities/Seat.hpp"
#include "Entities/SoundEmitter.hpp"
#include "Entities/TeleportSource.hpp"
#include "Entities/TriggeredDust.hpp"
#include "Entities/VideoScreen.hpp"
#include "Entities/VideoScreenChanger.hpp"
#include "Entities/WeatherSystem.hpp"
#include "Entities/WebScreen.hpp"

#include "EntityExtensions/AccountExtension.hpp"
#include "EntityExtensions/AlertBeastTriggerExtension.hpp"
#include "EntityExtensions/AvatarExtension.hpp"
#include "EntityExtensions/BeastExtension.hpp"
#include "EntityExtensions/BuildingExtension.hpp"
#include "EntityExtensions/ButtonExtension.hpp"
#include "EntityExtensions/CreatureExtension.hpp"
#include "EntityExtensions/DoorExtension.hpp"
#include "EntityExtensions/DroppedItemExtension.hpp"
#include "EntityExtensions/DustDevilExtension.hpp"
#include "EntityExtensions/EffectExtension.hpp"
#include "EntityExtensions/FlockExtension.hpp"
#include "EntityExtensions/FogExtension.hpp"
#include "EntityExtensions/GuardExtension.hpp"
#include "EntityExtensions/GuardRallyPointExtension.hpp"
#include "EntityExtensions/GuardSpawnerExtension.hpp"
#include "EntityExtensions/IndoorMapInfoExtension.hpp"
#include "EntityExtensions/InfoExtension.hpp"
#include "EntityExtensions/LandmarkExtension.hpp"
#include "EntityExtensions/LightGuardExtension.hpp"
#include "EntityExtensions/MenuScreenAvatarExtension.hpp"
#include "EntityExtensions/MerchantExtension.hpp"
#include "EntityExtensions/MovingPlatformExtension.hpp"
#include "EntityExtensions/PointOfInterestExtension.hpp"
#include "EntityExtensions/RandomNavigatorExtension.hpp"
#include "EntityExtensions/ReverbExtension.hpp"
#include "EntityExtensions/RipperExtension.hpp"
#include "EntityExtensions/SeatExtension.hpp"
#include "EntityExtensions/SoundEmitterExtension.hpp"
#include "EntityExtensions/TeleportSourceExtension.hpp"
#include "EntityExtensions/TriggeredDustExtension.hpp"
#include "EntityExtensions/VideoScreenExtension.hpp"
#include "EntityExtensions/VideoScreenChangerExtension.hpp"
#include "EntityExtensions/WeatherSystemExtension.hpp"
#include "EntityExtensions/WebScreenExtension.hpp"

template< class EXTENSION_TYPE >
EXTENSION_TYPE * makeGenericFor(
		const typename EXTENSION_TYPE::EntityType * pEntity )
{
	return new GenericEntityViewExtension< EXTENSION_TYPE >( pEntity );
}


EntityViewExtensionFactory::EntityViewExtensionFactory():
		EntityExtensionFactory()
{
}


EntityViewExtensionFactory::~EntityViewExtensionFactory()
{
}


AccountExtension * EntityViewExtensionFactory::createForEntity( const Account & entity )
{
	return makeGenericFor< AccountExtension >( &entity );
}


AlertBeastTriggerExtension * EntityViewExtensionFactory::createForEntity( const AlertBeastTrigger & entity )
{
	return makeGenericFor< AlertBeastTriggerExtension >( &entity );
}


AvatarExtension * EntityViewExtensionFactory::createForEntity( const Avatar & entity )
{
	return new AvatarEntityViewExtension( &entity );
}


BeastExtension * EntityViewExtensionFactory::createForEntity( const Beast & entity )
{
	return makeGenericFor< BeastExtension >( &entity );
}


BuildingExtension * EntityViewExtensionFactory::createForEntity( const Building & entity )
{
	return makeGenericFor< BuildingExtension >( &entity );
}


ButtonExtension * EntityViewExtensionFactory::createForEntity( const Button & entity )
{
	return makeGenericFor< ButtonExtension >( &entity );
}


CreatureExtension * EntityViewExtensionFactory::createForEntity( const Creature & entity )
{
	return new CreatureEntityViewExtension( &entity );
}


DoorExtension * EntityViewExtensionFactory::createForEntity( const Door & entity )
{
	return new DoorEntityViewExtension( &entity );
}


DroppedItemExtension * EntityViewExtensionFactory::createForEntity( const DroppedItem & entity )
{
	return new DroppedItemEntityViewExtension( &entity );
}


DustDevilExtension * EntityViewExtensionFactory::createForEntity( const DustDevil & entity )
{
	return makeGenericFor< DustDevilExtension >( &entity );
}


EffectExtension * EntityViewExtensionFactory::createForEntity( const Effect & entity )
{
	return makeGenericFor< EffectExtension >( &entity );
}


FlockExtension * EntityViewExtensionFactory::createForEntity( const Flock & entity )
{
	return makeGenericFor< FlockExtension >( &entity );
}


FogExtension * EntityViewExtensionFactory::createForEntity( const Fog & entity )
{
	return makeGenericFor< FogExtension >( &entity );
}


GuardExtension * EntityViewExtensionFactory::createForEntity( const Guard & entity )
{
	return new GuardEntityViewExtension( &entity );
}


GuardRallyPointExtension * EntityViewExtensionFactory::createForEntity( const GuardRallyPoint & entity )
{
	return makeGenericFor< GuardRallyPointExtension >( &entity );
}


GuardSpawnerExtension * EntityViewExtensionFactory::createForEntity( const GuardSpawner & entity )
{
	return new GuardSpawnerEntityViewExtension( &entity );
}


IndoorMapInfoExtension * EntityViewExtensionFactory::createForEntity( const IndoorMapInfo & entity )
{
	return makeGenericFor< IndoorMapInfoExtension >( &entity );
}


InfoExtension * EntityViewExtensionFactory::createForEntity( const Info & entity )
{
	return makeGenericFor< InfoExtension >( &entity );
}


LandmarkExtension * EntityViewExtensionFactory::createForEntity( const Landmark & entity )
{
	return makeGenericFor< LandmarkExtension >( &entity );
}


LightGuardExtension * EntityViewExtensionFactory::createForEntity( const LightGuard & entity )
{
	return makeGenericFor< LightGuardExtension >( &entity );
}


MenuScreenAvatarExtension * EntityViewExtensionFactory::createForEntity( const MenuScreenAvatar & entity )
{
	return makeGenericFor< MenuScreenAvatarExtension >( &entity );
}


MerchantExtension * EntityViewExtensionFactory::createForEntity( const Merchant & entity )
{
	return new MerchantEntityViewExtension( &entity );
}


MovingPlatformExtension * EntityViewExtensionFactory::createForEntity( const MovingPlatform & entity )
{
	return new MovingPlatformEntityViewExtension( &entity );
}


PointOfInterestExtension * EntityViewExtensionFactory::createForEntity( const PointOfInterest & entity )
{
	return makeGenericFor< PointOfInterestExtension>( &entity );
}


RandomNavigatorExtension * EntityViewExtensionFactory::createForEntity( const RandomNavigator & entity )
{
	return makeGenericFor< RandomNavigatorExtension >( &entity );
}


ReverbExtension * EntityViewExtensionFactory::createForEntity( const Reverb & entity )
{
	return makeGenericFor< ReverbExtension >( &entity );
}


RipperExtension * EntityViewExtensionFactory::createForEntity( const Ripper & entity )
{
	return makeGenericFor< RipperExtension >( &entity );
}


SeatExtension * EntityViewExtensionFactory::createForEntity( const Seat & entity )
{
	return new SeatEntityViewExtension( &entity );
}


SoundEmitterExtension * EntityViewExtensionFactory::createForEntity( const SoundEmitter & entity )
{
	return makeGenericFor< SoundEmitterExtension >( &entity );
}


TeleportSourceExtension * EntityViewExtensionFactory::createForEntity( const TeleportSource & entity )
{
	return makeGenericFor< TeleportSourceExtension >( &entity );
}


TriggeredDustExtension * EntityViewExtensionFactory::createForEntity( const TriggeredDust & entity )
{
	return makeGenericFor< TriggeredDustExtension >( &entity );
}


VideoScreenExtension * EntityViewExtensionFactory::createForEntity( const VideoScreen & entity )
{
	return makeGenericFor< VideoScreenExtension >( &entity );
}


VideoScreenChangerExtension * EntityViewExtensionFactory::createForEntity( const VideoScreenChanger & entity )
{
	return makeGenericFor< VideoScreenChangerExtension >( &entity );
}


WeatherSystemExtension * EntityViewExtensionFactory::createForEntity( const WeatherSystem & entity )
{
	return makeGenericFor< WeatherSystemExtension >( &entity );
}


WebScreenExtension * EntityViewExtensionFactory::createForEntity( const WebScreen & entity )
{
	return makeGenericFor< WebScreenExtension >( &entity );
}


// EntityViewExtensionFactory.mm
