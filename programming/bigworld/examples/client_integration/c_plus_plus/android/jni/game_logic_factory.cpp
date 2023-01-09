#include "game_logic_factory.hpp"

#include "GameLogic/AccountGameLogic.hpp"
//#include "GameLogic/AlertBeastTriggerGameLogic.hpp"
#include "GameLogic/AvatarGameLogic.hpp"
/*
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
*/
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


AccountExtension * GameLogicFactory::createForEntity( const Account & entity )
{
	return new AccountGameLogic( &entity );
}


AlertBeastTriggerExtension * GameLogicFactory::createForEntity( const AlertBeastTrigger & entity )
{
	return NULL;
}


AvatarExtension * GameLogicFactory::createForEntity( const Avatar & entity )
{
	return new AvatarGameLogic( &entity );
}


BeastExtension * GameLogicFactory::createForEntity( const Beast & entity )
{
	return new BeastExtension( &entity );
}

BuildingExtension * GameLogicFactory::createForEntity( const Building & entity )
{
	return new BuildingExtension( &entity );
}

ButtonExtension * GameLogicFactory::createForEntity( const Button & entity )
{
	return new ButtonExtension( &entity );
}

CreatureExtension * GameLogicFactory::createForEntity( const Creature & entity )
{
	return new CreatureExtension( &entity );
}

DoorExtension * GameLogicFactory::createForEntity( const Door & entity )
{
	return new DoorExtension( &entity );
}

DroppedItemExtension * GameLogicFactory::createForEntity( const DroppedItem & entity )
{
	return new DroppedItemExtension( &entity );
}

DustDevilExtension * GameLogicFactory::createForEntity( const DustDevil & entity )
{
	return new DustDevilExtension( &entity );
}

EffectExtension * GameLogicFactory::createForEntity( const Effect & entity )
{
	return new EffectExtension( &entity );
}

FlockExtension * GameLogicFactory::createForEntity( const Flock & entity )
{
	return new FlockExtension( &entity );
}

FogExtension * GameLogicFactory::createForEntity( const Fog & entity )
{
	return new FogExtension( &entity );
}

GuardExtension * GameLogicFactory::createForEntity( const Guard & entity )
{
	return new GuardExtension( &entity );
}

GuardRallyPointExtension * GameLogicFactory::createForEntity( const GuardRallyPoint & entity )
{
	return new GuardRallyPointExtension( &entity );
}

GuardSpawnerExtension * GameLogicFactory::createForEntity( const GuardSpawner & entity )
{
	return new GuardSpawnerExtension( &entity );
}

IndoorMapInfoExtension * GameLogicFactory::createForEntity( const IndoorMapInfo & entity )
{
	return new IndoorMapInfoExtension( &entity );
}

InfoExtension * GameLogicFactory::createForEntity( const Info & entity )
{
	return new InfoExtension( &entity );
}

LandmarkExtension * GameLogicFactory::createForEntity( const Landmark & entity )
{
	return new LandmarkExtension( &entity );
}

LightGuardExtension * GameLogicFactory::createForEntity( const LightGuard & entity )
{
	return new LightGuardExtension( &entity );
}

MenuScreenAvatarExtension * GameLogicFactory::createForEntity( const MenuScreenAvatar & entity )
{
	return new MenuScreenAvatarExtension( &entity );
}

MerchantExtension * GameLogicFactory::createForEntity( const Merchant & entity )
{
	return new MerchantExtension( &entity );
}

MovingPlatformExtension * GameLogicFactory::createForEntity( const MovingPlatform & entity )
{
	return new MovingPlatformExtension( &entity );
}

PointOfInterestExtension * GameLogicFactory::createForEntity( const PointOfInterest & entity )
{
	return new PointOfInterestExtension( &entity );
}

RandomNavigatorExtension * GameLogicFactory::createForEntity( const RandomNavigator & entity )
{
	return new RandomNavigatorExtension( &entity );
}

ReverbExtension * GameLogicFactory::createForEntity( const Reverb & entity )
{
	return new ReverbExtension( &entity );
}

RipperExtension * GameLogicFactory::createForEntity( const Ripper & entity )
{
	return new RipperExtension( &entity );
}

SeatExtension * GameLogicFactory::createForEntity( const Seat & entity )
{
	return new SeatExtension( &entity );
}

SoundEmitterExtension * GameLogicFactory::createForEntity( const SoundEmitter & entity )
{
	return new SoundEmitterExtension( &entity );
}

TeleportSourceExtension * GameLogicFactory::createForEntity( const TeleportSource & entity )
{
	return new TeleportSourceExtension( &entity );
}

TriggeredDustExtension * GameLogicFactory::createForEntity( const TriggeredDust & entity )
{
	return new TriggeredDustExtension( &entity );
}

VideoScreenExtension * GameLogicFactory::createForEntity( const VideoScreen & entity )
{
	return new VideoScreenExtension( &entity );
}

VideoScreenChangerExtension * GameLogicFactory::createForEntity( const VideoScreenChanger & entity )
{
	return new VideoScreenChangerExtension( &entity );
}

WeatherSystemExtension * GameLogicFactory::createForEntity( const WeatherSystem & entity )
{
	return new WeatherSystemExtension( &entity );
}

WebScreenExtension * GameLogicFactory::createForEntity( const WebScreen & entity )
{
	return new WebScreenExtension( &entity );
}


// game_logic_factory.cpp
