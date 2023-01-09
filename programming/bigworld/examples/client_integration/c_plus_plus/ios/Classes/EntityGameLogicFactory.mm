#include "EntityGameLogicFactory.hpp"
#include "GameLogic/AccountGameLogic.hpp"
#include "GameLogic/AlertBeastTriggerGameLogic.hpp"
#include "GameLogic/AvatarGameLogic.hpp"
#include "GameLogic/BeastGameLogic.hpp"
#include "GameLogic/BuildingGameLogic.hpp"
#include "GameLogic/ButtonGameLogic.hpp"
#include "GameLogic/CreatureGameLogic.hpp"
#include "GameLogic/DoorGameLogic.hpp"
#include "GameLogic/DroppedItemGameLogic.hpp"
#include "GameLogic/DustDevilGameLogic.hpp"
#include "GameLogic/EffectGameLogic.hpp"
#include "GameLogic/FlockGameLogic.hpp"
#include "GameLogic/FogGameLogic.hpp"
#include "GameLogic/GuardGameLogic.hpp"
#include "GameLogic/GuardRallyPointGameLogic.hpp"
#include "GameLogic/GuardSpawnerGameLogic.hpp"
#include "GameLogic/IndoorMapInfoGameLogic.hpp"
#include "GameLogic/InfoGameLogic.hpp"
#include "GameLogic/LandmarkGameLogic.hpp"
#include "GameLogic/LightGuardGameLogic.hpp"
#include "GameLogic/MenuScreenAvatarGameLogic.hpp"
#include "GameLogic/MerchantGameLogic.hpp"
#include "GameLogic/MovingPlatformGameLogic.hpp"
#include "GameLogic/PointOfInterestGameLogic.hpp"
#include "GameLogic/RandomNavigatorGameLogic.hpp"
#include "GameLogic/ReverbGameLogic.hpp"
#include "GameLogic/RipperGameLogic.hpp"
#include "GameLogic/SeatGameLogic.hpp"
#include "GameLogic/SoundEmitterGameLogic.hpp"
#include "GameLogic/TeleportSourceGameLogic.hpp"
#include "GameLogic/TriggeredDustGameLogic.hpp"
#include "GameLogic/VideoScreenGameLogic.hpp"
#include "GameLogic/VideoScreenChangerGameLogic.hpp"
#include "GameLogic/WeatherSystemGameLogic.hpp"
#include "GameLogic/WebScreenGameLogic.hpp"

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


AccountExtension * EntityGameLogicFactory::createForEntity( const Account & entity )
{
	return new AccountGameLogic( &entity );
}

AlertBeastTriggerExtension * EntityGameLogicFactory::createForEntity( const AlertBeastTrigger & entity )
{
	return new AlertBeastTriggerGameLogic( &entity );
}

AvatarExtension * EntityGameLogicFactory::createForEntity( const Avatar & entity )
{
	return new AvatarGameLogic( &entity );
}

BeastExtension * EntityGameLogicFactory::createForEntity( const Beast & entity )
{
	return new BeastGameLogic( &entity );
}

BuildingExtension * EntityGameLogicFactory::createForEntity( const Building & entity )
{
	return new BuildingGameLogic( &entity );
}

ButtonExtension * EntityGameLogicFactory::createForEntity( const Button & entity )
{
	return new ButtonGameLogic( &entity );
}

CreatureExtension * EntityGameLogicFactory::createForEntity( const Creature & entity )
{
	return new CreatureGameLogic( &entity );
}

DoorExtension * EntityGameLogicFactory::createForEntity( const Door & entity )
{
	return new DoorGameLogic( &entity );
}

DroppedItemExtension * EntityGameLogicFactory::createForEntity( const DroppedItem & entity )
{
	return new DroppedItemGameLogic( &entity );
}

DustDevilExtension * EntityGameLogicFactory::createForEntity( const DustDevil & entity )
{
	return new DustDevilGameLogic( &entity );
}

