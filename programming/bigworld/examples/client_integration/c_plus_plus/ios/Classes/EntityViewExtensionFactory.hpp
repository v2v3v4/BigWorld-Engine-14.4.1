#ifndef ENTITY_VIEW_EXTENSION_FACTORY_HPP
#define ENTITY_VIEW_EXTENSION_FACTORY_HPP

#include "EntityExtensionFactory.hpp"



class EntityViewExtensionFactory : public EntityExtensionFactory
{
public:
	EntityViewExtensionFactory();
	
	virtual ~EntityViewExtensionFactory();
	
	virtual AccountExtension * createForEntity( const Account & entity );
	virtual AlertBeastTriggerExtension * createForEntity( const AlertBeastTrigger & entity );
	virtual AvatarExtension * createForEntity( const Avatar & entity );
	virtual BeastExtension * createForEntity( const Beast & entity );
	virtual BuildingExtension * createForEntity( const Building & entity );
	virtual ButtonExtension * createForEntity( const Button & entity );
	virtual CreatureExtension * createForEntity( const Creature & entity );
	virtual DoorExtension * createForEntity( const Door & entity );
	virtual DroppedItemExtension * createForEntity( const DroppedItem & entity );
	virtual DustDevilExtension * createForEntity( const DustDevil & entity );
	virtual EffectExtension * createForEntity( const Effect & entity );
	virtual FlockExtension * createForEntity( const Flock & entity );
	virtual FogExtension * createForEntity( const Fog & entity );
	virtual GuardExtension * createForEntity( const Guard & entity );
	virtual GuardRallyPointExtension * createForEntity( const GuardRallyPoint & entity );
	virtual GuardSpawnerExtension * createForEntity( const GuardSpawner & entity );
	virtual IndoorMapInfoExtension * createForEntity( const IndoorMapInfo & entity );
	virtual InfoExtension * createForEntity( const Info & entity );
	virtual LandmarkExtension * createForEntity( const Landmark & entity );
	virtual LightGuardExtension * createForEntity( const LightGuard & entity );
	virtual MenuScreenAvatarExtension * createForEntity( const MenuScreenAvatar & entity );
	virtual MerchantExtension * createForEntity( const Merchant & entity );
	virtual MovingPlatformExtension * createForEntity( const MovingPlatform & entity );
	virtual PointOfInterestExtension * createForEntity( const PointOfInterest & entity );
	virtual RandomNavigatorExtension * createForEntity( const RandomNavigator & entity );
	virtual ReverbExtension * createForEntity( const Reverb & entity );
	virtual RipperExtension * createForEntity( const Ripper & entity );
	virtual SeatExtension * createForEntity( const Seat & entity );
	virtual SoundEmitterExtension * createForEntity( const SoundEmitter & entity );
	virtual TeleportSourceExtension * createForEntity( const TeleportSource & entity );
	virtual TriggeredDustExtension * createForEntity( const TriggeredDust & entity );
	virtual VideoScreenExtension * createForEntity( const VideoScreen & entity );
	virtual VideoScreenChangerExtension * createForEntity( const VideoScreenChanger & entity );
	virtual WeatherSystemExtension * createForEntity( const WeatherSystem & entity );
	virtual WebScreenExtension * createForEntity( const WebScreen & entity );
	
};

#endif // ENTITY_VIEW_EXTENSION_FACTORY_HPP
