#ifndef GAME_LOGIC_FACTORY_HPP
#define GAME_LOGIC_FACTORY_HPP

#include "EntityExtensionFactory.hpp"

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


class Account;
class AlertBeastTrigger;
class Avatar;
class Beast;
class Building;
class Button;
class Creature;
class Door;
class DroppedItem;
class DustDevil;
class Effect;
class Flock;
class Fog;
class Guard;
class GuardRallyPoint;
class GuardSpawner;
class IndoorMapInfo;
class Info;
class Landmark;
class LightGuard;
class MenuScreenAvatar;
class Merchant;
class MovingPlatform;
class PointOfInterest;
class RandomNavigator;
class Reverb;
class Ripper;
class Seat;
class SoundEmitter;
class TeleportSource;
class TriggeredDust;
class VideoScreen;
class VideoScreenChanger;
class WeatherSystem;
class WebScreen;
/**
 *	This class is responsible for creating instances of BWEntity with the
 *	correct sub-class, based on the entity type ID.
 */
class GameLogicFactory : public EntityExtensionFactory
{
public:
	GameLogicFactory() {}
	virtual ~GameLogicFactory() {}

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

private:
};

#endif // GAME_LOGIC_FACTORY_HPP