EffectExtension * EntityGameLogicFactory::createForEntity( const Effect & entity )
{
	return new EffectGameLogic( &entity );
}

FlockExtension * EntityGameLogicFactory::createForEntity( const Flock & entity )
{
	return new FlockGameLogic( &entity );
}

FogExtension * EntityGameLogicFactory::createForEntity( const Fog & entity )
{
	return new FogGameLogic( &entity );
}

GuardExtension * EntityGameLogicFactory::createForEntity( const Guard & entity )
{
	return new GuardGameLogic( &entity );
}

GuardRallyPointExtension * EntityGameLogicFactory::createForEntity( const GuardRallyPoint & entity )
{
	return new GuardRallyPointGameLogic( &entity );
}

GuardSpawnerExtension * EntityGameLogicFactory::createForEntity( const GuardSpawner & entity )
{
	return new GuardSpawnerGameLogic( &entity );
}

IndoorMapInfoExtension * EntityGameLogicFactory::createForEntity( const IndoorMapInfo & entity )
{
	return new IndoorMapInfoGameLogic( &entity );
}

InfoExtension * EntityGameLogicFactory::createForEntity( const Info & entity )
{
	return new InfoGameLogic( &entity );
}

LandmarkExtension * EntityGameLogicFactory::createForEntity( const Landmark & entity )
{
	return new LandmarkGameLogic( &entity );
}

LightGuardExtension * EntityGameLogicFactory::createForEntity( const LightGuard & entity )
{
	return new LightGuardGameLogic( &entity );
}

MenuScreenAvatarExtension * EntityGameLogicFactory::createForEntity( const MenuScreenAvatar & entity )
{
	return new MenuScreenAvatarGameLogic( &entity );
}

MerchantExtension * EntityGameLogicFactory::createForEntity( const Merchant & entity )
{
	return new MerchantGameLogic( &entity );
}

MovingPlatformExtension * EntityGameLogicFactory::createForEntity( const MovingPlatform & entity )
{
	return new MovingPlatformGameLogic( &entity );
}

PointOfInterestExtension * EntityGameLogicFactory::createForEntity( const PointOfInterest & entity )
{
	return new PointOfInterestGameLogic( &entity );
}

RandomNavigatorExtension * EntityGameLogicFactory::createForEntity( const RandomNavigator & entity )
{
	return new RandomNavigatorGameLogic( &entity );
}

ReverbExtension * EntityGameLogicFactory::createForEntity( const Reverb & entity )
{
	return new ReverbGameLogic( &entity );
}

RipperExtension * EntityGameLogicFactory::createForEntity( const Ripper & entity )
{
	return new RipperGameLogic( &entity );
}

SeatExtension * EntityGameLogicFactory::createForEntity( const Seat & entity )
{
	return new SeatGameLogic( &entity );
}

SoundEmitterExtension * EntityGameLogicFactory::createForEntity( const SoundEmitter & entity )
{
	return new SoundEmitterGameLogic( &entity );
}

TeleportSourceExtension * EntityGameLogicFactory::createForEntity( const TeleportSource & entity )
{
	return new TeleportSourceGameLogic( &entity );
}

TriggeredDustExtension * EntityGameLogicFactory::createForEntity( const TriggeredDust & entity )
{
	return new TriggeredDustGameLogic( &entity );
}

VideoScreenExtension * EntityGameLogicFactory::createForEntity( const VideoScreen & entity )
{
	return new VideoScreenGameLogic( &entity );
}

VideoScreenChangerExtension * EntityGameLogicFactory::createForEntity( const VideoScreenChanger & entity )
{
	return new VideoScreenChangerGameLogic( &entity );
}

WeatherSystemExtension * EntityGameLogicFactory::createForEntity( const WeatherSystem & entity )
{
	return new WeatherSystemGameLogic( &entity );
}

WebScreenExtension * EntityGameLogicFactory::createForEntity( const WebScreen & entity )
{
	return new WebScreenGameLogic( &entity );
}


// EntityGameLogicFactory.cpp
