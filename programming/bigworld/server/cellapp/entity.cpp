#include "script/first_include.hpp"

#include "entity.hpp"

#include "buffered_ghost_message_factory.hpp"
#include "buffered_ghost_messages.hpp"
#include "cell.hpp"
#include "cell_app_channels.hpp"
#include "cell_chunk.hpp"
#include "cellapp.hpp"
#include "cellapp_config.hpp"
#include "controllers.hpp"
#include "entity_extra.hpp"
#include "entity_population.hpp"
#include "entity_range_list_node.hpp"
#include "mailbox.hpp"
#include "noise_config.hpp"
#include "passengers.hpp"
#include "passenger_extra.hpp"
#include "portal_config_controller.hpp"
#include "py_client.hpp"
#include "range_list_appeal_trigger.hpp"
#include "range_trigger.hpp"
#include "real_caller.hpp"
#include "real_entity.hpp"
#include "replay_data_collector.hpp"
#include "space.hpp"
#include "witness.hpp"

#include "baseapp/baseapp_int_interface.hpp"

#include "chunk/chunk_space.hpp"
#include "chunk/chunk_obstacle.hpp"
#include "cstdmf/debug.hpp"
#include "cstdmf/memory_stream.hpp"
#include "cstdmf/stringmap.hpp"
#include "cstdmf/profiler.hpp"

#include "common/closest_triangle.hpp"

#include "connection/baseapp_ext_interface.hpp"
#include "connection/client_interface.hpp"

#include "entitydef/entity_delegate_helpers.hpp"
#include "entitydef/entity_description_map.hpp"
#include "entitydef/property_change.hpp"
#include "entitydef/script_data_sink.hpp"
#include "entitydef/script_data_source.hpp"

#include "entitydef_script/py_component.hpp"
#include "entitydef_script/delegate_entity_method.hpp"

#include "delegate_interface/game_delegate.hpp"

#include "network/bundle.hpp"
#include "network/channel_sender.hpp"
#include "network/compression_stream.hpp"
#include "network/exposed_message_range.hpp"

#include "physics2/worldtri.hpp"
#include "pyscript/pyobject_base.hpp"

#include "resmgr/bwresource.hpp"
#include "server/client_method_calling_flags.hpp"
#include "server/stream_helper.hpp"
#include "server/util.hpp"
#include "server/writedb.hpp"
#include "server/event_history_stats.hpp"

#include "face_entity_controller.hpp"
#include "timer_controller.hpp"
#include "turn_controller.hpp"

#include <algorithm>


DECLARE_DEBUG_COMPONENT( 0 );


BW_BEGIN_NAMESPACE

class Cell;

#ifndef CODE_INLINE
#include "entity.ipp"
#endif

EntityMovementCallback g_entityMovementCallback;

namespace
{
uint32 g_numEntitiesEver = 0;

uint32 g_nonVolatileBytes = 0;
uint32 g_volatileBytes = 0;

EventHistoryStats g_publicClientStats;
EventHistoryStats g_totalPublicClientStats;
EventHistoryStats g_privateClientStats;

size_t g_numEntities = 0;

uint32 numEntitiesLeaked()
{
	return Entity::population().size() - g_numEntities;

}

/**
 *	This method gets the IEntityDelegateFactory* from the singleton
 */
static IEntityDelegateFactory * getEntityDelegateFactory()
{
	IGameDelegate * pGameDelegate = IGameDelegate::instance();

	if (!pGameDelegate)
	{
		return NULL;
	}

	//INFO_MSG( "getEntityDelegateFactory: Getting IEntityDelegateFactory\n" );

	return pGameDelegate->getEntityDelegateFactory();
}

} // anonymous namespace


// -----------------------------------------------------------------------------
// Section: Entity
// -----------------------------------------------------------------------------

const Vector3 Entity::INVALID_POSITION( FLT_MAX, FLT_MAX, FLT_MAX );

// Not sure where or how these are defined, so they are here for now...
	/*~ attribute Entity base
	*  @components{ cell }
	*  This attribute provides a BaseEntityMailbox object. This can
	*  be used to call functions on the base Entity.
	*  @type Read-only BaseEntityMailBox
	*/
	/*~ attribute Entity client
	*  @components{ cell }
	*
	*  This attribute is an alias for Entity.ownClient.
	*  It provides a PyClient object. This can be
	*  used to call functions on the client Entity. Function calls that are made
	*  via this PyClient object are only passed to the client that owns the
	*  entity.
	*
	*  @see clientEntity
	*  @see allClients
	*  @see otherClients
	*  @type Read-only PyClient
	*/
	/*~ attribute Entity ownClient
	*  @components{ cell }
	*  This attribute provides a PyClient object. This can be
	*  used to call functions on the client Entity. Function calls that are made
	*  via this PyClient object are only passed to the client that owns the
	*  entity.
	*
	*  @see clientEntity
	*  @see allClients
	*  @see otherClients
	*  @type Read-only PyClient
	*/
	/*~ attribute Entity otherClients
	*  @components{ cell }
	*  When read, the otherClients attribute provides a PyClient object. This can
	*  be used to call functions on the client Entity. Function calls that are
	*  made via this PyClient object are passed to all clients except the one that
	*  owns the entity.
	*
	*  @see ownClient
	*  @see allClients
	*  @see clientEntity
	*  @type Read-only PyClient
	*/
	/*~ attribute Entity allClients
	*  @components{ cell }
	*  When read, the allClients attribute provides a PyClient object. This can be
	*  used to call functions on the client Entity. Function calls that are made
	*  via this PyClient object are passed to all clients.
	*
	*  @see ownClient
	*  @see clientEntity
	*  @see otherClients
	*  @type Read-only PyClient
	*/
PY_BASETYPEOBJECT( Entity )

PY_BEGIN_METHODS( Entity )

	/*~ function Entity destroy
	*  @components{ cell }
	*  This function destroys entity by destroying its local Entity instance,
	*  and informing every other application which holds some form of instance of
	*  it to do the same. It is expected to be called by the entity itself, and
	*  will throw a TypeError if the entity is a ghost. The callback onDestroy()
	*  is called if present.
	*/
	PY_METHOD( destroy )

	/*~ function Entity cancel
	*  @components{ cell }
	*  The cancel function stops a controller from affecting the Entity. It can
	*  only be called on a real entity.
	*  @param controllerID controllerID is an integer which is the index of the
	*  controller to cancel. Alternatively, a string of an exclusive controller
	*  category can be passed to cancel the controller of that category. For
	*  example, only one movement/navigation controller can be active at once.
	*  This can be cancelled with entity.cancel( "Movement" ).
	*/
	PY_METHOD( cancel )

	/*~ function Entity debug
	*  @components{ cell }
	*  The debug function generates debugging information regarding the position
	*  of this entity in the cell's sorted position lists (which contain all
	*  entities known to the cell, sorted by their position on the x and z axis),
	*  and sends it to wherever the cell is directing its debug messages. This
	*  cannot be set via Python, and defaults to stdout.
	*  The first line of debug information provides the entity's position on the
	*  cell's x axis, its type and ID, type and ID information for the previous
	*  object along the x axis, and type and ID information for the next object
	*  on the x axis. The second line is identical to the first, except that the
	*  data relates to the cell's z axis.
	*
	*  Sample Output:
	*  @{
	*  107.9 DEBUG: RangeListNode::debugX: me.PosX=-347.4982910, me=[Entity 16777218], prevX=[Head], nextX=[Entity 16777216]
	*  107.9 DEBUG: RangeListNode::debugZ: me.PosZ=369.6944580, me=[Entity 16777218], prevZ=[Entity 16777216], nextZ=[Tail]
	*  @}
	*/
	PY_METHOD( debug )

	/*~ function Entity isReal
	*  @components{ cell }
	*  The isReal method returns whether this Entity is real or a ghost.
	*
	*  This method should be rarely used but can be helpful for debugging.
	*
	*  @return (bool) True if the entity is real, False if it is not.
	*/
	PY_METHOD( isReal )

	/*~ function Entity isRealToScript
	*  @components{ cell }
	*  The isRealToScript method returns whether it is safe to treat this Entity
	*  as a Real entity. This method can be different from Entity.isReal()
	*  when cellApp/treatAllOtherEntitiesAsGhosts is set to True. This method
	*  will return False if script should consider this a ghost entity in the
	*  current calling context.
	*
	*  This method should be rarely used but can be helpful for debugging.
	*
	*  @return (bool) True if the entity is real to script, otherwise False.
	*/
	PY_METHOD( isRealToScript )

	/*~ function Entity getComponent
	 *	@components{ cell }
	 *	This method gets a component on an entity
	 */
	PY_METHOD( getComponent )

	/*~ function Entity destroySpace
	 *  @components{ cell }
	 *	This method attempts to shut down the space that the entity is in.
	 *	It is not possible to shut down the default space.
	 */
	PY_METHOD( destroySpace )

	/*~ function Entity writeToDB
	 *  @components{ cell }
	 *	This method write the data associated with this entity to the database.
	 *	It includes the base entity's data. The base entity's onWriteToDB
	 *	method will be called before the data is actually sent to the database.
	 *
	 *	The cell entity's data is also backed up to the base entity to ensure
	 *	that the fault tolerance data is at least as up-to-date as the disaster
	 *	recovery data.
	 *
	 *	This method can only be called on a real entity that has a base entity.
	 */
	PY_METHOD( writeToDB )

	/*~ function Entity entitiesInRange
	*  @components{ cell }
	*  The entitiesInRange function searches for entities whose position
	*  attributes are within a given distance from the Entity. This is a spherical
	*  search, as the distance is measured across all three axis.
	*  This can find entities which are outside the Entity's
	*  area of interest, but cannot find entities which are not known to the cell.
	*  For example:
	*  @{
	*    self.entitiesInRange( 100, 'Creature', (100,0,100) )
	*  @}
	*  Gives a list of entities that are instance of class Creature, but does not include
	*  entities that are instances of subclasses of Creature. The search radius is 100
	*  metres using (100,0,100) as centre.
	*  @{
	*    [ e for e in self.entitiesInRange( 100, None, (100,0,100) ) if isinstance( e, BaseType ) ]
	*  @}
	*  will give a list of entities that are instantiated from BaseType or subclasses of BaseType
	*  @param range range is the distance around this entity to search, as a float.
	*  @param entityType=None An optional string argument of entity type name that
	*  will be used in matching entities. If entityType is a valid class name then only
	*  entities of that class will be returned, otherwise all entities in range are returned.
	*  @param position An optional Vector3 argument to be used as the centre of
	*  the searching radius.
	*  @return A list of Entity objects that are within the given range.
	*/
	PY_METHOD( entitiesInRange )

	/*~ function Entity clientEntity
	*  @components{ cell }
	*  The clientEntity function can be used to make calls on a single instance
	*  of an entity, as opposed to all instances as is the case with
	*  Entity.allClients.
	*
	*  For example, playerEntity.clientEntity( buttonEntityID ).flash() will
	*  call the flash() method on the button entity only on this player's
	*  application. Any other player with the button in their AoI will not have
	*  this method called.
	*
	*  @see ownClient
	*  @see allClients
	*  @see otherClients
	*  @param destID destID is the ID number of the entity to which the new
	*  PyClient is to be associated.
	*  @return A PyClient for the specified entity.
	*/
	PY_METHOD( clientEntity )

	/*~ function Entity makeNoise
	*  @components{ cell }
	*  The makeNoise function informs all other entities within a specified
	*  range that this entity has made a sound.
	*
	*  It does this by calling the onNoise function of each of these entities.
	*  onNoise must take five arguments, and is not defined by default. Errors
	*  thrown by the attempt to call onNoise are silently ignored. The
	*  arguments with which onNoise is called are the ID of the entity which
	*  made the noise, the distance from the source entity at which the noise
	*  can still be heard (as a float), the distance at which the noise was
	*  heard (as a float), an integer indicating the event which caused the
	*  sound, and an arbitrary user supplied integer which can be used to
	*  provide more information.
	*
	*  The distance between entities for noise propagation purposes is
	*  calculated as the shortest route of travel via chunk portals. This means
	*  that, for instance, if two chunks represent long parallel corridors,
	*  then a noise made in one may not be heard by an entity standing close by
	*  in the other.
	*
	*  This function can only be called on a real entity, a TypeError is raised
	*  if this is not the case.
	*
	*  @param noiseLevel	The noiseLevel parameter is a float which is
	*						multiplied by the value found in
	*						server/bw.xml/cellApp/noise/standardRange to
	*						determine the maximum distance at which the noise
	*						can be heard.
	*  @param event			The event argument is an integer which indicates
	*						the type of event that caused the sound. This is 0
	*						for movement related noise (see setStealthFactor),
	*						and is otherwise unrestricted.
	*  @param info=0		The info argument is optional, and can be used to
	*						supply further information regarding a sound to the
	*						entities which hear it, as an integer. This is used
	*						as the fifth argument in all calls to onNoise that
	*						this makes.
	*/
	PY_METHOD( makeNoise )

	/*~ function Entity getGroundPosition
	*  @components{ cell }
	*  getGroundPosition provides the position of the ground immediately below the
	*  entity in world coordinates, or the position of the entity if there is no
	*  ground below it.
	*  @return The position of the ground below the entity (or the entity itself,
	*  if no ground is found), expressed as 3 floats (x, y, z), in worldspace.
	*/
	PY_METHOD( getGroundPosition )

	/*~ function Entity trackEntity
	*  @components{ cell }
	*  The trackEntity function causes the Entity to turn towards another entity.
	*  This is achieved by creating a new controller and adding it to the Entity.
	*  It can only be called on a real entity, and the controller can be removed
	*  via the Entity's cancel function.
	*  @see cancel
	*  @param entityID The ID of the entity that this should turn towards. This
	*  entity must be known by the cell.
	*  @param velocity=2*pi velocity is the speed at which the Entity will turn, in
	*  radians per second.
	*  @param period=10 ticks ticks is the number of ticks that occur between
	*  updates. This affects how accurately the Entity will track its target.
	*  @param userArg=0 userArg is an integer, and is not currently used.
	*  @return trackEntity returns the ID of the new controller. This can later be
	*  used with the Entity's cancel function to remove the controller and stop
	*  the Entity from tracking.
	*/
	PY_METHOD( trackEntity )

	/*~ function Entity.setPortalState
	 *  @components{ cell }
	 *  The setPortalState function allows an entity to control the state of a
	 *	portal.
	 *
	 *	The position of the entity must be within 1 metre of the portal that is
	 *	to be configured, otherwise the portal will probably not be found. No
	 *	other portal may be closer.
	 *
	 *	@param permissive	Whether the portal is open or not.
	 *  @param triFlags Optional argument that sets the flags on the portal's
	 *		triangles as returned from collision tests.
	 */
	PY_METHOD( setPortalState )

	/*~ function Entity getDict
	 *	@components{ cell }
	 *	The getDict function constructs a dictionary containing all of this
	 *	entity's properties that are defined in its def file. Modifying this
	 *	dictionary does not alter this entity's properties.
	 *
	 *	This is a useful debugging method but should be avoided in production.
	 */
	PY_METHOD( getDict )

	PY_PICKLING_METHOD()

PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( Entity )

	/*~ attribute Entity aoiUpdateScheme
	 *	@components{ cell }
	 *	This property affects the frequency at which updates about this entity
	 *	are sent to clients. Note that this only affects the scheme used by new
	 *	AoIs that this entity enters.
	 *
	 *	The available schemes are specified in bw.xml's cellApp/aoiUpdateSchemes
	 *	section.
	 */
	PY_ATTRIBUTE( aoiUpdateScheme )

	PY_ATTRIBUTE( periodsWithoutWitness )

	/*~ attribute Entity vehicle
	*  @components{ cell }
	*  vehicle is the vehicle Entity that this Entity is currently riding in.
	*  @type Entity
	*/
	PY_ATTRIBUTE( vehicle )

	/*~ attribute Entity volatileInfo
	*  @components{ cell }
	*  The volatileInfo attribute specifies how often (if ever) the Entity's
	*  volatile data should be sent to clients. The four float (or None) values
	*  that this contains are the position priority, yaw priority, pitch priority,
	*  and roll priority (where higher numbers result in more frequent updates,
	*  and None indicates that no updates are to be sent at all).
	*  The values for these are initially read from the entity's .def file,
	*  defaulting to None if not provided. They can later be specified, and are
	*  each clamped between BigWorld.VOLATILE_NEVER and BigWorld.VOLATILE_ALWAYS.
	*  The latter three elements must be in descending order (where 0 is
	*  interpreted as being greater than None), or else a TypeError will be
	*  thrown.
	*
	*  Sample .def Excerpt:
	*  @{
	*  &lt;Volatile&gt;
	*      &lt;position/&gt;           &lt;!-- always --&gt;
	*      &lt;yaw/&gt;                &lt;!-- always --&gt;
	*      &lt;pitch&gt;20&lt;/pitch&gt;     &lt;!-- 20     --&gt;
	*  &lt;/Volatile&gt;               &lt;!-- roll never  --&gt;
	*  @}
	*  @type A sequence of 4 floats (or None objects)
	*/
	PY_ATTRIBUTE( volatileInfo )

	/*~ attribute Entity className
	 *  @components{ cell }
	 *
	 *	The class name of this Entity.
	 *	@type Read-only string
	 */
	PY_ATTRIBUTE( className )
	/*~ attribute Entity id
	*  @components{ cell }
	*  This is the unique identifier for the Entity. It is common across client,
	*  cell, and base applications.
	*  @type Read-only Integer
	*/
	PY_ATTRIBUTE( id )

	/*~ attribute Entity spaceID
	*  @components{ cell }
	*  This is the unique identifier for the space that currently contains the
	*  Entity. It is common across client and cell applications.
	*  @type Read-only Integer.
	*/
	PY_ATTRIBUTE( spaceID )

	/*~ attribute Entity position
	*  @components{ cell }
	*  The position attribute is the location of the Entity, in world
	*  coordinates. This can be written to. If the entity has a client who is
	*  modifying its position, these changes will be ignored until the client
	*  acknowledges this position change.
	*
	*  It is also important to consider the volatileInfo of the entity. If the
	*  position is not volatile, each change to this value will be sent down to
	*  all client applications within range.
	*
	*  Note: This property is mutable which means that you can change individual
	*  elements of the position. It also means if you keep a reference to it and
	*  the entity moves, this property will change.
	*  @{
	*  self.position.y = 50.0
	*  @}
	*
	*  If you want a copy of the current value, you should cast to a Vector3.
	*
	*  For example,
	*  @{
	*  self.startPosition = Math.Vector3( self.position )
	*  @}
	*
	*  @type Vector3
	*/
	PY_ATTRIBUTE( position )

	/*~ attribute Entity yaw
	*  @components{ cell }
	*  yaw provides the yaw component of the direction attribute. It is the yaw
	*  of the current facing direction of the Entity, in radians and in world
	*  space.
	*  This can be set via the direction attribute.
	*  @type Read-only Float
	*/
	PY_ATTRIBUTE( yaw )

	/*~ attribute Entity pitch
	*  @components{ cell }
	*  pitch provides the pitch component of the direction attribute. It is
	*  the pitch of the current facing direction of the Entity, in radians
	*  and in world space.
	*  This can be set via the direction attribute.
	*  @type Read-only Float
	*/
	PY_ATTRIBUTE( pitch )

	/*~ attribute Entity roll
	*  @components{ cell }
	*  roll provides the roll component of the direction attribute. It is the
	*  roll of the current facing direction of the Entity, in radians and in
	*  world space.
	*  This can be set via the direction attribute.
	*  @type Read-only Float
	*/
	PY_ATTRIBUTE( roll )
	/*~ attribute Entity direction
	*  @components{ cell }
	*  The direction attribute is the current orientation of the Entity, in
	*  world space. This can be written to, however entities for whom
	*  a client is authoritative (in regards to position) will have this
	*  data overwritten.
	*
	*  Changes to this data will not be propagated to clients if the
	*  volatileInfo for this Entity specifies that position is not to be
	*  updated.
	*
	*  This property is roll, pitch and yaw combined (in that order). All
	*  values are in radians.
	*
	*  Like Entity.position, this value is mutable.
	*
	*  If you want a copy of the current value, you should cast to a Vector3.
	*
	*  For example,
	*  @{
	*  self.startDirection = Math.Vector3( self.direction )
	*  @}
	*
	*
	*  @type Tuple of 3 floats as (roll, pitch, yaw)
	*/
	PY_ATTRIBUTE( direction )

	/*~ attribute Entity localPosition
	 *	@components{ cell }
	 *	This read-only attribute returns the position of this entity relative
	 *	to the vehicle that it is riding on.
	 *	@type Tuple of 3 floats as (x, y, z).
	 */
	PY_ATTRIBUTE( localPosition )

	/*~ attribute Entity localYaw
	 *	@components{ cell }
	 *	This read-only attribute returns yaw of this entity relative to the
	 *	vehicle that it is riding on.
	 *	@type Read-only Float.
	 */
	PY_ATTRIBUTE( localYaw )

	/*~ attribute Entity localPitch
	 *	@components{ cell }
	 *	This read-only attribute returns pitch of this entity relative to the
	 *	vehicle that it is riding on.
	 *	@type Read-only Float.
	 */
	PY_ATTRIBUTE( localPitch )

	/*~ attribute Entity localRoll
	 *	@components{ cell }
	 *	This read-only attribute returns roll of this entity relative to the
	 *	vehicle that it is riding on.
	 *	@type Read-only Float.
	 */
	PY_ATTRIBUTE( localRoll )

/*
	PY_ATTRIBUTE( client )
	PY_ATTRIBUTE( otherClients )
	PY_ATTRIBUTE( allClients )
	PY_ATTRIBUTE( direction )
*/
	/*~ attribute Entity isDestroyed
	*  @components{ cell }
	*  The isDestroyed attribute will be True if the Entity has been destroyed.
	*  @type Read-only bool
	*/
	PY_ATTRIBUTE( isDestroyed )

	/*~ attribute Entity debug_numTimesRealOffloaded
	 *	@components{ cell }
	 *	The debug_numTimesRealOffloaded attribute is the number of times that
	 *	the real entity for this entity has been offloaded. It may be useful
	 *	for debugging but is not expected to be used in production code.
	 *
	 *	This is a 16 bit unsigned value and is expected to wrap at 65535.
	 */
	PY_ATTRIBUTE( debug_numTimesRealOffloaded )

	/*~ attribute Entity isOnGround
	*  @components{ cell }
	*  The isOnGround attribute will be True if the position of the Entity is on the ground and false otherwise.
	*  @type Read-write bool
	*/
	PY_ATTRIBUTE( isOnGround )

	/*~ attribute Entity isIndoors
	*  @components{ cell }
	*  The isIndoors attribute will be True if the entity is indoors and False
	*  if outdoors or in no chunk.
	*/
	PY_ATTRIBUTE( isIndoors )

	/*~ attribute Entity isOutdoors
	*  @components{ cell }
	*  The isOutdoors attribute will be True if the entity is outdoors and False
	*  if indoors or in no chunk.
	*/
	PY_ATTRIBUTE( isOutdoors )

	/*~ attribute Entity velocity
	*  @components{ cell }
	*  The velocity attribute is the current velocity of the Entity, in world
	*  coordinates. This is calculated by the cell each time the Entity is moved,
	*  and is assumed to equal the displacement from the last move divided by the
	*  time between position updates. Accessing this throws a TypeError if the
	*  Entity is a ghost.
	*  @type Read-only tuple of 3 floats, as (x, y, z)
	*/
	PY_ATTRIBUTE( velocity )

	/*~ attribute Entity topSpeed
	*  @components{ cell }
	*  This is the top speed of the entity for physics checking purposes.  It
	*  only makes sense to set it on real entities, setting it on ghosts has no
	*  effect and will some day generate an exception.
	*
	*  If set to 0 (the default) no physics checking is done. If set to greater
	*  than zero, entity movements are checked against chunk portals, and speed
	*  is checked against the value of this property.
	*
	*  Setting this value to a negative value is undefined.
	*
	*  Additionally, if this attribute value is greater than zero and if the
	*  value of the attribute topSpeedY is also greater than zero, then this
	*  attribute's value enforces the magnitude of the velocity along the x-
	*  and z-axes only, and value of the attribute topSpeedY restricts the
	*  magnitude of the velocity along the y-axis.
	*
	*  @see topSpeedY
	*  @type float
	*/
	PY_ATTRIBUTE( topSpeed )

	/*~ attribute Entity topSpeedY
	*  @components{ cell }
	*  If the value of this attribute is greater than 0 and value of the
	*  topSpeed attribute is also greater than 0, then the topSpeed attribute
	*  applies only along the x- and z-axes, and this attribute restricts the
	*  top speed of entities along the y-axis.
	*
	*  If value of topSpeed attribute is not greater than zero, then the value
	*  of the topSpeedY attribute has no effect.
	*
	*  This attribute is only available on the real entity and reading/writing
	*  to this value on a ghost is not defined.
	*
	*  If set to zero, then the value of the attribute topSpeed applies to the
	*  magnitude of the velocity along x-, y- and z-axes.
	*
	*  @see topSpeed
	*  @type float
	*/
	PY_ATTRIBUTE( topSpeedY )
PY_END_ATTRIBUTES()

// Callbacks
/*~ callback Entity.onDestroy
 *  @components{ cell }
 *	If present, this method is called when this entity is being destroyed.
 *	Customised clean-up should be put in this callback. This method has no
 *	arguments.
 */
/*~ callback Entity.onRestore
 *  @components{ cell }
 *	If present, this method is called if this entity has been restored after the
 *	failure of a Cell application. This method has no arguments.
 */

/*~ callback Entity.onGhostCreated
 *  @components{ cell }
 *	If present, this method is called on a ghost representation of this entity
 *	as the ghost is created.
 *
 *	Note: This callback must have the decorator @bwdecorators.callableOnGhost in
 *	order to be successfully called.
 */
/*~ callback Entity.onGhostDestroyed
 *  @components{ cell }
 *	If present, this method is called on a ghost representation of this entity
 *	as the ghost is destroyed.
 *
 *	Note: This callback must have the decorator @bwdecorators.callableOnGhost in
 *	order to be successfully called.
 */

// TODO: This documentation has been removed since we do not support the
// "complex" updater. Also, it's probably would've been better to call them
// something like onUpdateResource and onUpdatedAllResources.
/*	callback Entity.onMigrate
 *  @components{ cell }
 *	If present, this method is called when this entity has been updated during
 *	the update process.
 *	@param oldEntity The previous instance of this entity.
 */
/* callback Entity.onMigratedAll
 *  @components{ cell }
 *	If present, this method is called when all entities have been updated
 *	during the update process. This method has no arguments.
 */

/*~ callback Entity.onWindowOverflow
 *	@components{ cell }
 *	If present, this method is called if this entity's channel is close to
 *	overflowing. This threshold is configured by the
 *	cellApp/sendWindowCallbackThreshold setting in bw.xml.
 */

/*~ callback Entity.onWriteToDB
 *  @components{ cell }
 *	If present, this method is called on the entity when it is about to write
 *	its data to the database.
 */
/*~ callback Entity.onNoise
 *  @components{ cell }
 *	This method is called when this entity hears a noise. See Entity.makeNoise
 *	for more information about how an entity can make noise.
 *	@param entity	The entity that made the noise.
 *	@param propRange The propagation range of the noise.
 *	@param distance This is how far away the noise was made.
 *	@param event The integer event number that was passed to Entity.makeNoise.
 *	@param info The integer info argument that was passed to Entity.makeNoise.
 */
/*~ callback Entity.onSpaceGone
 *  @components{ cell }
 *	This method is called when the space this entity is in wants to shut down.
 */
/*~ callback Entity.onGetWitness
 *  @components{ cell }
 *	This method is called when a witness attaches to this entity.
 */
/*~ callback Entity.onLoseWitness
 *  @components{ cell }
 *	This method is called when a witness detaches from this entity.
 */
/*~ callback Entity.onWitnessed
 *  @components{ cell }
 *	This method is called when the state of this entity being witnessed changes.
 *	That is, if there are no attached clients who have this entity in their Area
 *	of Interest, this entity's witnessed state will be False. If a client is
 *	interested in this entity, its witnessed state will be True.
 *
 *	When entities are created, their initial witness state is unwitnessed. If
 *	they are created within the AoI of a witness, then they will have
 *	Entity.onWitnessed( True ) called as soon as the witness is notified of
 *	this entity.
 *
 *	This can be useful as an optimisation. For example, CPU intensive entities
 *	may be able to pause their AI while they are not being watched.
 *
 *	@see Entity.isWitnessed
 *
 *	@param isWitnessed A boolean indicating whether or not the entity is now
 *	witnessed.
 */
/*~	callback Entity.onLoseControlledBy
 *  @components{ cell }
 *	This method is called when the entity associated with the controlledBy
 *	property is destroyed.
 *	@param id The id of the controlledBy entity.
 */
/*~ callback Entity.onEnteringCell
 *  @components{ cell }
 *	This method is called on a ghost entity when it is about to become real.
 *
 *	On the source cell, onLeavingCell is called on the real before it is
 *	offloaded. onLeftCell is then called on this entity after being converted
 *	to a ghost.
 *
 *	On the destination cell, onEnteringCell is called on the ghost entity. This
 *	ghost is then converted to a real entity and onEnteredCell is called.
 *
 *	This method should be decorated with @bwdecorators.callableOnGhost since
 *	it will only be called on ghost entities.
 *
 *	Note: Think twice before using this callback. These can often be avoided by
 *	using smarter data types.
 *
 *	@see Entity.onEnteredCell
 *	@see Entity.onLeavingCell
 *	@see Entity.onLeftCell
 */
/*~ callback Entity.onEnteredCell
 *  @components{ cell }
 *	This method is called on a real entity when it has just entered a new cell.
 *
 *	On the source cell, onLeavingCell is called on the real before it is
 *	offloaded.
 *	onLeftCell is then called on this entity after being converted to a ghost.
 *
 *	On the destination cell, onEnteringCell is called on the ghost entity. This
 *	ghost is then converted to a real entity and onEnteredCell is called.
 *
 *	Note: Think twice before using this callback. These can often be avoided by
 *	using smarter data types.
 *
 *	@see Entity.onEnteringCell
 *	@see Entity.onLeavingCell
 *	@see Entity.onLeftCell
 */
/*~ callback Entity.onLeavingCell
 *  @components{ cell }
 *	This method is called on a real entity when it is about to move to a new
 *	cell.
 *
 *	On the source cell, onLeavingCell is called on the real before it is
 *	offloaded. onLeftCell is then called on this entity after being converted
 *	to a ghost.
 *
 *	On the destination cell, onEnteringCell is called on the ghost entity. This
 *	ghost is then converted to a real entity and onEnteredCell is called.
 *
 *	Note: Think twice before using this callback. These can often be avoided by
 *	using smarter data types.
 *
 *	@see Entity.onEnteringCell
 *	@see Entity.onEnteredCell
 *	@see Entity.onLeftCell
 */
/*~ callback Entity.onLeftCell
 *  @components{ cell }
 *	This method is called on a ghost entity when the real entity has just left
 *	this cell.
 *
 *	On the source cell, onLeavingCell is called on the real before it is
 *	offloaded. onLeftCell is then called on this entity after being converted
 *	to a ghost.
 *
 *	On the destination cell, onEnteringCell is called on the ghost entity. This
 *	ghost is then converted to a real entity and onEnteredCell is called.
 *
 *	This method should be decorated with @bwdecorators.callableOnGhost since
 *	it will only be called on ghost entities.
 *
 *	Note: Think twice before using this callback. These can often be avoided by
 *	using smarter data types.
 *
 *	@see Entity.onEnteringCell
 *	@see Entity.onEnteredCell
 *	@see Entity.onLeavingCell
 */
/*~ callback Entity.onTeleport
 *  @components{ cell }
 *	This method is called on a real entity just before the entity is teleported
 *	via a Base.teleport call. Note that this method is not called when the
 *	entity is teleported via the cell's Entity.teleport method. You should
 *	explicit call this method if you need this functionality.
 *
 *	@see Entity.teleport
 */
/*~ callback Entity.onTeleportSuccess
 *  @components{ cell }
 *	This method is called on a real entity when a teleport() call for that
 *	entity has succeeded. The nearby entity is passed as the only argument.
 *
 *	When the teleport has been caused by a call from the base entity, the
 *	position of the entity will be BigWorld.INVALID_POSITION. The desired
 *	position should be set in this callback. This will typically be the
 *	position of the nearby entity or a close offset.
 *
 *	The nearby entity argument is guaranteed to be a real entity. Two-way
 *	methods that modify state can be called on this argument. This can be useful
 *	if the nearby entity contains state for where the entity should be
 *	positioned.
 *
 *	@param nearbyEntity The entity associated with the mailbox passed to
 *		Entity.teleport or Base.teleport. This is guaranteed to be a real
 *		entity.
 *
 *	@see Entity.teleport
 */
/*~ callback Entity.onTeleportFailure
 *  @components{ cell }
 *	This method is called on a real entity when a teleport() call for that
 *	entity fails. This can occur if the nearby entity mailbox passed into
 *	teleport() is stale, meaning that the entity that it points to no longer
 *	exists on the destination CellApp pointed to by the mailbox.
 *
 *	@see Entity.teleport
 */
/*~ callback Entity.onEnteredAoI
 *  @components{ cell }
 *	This method is called on a real entity with witness whenever an entity
 *	enters its Area of Interest.
 *
 *	Note: Any persistent entity tracking performed as a result of this callback
 *	must be cleared in Entity.onRestore as the AoI is not restored during a
 *	CellApp failure.
 *
 *	@see Entity.onLeftAoI
 *	@see Entity.onLeftAoIID
 *
 *	@param entity	The entity that entered the AoI.
 */
/*~ callback Entity.onLeftAoI
 *  @components{ cell }
 *	This method is called on a real entity with witness whenever an entity
 *	leaves its Area of Interest. Entity.onLeftAoIID should also be implemented
 *	in order to handle offload scenarios where the leaving entity may not be
 *	able to be directly referenced.
 *
 *	NB: It is possible that the entity passed into this callback may already be
 *	destroyed, ie: entity.isDestroyed is True
 *
 *	@see Entity.onEnteredAoI
 *	@see Entity.onLeftAoIID
 *
 *	@param entity	The entity that left the AoI.
 */
/*~ callback Entity.onLeftAoIID
 *  @components{ cell }
 *	This method is called on a real entity with witness when an entity
 *	leaves its Area of Interest where the leaving entity is not able to be
 *	directly referenced, e.g., during entity offload.
 *
 *	@see Entity.onEnteredAoI
 *	@see Entity.onLeftAoI
 *
 *	@param entityID	The ID of the entity that left the AoI.
 */
/*~ callback Entity.__init__
 *	@components{ cell }
 *	This method is called when the real entity is initially created. It is not
 *	called on ghost entities or when the real entity is offloaded.
 *
 *	@param nearbyEntity This optional argument accepts the entity that was used
 *		in the Base.createCellEntity call. This is useful for setting the
 *		initial position of the entity. This argument is guaranteed to be a
 *		real entity.
 *
 *	@see Entity.onTeleportSuccess
 */

EntityPopulation Entity::population_;

namespace
{

/**
 *	Prints out a commonly used performance warning.
 */
void outputEntityPerfWarning( const char * message, const Entity& entity,
		double duration, int size )
{
	WARNING_MSG( "%s (%s %u of size %d bytes took %f seconds)\n",
			message, entity.pType()->description().name().c_str(), entity.id(),
			size, duration );
}

class PyCellComponent : public PyComponent
{
public:
	PyCellComponent( const IEntityDelegatePtr & delegate,
			EntityID entityID,
			const BW::string & componentName,
			const EntityDescription * pEntityDescription) :
		PyComponent( delegate, entityID, componentName ),
		pEntityDescription_( pEntityDescription )
	{
	}
private:
	virtual DataDescription * findProperty( const char * attribute,
											const char * component )
	{
		DataDescription * pDescription = 
			pEntityDescription_->findProperty( attribute, component );

		if (pDescription && !pDescription->isCellData())
		{
			pDescription = NULL;
		}
		return pDescription;
	}

	const EntityDescription * pEntityDescription_;
};

} // anonymous namespace

void Entity::s_init()
{
	g_publicClientStats.init( "eventProfiles/publicClientEvents" );
	g_totalPublicClientStats.init( "eventProfiles/totalPublicClientEvents" );
	g_privateClientStats.init( "eventProfiles/privateClientEvents" );
}

uint16 Entity::s_physicsCorrectionsOutstandingWarningLevel = 256;

// -----------------------------------------------------------------------------
// Section: EntityExtraInfo
// -----------------------------------------------------------------------------

/**
 *	This class is used to store information for an EntityExtra type.
 */
class EntityExtraInfo
{
public:
	PyMethodDef * pMethods_;
	PyGetSetDef * pAttributes_;
};

// -----------------------------------------------------------------------------
// Section: BuildEntityTypeDict
// -----------------------------------------------------------------------------

/**
 *	This class combines the entity class's extra methods and members into its
 *	vector at after Script::init
 */
class BuildEntityTypeDict : public Script::InitTimeJob
{
public:
	BuildEntityTypeDict() :
		Script::InitTimeJob( 1 )
	{}

	virtual void init()
	{
		this->addMethods( Entity::s_getMethodDefs() );
		this->addAttributes( Entity::s_getAttributeDefs(), /* mainMethods: */ true );

		// Add entity extra
		BW::vector< EntityExtraInfo * > & entityExtra
			= Entity::s_entityExtraInfo();
		for (BW::vector< EntityExtraInfo * >::iterator it = entityExtra.begin();
				it < entityExtra.end(); ++it)
		{
			EntityExtraInfo * eei = *it;
			if (eei != NULL)
			{
				this->addMethods( eei->pMethods_ );
				this->addAttributes( eei->pAttributes_ );
			}
		}

		// Add real
		this->addMethods( RealEntity::s_getMethodDefs() );
		this->addAttributes( RealEntity::s_getAttributeDefs() );

		// Witness
		this->addMethods( Witness::s_getMethodDefs() );
		this->addAttributes( Witness::s_getAttributeDefs() );

		// Add Sentinels
		PyMethodDef nullMethod = { NULL, NULL, 0, NULL };
		methods_.push_back( nullMethod );

		PyGetSetDef nullMember = { NULL, NULL, NULL, NULL, NULL };
		members_.push_back( nullMember );

		Entity::s_type_.tp_methods = &methods_[0];
		Entity::s_type_.tp_getset = &members_[0];
	}

	const StringSet<ConstPolicy> & attributeSet()
	{
		return attributeSet_;
	}

private:
	void addMethods( PyMethodDef * pMethods )
	{
		while (pMethods && pMethods->ml_name != NULL)
		{
			methods_.push_back( *pMethods );
			attributeSet_.insert( pMethods->ml_name );
			++pMethods;
		}
	}

	void addAttributes( PyGetSetDef * pMembers, bool mainMethods = false )
	{
		while (pMembers && pMembers->name != NULL )
		{
			if (mainMethods ||
				(strcmp( pMembers->name, "__members__" ) != 0 &&
				 strcmp( pMembers->name, "__methods__" ) != 0))
			{
				members_.push_back( *pMembers );
				attributeSet_.insert( pMembers->name );
			}

			++pMembers;
		}
	}

	BW::vector<PyMethodDef> methods_;
	BW::vector<PyGetSetDef> members_;
	StringSet<ConstPolicy> attributeSet_;
};

BuildEntityTypeDict s_buildEntityDictInitTimeJob;

// -----------------------------------------------------------------------------
// Section: Entity
// -----------------------------------------------------------------------------

/**
 *	The constructor for Entity. Either the initReal or initGhost method should
 *	be called immediately after an entity is constructed.
 *
 *	@see initReal
 *	@see initGhost
 */
Entity::Entity( EntityTypePtr pEntityType ):
	PyObjectPlus( pEntityType->pPyType(), true ),
	// pSpace_( pSpace ),
	removalHandle_( NO_SPACE_REMOVAL_HANDLE ),
	id_( 0 ),
	pEntityType_( pEntityType ),
	// globalPosition_(),
	// globalDirection_(),
	// localPosition_(),
	// localDirection_(),
	baseAddr_( Mercury::Address::NONE ),
	// pVehicle_( NULL ),
	// vehicleChangeNum_( 0 ),
	// aoiUpdateSchemeID_( 0 ),
	// numTimesRealOffloaded_( 0 ),
	// pRealChannel_( NULL ),
	// pReal_( NULL ),
	// properties_(),
	propertyOwner_( *this ),
	eventHistory_( pEntityType->description().numLatestChangeOnlyMembers() ),
	isDestroyed_( true ), // cleared in setToInitialState
	// inDestroy_( false ),
	// isInAoIOffload_( false ),
	// pVehicle_( NULL ),
	// volatileUpdateNumber_( 0 ),
	// isOnGround_( false ),
	// lastEventNumber_( 0 ),
	pRangeListNode_( NULL ),
	pRangeListAppealTrigger_( NULL ),
	pControllers_( NULL ),
	shouldReturnID_( false ),
	extras_( NULL )
	// pChunk_( NULL ),
	// pPrevInChunk_( NULL ),
	// pNextInChunk_( NULL )
{
	++g_numEntitiesEver;

	pRangeListNode_ = new EntityRangeListNode( this );

	const float appealRadius = pEntityType->description().appealRadius();
	const bool hasAoIAppeal = !isZero( appealRadius );

	if (hasAoIAppeal)
	{
		pRangeListAppealTrigger_ =
			new RangeListAppealTrigger( pRangeListNode_, appealRadius );
	}

	// this->setToInitialState( pSpace );

	// population_.add( *this );
}


/**
 *	This method sets this entity to its initial state. This is the state it
 *	should be in before initReal or initGhost is called.
 */
void Entity::setToInitialState( EntityID id, Space * pSpace )
{
	// PyObjectPlus(),
	MF_ASSERT( pEntityDelegate_ == NULL );
	pSpace_ = pSpace;
	MF_ASSERT( removalHandle_ == NO_SPACE_REMOVAL_HANDLE );
	// globalPosition_ = Vector3();
	// globalDirection_;
	// localPosition_ = Vector3();
	// localDirection_;
	baseAddr_ = Mercury::Address::NONE;
	pVehicle_ = NULL;
	vehicleChangeNum_ = 0;
	aoiUpdateSchemeID_ = 0;
	numTimesRealOffloaded_ = 0;
	pRealChannel_ = NULL;
	nextRealAddr_ = Mercury::Address::NONE;
	IF_NOT_MF_ASSERT_DEV( !this->isReal() )
	{
		this->destroyReal();
		MF_ASSERT( !this->isReal() );
	}
	MF_ASSERT( properties_.empty() );
	properties_.assign( pEntityType_->propCountGhost(), ScriptObject() );
	eventHistory_.clear();

	MF_ASSERT( isDestroyed_ )
	isDestroyed_ = false;
	++g_numEntities;

	inDestroy_ = false;
	isInAoIOffload_ = false;
	isOnGround_ = false;
	volatileUpdateNumber_ = 0;
	topSpeed_ = 0.f;
	topSpeedY_ = 0.f;
	physicsCorrections_ = 0;
	physicsLastValidated_ = timestamp();
	physicsNetworkJitterDebt_ = 0.f;
	lastEventNumber_ = 0;
	// pRangeListNode_ = NULL;
	// pRangeListAppealTrigger_ = NULL;

	delete pControllers_;
	pControllers_ = new Controllers;

	if (extras_ == NULL)
	{
		extras_ = new EntityExtra*[ s_entityExtraInfo().size() ];
		for (uint i = 0; i < s_entityExtraInfo().size(); i++)
			extras_[i] = NULL;
	}
	else
	{
		for (uint i = 0; i < s_entityExtraInfo().size(); i++)
		{
			delete extras_[i];
			extras_[i] = NULL;
		}
	}
	pChunk_ = NULL;
	pPrevInChunk_ = NULL;
	pNextInChunk_ = NULL;

	periodsWithoutWitness_ = NOT_WITNESSED_THRESHOLD;

	MF_ASSERT( id_ == 0 || id_ == id );

	if (id_ != id)
	{
		id_ = id;
		population_.add( *this );
	}
}


/**
 *	Destructor
 */
Entity::~Entity()
{
	MF_ASSERT( this->isDestroyed() );
	MF_ASSERT( pChunk_ == NULL );
	MF_ASSERT( properties_.empty() );

	// The real part of this entity should have already been destroyed in
	// Entity::destroy().
	MF_ASSERT( !this->isReal() );

	// Get rid of any entity extras (for the second time)
	for (uint i = 0; i < s_entityExtraInfo().size(); i++)
	{
		delete extras_[i];
		extras_[i] = NULL;
	}
#ifdef _DEBUG
	// Make sure none came back through self reference
	for (uint i = 0; i < s_entityExtraInfo().size(); i++)
	{
		MF_ASSERT( extras_[i] == NULL );
	}
#endif
	delete [] extras_;

	population_.remove( *this );

	// since we are out of all range lists no one should have triggers on us!
	MF_ASSERT( triggers_.empty() );

	bw_safe_delete( pRangeListNode_ );
	bw_safe_delete( pRangeListAppealTrigger_ );
	bw_safe_delete( pControllers_ );

	if (shouldReturnID_)
	{
		// we didn't have a base, and we were the real that got
		// destroyed, so we now return our id
		CellApp::instance().idClient().putUsedID( this->id() );
	}
}

// #define DEBUG_FAULT_TOLERANCE
#ifdef DEBUG_FAULT_TOLERANCE
extern bool g_crashOnOnload;
extern bool g_crashOnOffload;
#endif

namespace
{
Entity * s_pTreatAsReal = NULL;
Entity * s_pRealEntityArg = NULL;
}


/**
 *	This static method initialises watchers associated with entities.
 */
void Entity::addWatchers()
{
#ifdef DEBUG_FAULT_TOLERANCE
	// These are handy for testing whether the fault tolerance can handle
	// restoring an entity while an offload or onload is in progress.
	MF_WATCH( "debugging/crashOnOnload", g_crashOnOnload );
	MF_WATCH( "debugging/crashOnOffload", g_crashOnOffload );
#endif

	MF_WATCH( "config/physicsCorrectionsOutstandingWarningLevel",
		Entity::s_physicsCorrectionsOutstandingWarningLevel,
		Watcher::WT_READ_WRITE,
		"The number of outstanding physicsCorrections from an entity "
		"to a client at which to log a warning. Set to 0 to disable." );

	if (CellAppConfig::treatAllOtherEntitiesAsGhosts() &&
		CellAppConfig::isProduction())
	{
		INFO_MSG( "Production Mode: Treating other entities as ghosts enabled. "
				"This may reduce your performance.\n" );
	}

	MF_WATCH( "stats/numEntities", g_numEntities, Watcher::WT_READ_ONLY,
			"The number of entities on this CellApp." );
	MF_WATCH( "stats/numEntitiesLeaked", numEntitiesLeaked,
			"The number of entities that have been destroyed but are still "
			"referenced." );

	MF_WATCH( "stats/totalEntitiesEver", g_numEntitiesEver );

	MF_WATCH( "stats/nonVolatileBytes", g_nonVolatileBytes );
	MF_WATCH( "stats/volatileBytes", g_volatileBytes );

	RealEntity::addWatchers();
}


/**
 *	This method should be called on a newly created entity to make it a real
 *	entity. Either this method or initGhost should be called immediately after
 *	the constructor.
 *
 *	@see initGhost
 */
bool Entity::initReal( BinaryIStream & data, const ScriptDict & properties,
		bool isRestore,
		Mercury::ChannelVersion channelVersion,
		EntityPtr pNearbyEntity )
{
	static ProfileVal localProfile( "initReal" );
	SCOPED_PROFILE( TRANSIENT_LOAD_PROFILE );
	START_PROFILE( localProfile );

	int dataSize = data.remainingLength();

	if (!this->readRealDataInEntityFromStreamForInitOrRestore( data,
		properties ))
	{
		// Forget the rest of the creation data, it obviously failed
		data.finish();
		// Fix state that comes from Entity::setToInitialState, but for
		// which our destructor depends on Entity::destroy to clean up
		this->setDestroyed();
		this->clearPythonProperties();
		STOP_PROFILE( localProfile );
		return false;
	}

	// Now called in Cell::createEntityInternal
	// Entity::callbacksPermitted( false );	//{
	MF_ASSERT( !Entity::callbacksPermitted() );

	// add it to the space
	pSpace_->addEntity( this );

	if (!isRestore)
	{
		volatileInfo_ = pEntityType_->volatileInfo();
	}
	else
	{
		data >> volatileInfo_;
	}

	this->createReal();

	bool shouldUpdateGhostPositions = pReal_->init( data,
			isRestore ? CREATE_REAL_FROM_RESTORE : CREATE_REAL_FROM_INIT,
			channelVersion );

	MF_ASSERT( isRestore != shouldUpdateGhostPositions );

	// Read any BASE_AND_CLIENT data exposed for recording.
	if (!this->readBasePropertiesExposedForReplayFromStream( data ))
	{
		ERROR_MSG( "Entity::readRealDataInEntityFromStreamForInitOrRestore"
				"(%u): Error streaming off exposed base entity "
				"properties\n", id_ );
		return false;
	}

	this->updateGlobalPosition( shouldUpdateGhostPositions );

	if (isRestore)
	{
		this->startRealControllers();
	}

	// add us to the right chunk's list
	if (pSpace_ != NULL && pChunk_ != NULL)
	{
		CellChunk::instance( *pChunk_ ).removeEntity( this );
	}
	MF_ASSERT_DEBUG( this->pChunk() == NULL );
	MF_ASSERT_DEBUG( this->nextInChunk() == NULL );
	MF_ASSERT_DEBUG( this->prevInChunk() == NULL );

	this->checkChunkCrossing();

	// We don't want to stop callbacks here but we also do not yet want to
	// replay any queued callback yet. This is a bit of a risk because some
	// callbacks could come in in an unexpected order.
	s_callbackBuffer_.enableHighPriorityBuffering();

	// finally call __init__ (which could change position,
	// change spaces, offload or even destroy us) ...
	// our caller is keeping a ref so that we don't get destructed
	this->callScriptInit( isRestore, pNearbyEntity );

	s_callbackBuffer_.disableHighPriorityBuffering();

	// release any queued callbacks
	// Entity::callbacksPermitted( true );		//}

	STOP_PROFILE( localProfile );

	uint64 duration = localProfile.lastTime();

	if (duration > g_profileInitRealTimeLevel)
	{
		outputEntityPerfWarning( "Profile initReal/timeWarningLevel exceeded",
				*this, stampsToSeconds( duration ), dataSize );
	}
	else if (dataSize > g_profileInitRealSizeLevel)
	{
		outputEntityPerfWarning( "Profile initReal/sizeWarningLevel exceeded",
				*this, stampsToSeconds( duration ), dataSize );
	}

	return true;
}

/**
 *	Read the real data stored in an Entity from the stream. The data is put
 *	on either by the base when creating its cell part, or by ourselves when
 *	creating a cell-only entity. Other external processes like eload might
 *	also stream on this data. The point is, change it and you need to update
 *	lots of places! :)
 */
bool Entity::readRealDataInEntityFromStreamForInitOrRestore(
	BinaryIStream & data, const ScriptDict & properties )
{
	uint8 flags;
	// This is the data that is streamed on by StreamHelper::addEntityData
	data >> localPosition_ >> flags >> localDirection_ >>
		lastEventNumber_ >> baseAddr_;

	isOnGround_ = (flags & StreamHelper::AddEntityData::isOnGroundFlag);
	bool isTemplate = (flags & StreamHelper::AddEntityData::hasTemplateFlag);

	// DEBUG_MSG( "Entity::initReal: entityTypeID_ = %d\n", entityTypeID_ );

	const EntityDescription & description = pEntityType_->description();

	// Initialise the structure that stores the time-stamps for when
	// clientServer properties were last changed.
	// TODO: For now, setting all properties to the lastEventNumber_. In the
	// future, these may be stored in the database and streamed off.
	propertyEventStamps_.init( description, lastEventNumber_ );

	// Properties can come from only one of three sources:
	// A ScriptDict, which has come from Python script locally.
	// A template identified by a BW::string, either locally or from a BaseApp.
	// An EntityDescription::CELL_DATA stream from a BaseApp.

	ScriptDict propDict;
	if (isTemplate)
	{
		// We don't support templates with an extra properties dictionary
		MF_ASSERT( !properties );

		BW::string templateID;
		data >> templateID;

		// The template may override localPosition_, isOnGround_ and localDirection_
		this->createEntityDelegateWithTemplate( templateID );

		// TODO: Support TemplateID creation without an IEntityDelegate. We need
		// a way to identify a specific Entity in a ChunkSpace... GUID perhaps.
		if (!pEntityDelegate_)
		{
			ERROR_MSG(
				"Entity::readRealDataInEntityFromStreamForInitOrRestore(%u): "
					"Trying to create an Entity of type %s by template %s but "
					"delegate cannot handle that type or template.\n",
				id_, pEntityType_->name(), templateID.c_str() );
			return false;
		}

		// Empty dictionary for properties_ population below
		propDict = ScriptDict::create();
	}
	else
	{
		this->createEntityDelegate();

		if (properties)
		{
			propDict = properties;
		}
		else
		{
			propDict = ScriptDict::create();
			// If properties are provided via stream, get a dictionary of them.
			// TODO: Optimise by streaming directly into properties_ and/or the
			// delegate.

			// Written by one of:
			//	Base::addCellCreationData
			//	RealEntity::writeBackupProperties
			// if we have a stream then put it into a temp dict
			if (!description.readStreamToDict( data,
						EntityDescription::CELL_DATA, propDict ))
			{
				ERROR_MSG(
					"Entity::readRealDataInEntityFromStreamForInitOrRestore(%u): "
						"Error streaming off entity properties\n", id_ );
				return false;
			}
		}

		if (!populateDelegateWithDict( description, pEntityDelegate_.get(),
			propDict, EntityDescription::CELL_DATA ))
		{
			ERROR_MSG(
				"Entity::readRealDataInEntityFromStreamForInitOrRestore(%u): "
					"Failed to populate the delegate of an Entity of type %s\n",
				id_, pEntityType_->name() );
			return false;
		}
	}

	globalPosition_ = INVALID_POSITION;
	globalDirection_ = localDirection_;

	// Resize properties_ to the correct size
	properties_.insert( properties_.end(),
		pEntityType_->propCountGhostPlusReal()-properties_.size(),
		ScriptObject() );

	// Now set properties_
	for (uint i = 0; i < properties_.size(); ++i)
	{
		DataDescription & ds = *pEntityType_->propIndex( i );
		DataType & dt = *ds.dataType();

		// first detach in case we are restoring
		if (properties_[i])
		{
			dt.detach( properties_[i] );
			properties_[i] = NULL;	// necessary; see below
		}
		// now attach the new one
		ScriptObject pNewVal = ds.getItemFrom( propDict );
		if (!pNewVal ||
				!(properties_[i] = dt.attach( pNewVal,
						&propertyOwner_, i )))
		{	// prop currently null so assignment won't exec python so
			PyErr_Clear();	// it's ok to delay clearing err 'till here

			if (pNewVal)
			{
				ERROR_MSG(
					"Entity::readRealDataInEntityFromStreamForInitOrRestore(%u): "
						"Trying to set %s.%s to invalid value\n",
					id_, pEntityType_->name(), ds.name().c_str() );
			}

			properties_[i] =
					dt.attach( ds.pInitialValue(), &propertyOwner_, i );
			MF_ASSERT( properties_[i] );
		}
	}

	return true;
}


/**
 *	This method should be called on a newly created entity to make it a ghost
 *	entity. Either this method or initReal should be called immediately after
 *	the constructor.
 *
 *	@see initReal
 */
void Entity::initGhost( BinaryIStream & data )
{
	static ProfileVal localProfile( "initGhost" );
	START_PROFILE( localProfile );

	int dataSize = data.remainingLength();

	this->createEntityDelegate();

	this->readGhostDataFromStream( data );

	// TODO: Consider putting this above readGhostDataFromStream.
	// In case any of the ghostControllers or entity extras cause
	// a script method to be called back on some entity.
	// (obv. a different one to this one which is just a ghost).
	// e.g. if a ghost controller added it to some plane of entities
	// that others got script notifications of (like with a proximity
	// controller) and then they couldn't find this entity as it was
	// not yet in the space ... anyway, yeah, worth thinking about,
	// but not for this commit.
	Entity::callbacksPermitted( false );	//{

	pSpace_->addEntity( this );
	this->onPositionChanged();

	// make sure the chunk is NULL (controller could have set it already)
	if (pChunk_ != NULL)
		CellChunk::instance( *pChunk_ ).removeEntity( this );

	// find which chunk we are in
	pChunk_ = this->pChunkSpace()->findChunkFromPointExact(
		globalPosition_ + Vector3(0.f, 0.1f, 0.f) );

	// add us to its list
	if (pChunk_ != NULL)
		CellChunk::instance( *pChunk_ ).addEntity( this );

	Entity::callbacksPermitted( true );		//}

	STOP_PROFILE( localProfile );

	uint64 duration = localProfile.lastTime();

	if (duration > g_profileInitGhostTimeLevel)
	{
		outputEntityPerfWarning( "Profile initGhost/timeWarningLevel exceeded",
				*this, stampsToSeconds( duration ), dataSize );
	}
	else if (dataSize > g_profileInitGhostSizeLevel)
	{
		outputEntityPerfWarning( "Profile initGhost/sizeWarningLevel exceeded",
				*this, stampsToSeconds( duration ), dataSize );
	}

	this->callback( "onGhostCreated" );
}


/**
 *	This method reads ghost data from the input stream. This matches the data
 *	that was added by writeGhostDataToStream. It is called by initGhost.
 *
 *	@see writeGhostDataToStream
 *	@see initGhost
 */
void Entity::readGhostDataFromStream( BinaryIStream & data )
{
	CompressionIStream compressionStream( data );
	this->readGhostDataFromStreamInternal( compressionStream );
}


/**
 *	This method is called by readGhostDataFromStream once the decision on
 *	whether or not to uncompress has been made.
 */
void Entity::readGhostDataFromStreamInternal( BinaryIStream & data )
{
	// This was streamed on by Entity::writeGhostDataToStream.
	data >> numTimesRealOffloaded_ >> localPosition_ >> isOnGround_ >>
		lastEventNumber_ >> volatileInfo_;

	eventHistory_.lastTrimmedEventNumber( lastEventNumber_ );

	globalPosition_ = localPosition_;

	// Initialise the structure that stores the time-stamps for when
	// clientServer properties were last changed.
	propertyEventStamps_.init( pEntityType_->description() );

	Mercury::Address realAddr;
	data >> realAddr;
	pRealChannel_ = CellAppChannels::instance().get( realAddr );

	data >> baseAddr_;
	data >> localDirection_;
	globalDirection_ = localDirection_;

	propertyEventStamps_.removeFromStream( data );

	TOKEN_CHECK( data, "GProperties" );

	// Read in the ghost properties
	MF_ASSERT( properties_.size() == pEntityType_->propCountGhost() );
	for (uint32 i = 0; i < properties_.size(); ++i)
	{
		DataDescription & dataDescr = *pEntityType_->propIndex( i );

		// TODO - implement component properties processing here
		MF_ASSERT( !dataDescr.isComponentised() );

		DataType & dt = *dataDescr.dataType();
		// read and attach the property
		ScriptDataSink sink;
		MF_VERIFY( dt.createFromStream( data, sink,
			/* isPersistentOnly */ false ) );
		ScriptObject value = sink.finalise();
		if (!(properties_[i] = dt.attach( value, &propertyOwner_, i )))
		{
			CRITICAL_MSG( "Entity::initGhost(%u):"
				"Error streaming off entity property %u\n", id_, i );
		}
	}

	TOKEN_CHECK( data, "GController" );

	// Finally get controllers
	this->readGhostControllersFromStream( data );

	TOKEN_CHECK( data, "GTail" );
	data >> periodsWithoutWitness_ >> aoiUpdateSchemeID_;

	MF_ASSERT( data.remainingLength() == 0 );
	MF_ASSERT( !data.error() );
}


/**
 *	This method puts the variable state data onto the stream that initGhost
 *	expects to take off.
 *
 *	@param stream		The stream to put the data on.
 */
void Entity::writeGhostDataToStream( BinaryOStream & stream ) const
{
	// Note: The id and entityTypeID is not read off by readGhostDataFromStream.
	// They are read by Space::createGhost
	// Note: Also read by BufferGhostMessage to get numTimesRealOffloaded_.
	stream << id_ << this->entityTypeID();

	CompressionOStream compressionStream( stream,
			pEntityType_->description().internalNetworkCompressionType() );
	this->writeGhostDataToStreamInternal( compressionStream );
}


/**
 *	This method is called by writeGhostDataToStream once the decision on
 *	whether or not to compress has been made.
 */
void Entity::writeGhostDataToStreamInternal( BinaryOStream & stream ) const
{
	stream << numTimesRealOffloaded_ << localPosition_ << isOnGround_ <<
		lastEventNumber_ << volatileInfo_;

	stream << CellApp::instance().interface().address();
	stream << baseAddr_;
	stream << localDirection_;

	propertyEventStamps_.addToStream( stream );

	TOKEN_ADD( stream, "GProperties" );

	// Do ghosted properties dependent on entity type
	//this->pType()->addDataToStream( this, stream, DATA_GHOSTED );

	// write our ghost properties to the stream
	for (uint32 i = 0; i < pEntityType_->propCountGhost(); ++i)
	{
		MF_ASSERT( properties_[i] );

		DataDescription * pDataDesc = pEntityType_->propIndex( i );

		// TODO - implement component properties processing here
		MF_ASSERT( !pDataDesc->isComponentised() );

		ScriptDataSource source( properties_[i] );
		if (!pDataDesc->addToStream( source, stream, false ))
		{
			CRITICAL_MSG( "Entity::writeGhostDataToStream(%u): "
					"Could not write ghost property %s.%s to stream\n",
				id_, this->pType()->name(), pDataDesc->name().c_str() );
		}
	}
	TOKEN_ADD( stream, "GController" );

	this->writeGhostControllersToStream( stream );

	TOKEN_ADD( stream, "GTail" );
	stream << periodsWithoutWitness_ << aoiUpdateSchemeID_;
}


/**
 *	This method reads ghost controllers from the input stream. This matches the
 *	data that was added by writeGhostDataToStream. It is called by initGhost.
 *
 *	@see writeGhostDataToStream
 */
void Entity::readGhostControllersFromStream( BinaryIStream & data )
{
	pControllers_->readGhostsFromStream( data, this );
}


/**
 *	This method reads real controllers from the input stream. This matches the
 *	data that was added by writeRealControllersToStream.
 *
 *	@see writeRealControllersToStream
 */
void Entity::readRealControllersFromStream( BinaryIStream & data )
{
	pControllers_->readRealsFromStream( data, this );
}


/**
 *	This method calls the __init__ method on the script object.
 */
void Entity::callScriptInit( bool isRestore, EntityPtr pNearbyEntity )
{
	if (isRestore)
	{
		this->callback( "onRestore" );
	}
	else if (!pEntityType_->expectsNearbyEntity())
	{
		this->callback( "__init__" );
	}
	else
	{
		MF_ASSERT( s_pRealEntityArg == NULL );
		s_pRealEntityArg = pNearbyEntity.get();
		this->callback( "__init__",
				Py_BuildValue( "(O)",
					pNearbyEntity ? pNearbyEntity.get() : Py_None ),
					"__init__", true );
		s_pRealEntityArg = NULL;
	}

	//TODO: this is not correct place to call this method, because if
	//the position would be changed during this call, the server will crash.
	//The correct way of doing it will be to queue this call until after the
	//callbacks are re-enabled; or to implement/override the python's 
	//__init__ equivalent in c++, and to call it from there
	// (see the draft proposition below in the commented-out method DelegateEntityInit,
	if (pEntityDelegate_)
	{
		pEntityDelegate_->onEntityInitialised();
	}
	//serp
}


/**
 *	This method checks if the entity has been offloaded to an destination but
 *	has yet to receive the corresponding onload confirmation.
 *
 *	@param addr	The destination address to check.
 *
 * 	@return	True if offloading to the destination, false otherwise.
 *
 *	@see convertRealToGhost
 *	@see ghostSetReal
 */
bool Entity::isOffloadingTo( const Mercury::Address & addr ) const
{
	return this->realAddr() == nextRealAddr_ && nextRealAddr_ == addr;
}


/**
 *	This method offloads this real entity to the input adjacent cell. It should
 *	only be called on a real entity. This entity is converted into a ghost
 *	entity.
 *
 *	@param pChannel	The channel to the application to move to.
 *	@param isTeleport Indicates whether this is a teleport.
 *
 *	@see onload
 */
void Entity::offload( CellAppChannel * pChannel, bool isTeleport )
{
#ifdef DEBUG_FAULT_TOLERANCE
	if (g_crashOnOffload)
	{
		MF_ASSERT( !"Entity::offload: Crash on offload" );
	}
#endif

	MF_ASSERT( this->isReal() );

	Mercury::Bundle & bundle = pChannel->bundle();

	// if we are teleporting then we already have a message on the bundle
	if (!isTeleport)
	{
		bundle.startMessage( CellAppInterface::onload );
	}

	this->convertRealToGhost( &bundle, pChannel, isTeleport );

//	DEBUG_MSG( "Entity::offload( %d ): Offloading to %s\n",
//		id_, pChannel->addr().c_str() );
}


/**
 *	This method converts a real entity into a ghost entity.
 *
 *	@param pStream	The stream to write the conversion data to.
 *	@param pChannel	The application the real entity is being moved to. If
 *						NULL, the real entity is being destroyed.
 *	@param isTeleport Indicates whether this is a teleport.
 *
 *	@see offload
 *	@see destroy
 */
void Entity::convertRealToGhost( BinaryOStream * pStream,
		CellAppChannel * pChannel, bool isTeleport )
{
	MF_ASSERT( this->isReal() );
	MF_ASSERT( !pRealChannel_ );

	Entity::callbacksPermitted( false );

	Witness * pWitness = this->pReal()->pWitness();
	if (pWitness != NULL)
	{
		pWitness->flushToClient();
	}

	if (pChannel != NULL)
	{
		// Offload the entity if we have a pChannel to the next real.
		MF_ASSERT( pStream != NULL );

		this->writeRealDataToStream( *pStream, isTeleport );

		pRealChannel_ = pChannel;

		// Once the real is created on the other CellApp, it will send a
		// ghostSetReal back to this app, so we better be ready for it.
		nextRealAddr_ = pRealChannel_->addr();

		// Delete the real part (includes decrementing refs of haunts
		// and notifying haunts of our nextRealAddr_)
		this->offloadReal();
	}
	else
	{
		// Delete the real part (includes decrementing refs of haunts)
		// as we're being destroyed.
		this->destroyReal();
	}
	MF_ASSERT( !this->isReal() );

	// make it a ghost script
	//this->pType()->convertToGhostScript( this );
	// .. by dropping all the properties of the real
	MF_ASSERT( properties_.size() == pEntityType_->propCountGhostPlusReal() );
	for (uint i = pEntityType_->propCountGhost(); i < properties_.size(); ++i)
	{
		if (properties_[i])
		{
			pEntityType_->propIndex(i)->dataType()->detach( properties_[i] );
		}
	}
	properties_.erase( properties_.begin() + pEntityType_->propCountGhost(),
		properties_.end() );

	this->relocated();

	Entity::callbacksPermitted( true );
}


/**
 *	This method adds the data associated with this entity to the input stream.
 *	It must be done in the same order as onload.
 */
void Entity::writeRealDataToStream( BinaryOStream & data,
	bool isTeleport ) const
{
	// Put the information on the stream. This must be done in the same order as
	// readRealDataFromStreamForOnload. First we do the Entity, then the script
	// related stuff and then the real stuff.

	data << this->id();

	data << false; // teleportFailure

	CompressionOStream compressionStream( data,
			pEntityType_->description().internalNetworkCompressionType() );
	this->writeRealDataToStreamInternal( compressionStream,
			isTeleport );
}


/**
 *	This method is called by writeRealDataToStream once the decision whether or
 *	not to compress has been made.
 */
void Entity::writeRealDataToStreamInternal( BinaryOStream & data,
	bool isTeleport ) const
{
	//this->pType()->dumpRealScript( this, data );
	MF_ASSERT( properties_.size() == pEntityType_->propCountGhostPlusReal() );
	for (uint i = pEntityType_->propCountGhost(); i < properties_.size(); ++i)
	{
		MF_ASSERT( properties_[i] );
		const DataDescription * pDataDesc = pEntityType_->propIndex( i );

		// TODO - implement component property processing here 
		MF_ASSERT( !pDataDesc->isComponentised() );

		ScriptDataSource source( properties_[i] );
		if (!pDataDesc->addToStream( source, data, false ))
		{
			CRITICAL_MSG( "Entity::writeRealDataToStream(%d): "
					"Could not stream on property %s.%s\n",
				id_,
				pEntityType_->description().name().c_str(),
				pDataDesc->name().c_str() );
		}
	}

	TOKEN_ADD( data, "RealProps" );

	pReal_->writeOffloadData( data, isTeleport );

	this->writeBasePropertiesExposedForReplayToStream( data );
}


/**
 * This method reads base properties exposed for replay from a stream.
 *	@param data	The stream to read the data from
 *	@return		true on success, false on error
 */
bool Entity::readBasePropertiesExposedForReplayFromStream( BinaryIStream & data )
{
	const int basePropertiesDataSize = data.readPackedInt();
	if (basePropertiesDataSize)
	{
		MemoryOStream basePropertiesData;
		basePropertiesData.transfer( data, basePropertiesDataSize );

		ScriptDict basePropDict = ScriptDict::create();

		const EntityDescription & description = pEntityType_->description();

		if (!description.readStreamToDict( basePropertiesData,
					EntityDescription::BASE_DATA, basePropDict ))
		{
			return false;
		}

		exposedForReplayClientProperties_ = basePropDict;
	}
	return true;
}


/**
 * This method writess base properties exposed for replay to a stream.
 *	@param data	The stream to write the data to
 */
void Entity::writeBasePropertiesExposedForReplayToStream(
	BinaryOStream & data ) const
{
	MemoryOStream propData;
	const EntityDescription & edesc = pEntityType_->description();

	if (exposedForReplayClientProperties_ &&
			exposedForReplayClientProperties_.size() > 0 &&
			!edesc.addDictionaryToStream(
				exposedForReplayClientProperties_, propData,
				EntityDescription::BASE_DATA ))
	{
		ERROR_MSG( "Entity::writeBasePropertiesExposedForReplayToStream: "
				"Add exposed base properties failed\n" );
	}

	data.writePackedInt( propData.remainingLength() );
	data.transfer( propData, propData.remainingLength() );
}


/**
 *	This method puts ghost controller data onto the input stream.
 *
 *	@param stream		The stream to put the data on.
 *	@see Entity::readGhostControllersFromStream
 */
void Entity::writeGhostControllersToStream( BinaryOStream & stream ) const
{
	pControllers_->writeGhostsToStream( stream );
}


/**
 *	This method puts ghost controller data onto the input stream.
 *
 *	@param stream		The stream to put the data on.
 *	@see Entity::readRealControllersFromStream
 */
void Entity::writeRealControllersToStream( BinaryOStream & data ) const
{
	pControllers_->writeRealsToStream( data );
}


/**
 *	This method starts all of the real controllers of this entity.
 */
void Entity::startRealControllers()
{
	MF_ASSERT( this->isReal() );

	pControllers_->startReals();
}


/**
 *	This method stops all the real controllers of this entity.
 */
void Entity::stopRealControllers()
{
	pControllers_->stopReals( this->isDestroyed() );
}


/**
 *	This class is used to handle a reply message and forward it onto a new
 *	address.
 */
class ReplyForwarder : public Mercury::ShutdownSafeReplyMessageHandler
{
public:
	ReplyForwarder( const Mercury::Address& destAddr,
			Mercury::ReplyID replyID ) :
		destAddr_( destAddr ), replyID_( replyID ) {}

private:
	virtual void handleMessage( const Mercury::Address& source,
			Mercury::UnpackedMessageHeader& header, BinaryIStream& data,
			void * arg )
	{
		Mercury::ChannelSender sender( CellApp::getChannel( destAddr_ ) );
		sender.bundle().startReply( replyID_ );
		sender.bundle().transfer( data, data.remainingLength() );
		delete this;
	}

	virtual void handleException( const Mercury::NubException& exception,
			void * arg )
	{
		ERROR_MSG( "ReplyForwarder::handleException: destAddr_ = %s\n",
			   destAddr_.c_str() );
		delete this;
	}

	// destination for the reply
	Mercury::Address destAddr_;
	Mercury::ReplyID replyID_;
};


/**
 *	This static message is used to forward a message to another CellApp.
 *
 *	@param realChannel  The channel on which the message will be forwarded.
 *	@param entityID The id of the entity to send the message to.
 *	@param messageID The id of the message to forward.
 *	@param data The message data to forward.
 *	@param srcAddr This is used if the message is a request. The reply will be
 *		forwarded to this address.
 *	@param replyID If not REPLY_ID_NONE, the message is a
 *		request. The reply will be forwarded to srcAddr via this application.
 */
void Entity::forwardMessageToReal(
		CellAppChannel & realChannel,
		EntityID entityID,
		uint8 messageID, BinaryIStream & data,
		const Mercury::Address & srcAddr, Mercury::ReplyID replyID )
{
	AUTO_SCOPED_PROFILE( "forwardToReal" );

	Mercury::ChannelSender sender( realChannel.channel() );
	Mercury::Bundle & bundle = sender.bundle();

	const Mercury::InterfaceElement & ie =
		CellAppInterface::gMinder.interfaceElement( messageID );

	if (replyID == Mercury::REPLY_ID_NONE)
	{
		bundle.startMessage( ie );
	}
	else
	{
		bundle.startRequest( ie, new ReplyForwarder( srcAddr, replyID ) );
	}

	bundle << entityID;

	bundle.transfer( data, data.remainingLength() );
}

BW_END_NAMESPACE

#include "pyscript/pywatcher.hpp"

BW_BEGIN_NAMESPACE

int Entity::numHaunts() const
{
	if (this->isReal())
	{
		return pReal_->numHaunts();
	}
	return -1;
}

namespace
{

BW::string entityPropertyIndexToName( const void * base, int index )
{
	Entity * pEntity = (Entity *)base;
	return pEntity->pType()->propIndex( index )->name();
}

// TODO: support component properties
int entityPropertyNameToIndex( const void * base, const BW::string & name )
{
	Entity * pEntity = (Entity *)base;

	const DataDescription * pDescription =
			pEntity->pType()->description( name.c_str(), /*component*/"");

	if (!pDescription)
	{
		return -1;
	}

	return pDescription->localIndex();
}

} // namespace anonymous


/**
 *	This method returns the static Watcher associated with this class.
 */
WatcherPtr Entity::pWatcher()
{
	static DirectoryWatcherPtr pWatcher = NULL;

	if (!pWatcher)
	{
		pWatcher = new DirectoryWatcher;

		//pWatcher.addChild( "profiles", new DirectoryWatcher() );

		Entity * pNull = NULL;

		pWatcher->addChild( "type",
							new SmartPointerDereferenceWatcher(
								makeWatcher( &EntityType::name ) ),
							&pNull->pEntityType_ );

		pWatcher->addChild( "id", makeWatcher( &Entity::id_ ) );
		pWatcher->addChild( "pos", makeWatcher( &Entity::globalPosition_ ) );
		pWatcher->addChild( "dir", makeWatcher( &Entity::globalDirection_ ) );
		pWatcher->addChild( "localPos", makeWatcher( &Entity::localPosition_ ) );
		pWatcher->addChild( "localDir", makeWatcher( &Entity::localDirection_ ) );
		pWatcher->addChild( "haunts", makeWatcher( &Entity::numHaunts ) );
		pWatcher->addChild( "spaceID", makeWatcher( &Entity::spaceID ) );

		typedef SequenceWatcher< Properties > PropWatcher;

		PropWatcher * propertiesWatcher =
			new PropWatcher( pNull->properties_ );
		propertiesWatcher->setStringIndexConverter( entityPropertyNameToIndex,
													entityPropertyIndexToName );
		propertiesWatcher->addChild( "*",
			new SmartPointerDereferenceWatcher( new SimplePythonWatcher() ) );
		pWatcher->addChild( "properties", propertiesWatcher );

		pWatcher->addChild( "profile",
							EntityProfiler::pWatcher(),
							&pNull->profiler_ );

#if 0
		pWatcher->addChild( "real", new BaseDereferenceWatcher(
			&RealEntity::pWatcher() ), &pNull->pReal_ );
#endif
	}

	return pWatcher;
}


/**
 *	This method adds this entity to a range list.
 *
 *	@param rangeList The range list to add to.
 */
void Entity::addToRangeList( RangeList & rangeList,
	RangeTriggerList & appealRadiusList )
{
	AUTO_SCOPED_THIS_ENTITY_PROFILE;

	rangeList.add( pRangeListNode_ );

	if (pRangeListAppealTrigger_)
	{
		// AppealRadius is non-zero.
		// Entity node is no longer an AoI trigger since range is taking over.
		pRangeListNode_->isAoITrigger( false );

		pRangeListAppealTrigger_->insert();
		this->addTrigger( pRangeListAppealTrigger_ );

		appealRadiusList.push_back( pRangeListAppealTrigger_ );
	}
}


/**
 *	This method removes this entity from a range list.
 *
 *	@param rangeList The range list to add to.
 */
void Entity::removeFromRangeList( RangeList & rangeList,
	RangeTriggerList & appealRadiusList )
{
	AUTO_SCOPED_THIS_ENTITY_PROFILE;

	if (pRangeListAppealTrigger_)
	{
		appealRadiusList.remove( pRangeListAppealTrigger_ );

		this->delTrigger( pRangeListAppealTrigger_ );
		pRangeListAppealTrigger_->remove();

		// Entity node now needs to be an AoI trigger
		pRangeListNode_->isAoITrigger( true );
	}

	pRangeListNode_->remove();
}


/**
 *	This method adds the given history event to this entity locally.
 *	@see RealEntity::addHistoryEvent for what you probably want.
 */
HistoryEvent * Entity::addHistoryEventLocally( uint8 type,
	MemoryOStream & stream,
	const MemberDescription & description, int16 msgStreamSize,
	HistoryEvent::Level level )
{
	AUTO_SCOPED_THIS_ENTITY_PROFILE;

#if ENABLE_WATCHERS
	pEntityType_->stats().countAddedToHistoryQueue( stream.size() );
#endif

	return this->eventHistory().add( this->getNextEventNumber(),
			type, stream, description, level, msgStreamSize );
}


/**
 *	This method writes any relevant information about this entity that has
 *	occurred since the last update time to the given bundle. This includes
 *	changes in volatile position and new history events.
 *
 *	@param bundle			The bundle to put the information on.
 *	@param basePos	The reference point for relative positions.
 *	@param cache	The current entity cache.
 *	@param lodPriority	Indicates what level of detail to use.
 *
 *	@return		True if a reliable position update message was included in the
 *				bundle, false otherwise.
 */
bool Entity::writeClientUpdateDataToBundle( Mercury::Bundle & bundle,
		const Vector3 & basePos,
		EntityCache & cache,
		float lodPriority ) const
{
	AUTO_SCOPED_THIS_ENTITY_PROFILE;

	const int initSize = bundle.size();
	int oldSize = initSize;
	int numEvents = 0;

	this->writeVehicleChangeToBundle( bundle, cache );

	// Send the appropriate history for this entity
	EventHistory::const_reverse_iterator eventIter = eventHistory_.rbegin();
	EventHistory::const_reverse_iterator eventEnd  = eventHistory_.rend();

	// Go back to find the correct place then forward to add to the bundle in
	// chronological order.

	// TODO: Consider the wrap around case. To wrap around a 32-bit value
	// needs 12 events a second for 10 years (24 hours a day).

	bool hasEventsToSend = false;
	bool hasSelectedEntity = false;

	while (eventIter != eventEnd &&
			(*eventIter)->number() > cache.lastEventNumber())
	{
		HistoryEvent & event = **eventIter;
		hasEventsToSend = hasEventsToSend ||
			event.shouldSend( lodPriority, cache.detailLevel() );

		eventIter++;
	}

	bool hasAddedReliableRelativePosition = false;

	// Not currently enabled as it affects the filters if this is not sent
	// regularly.
	//if (cache.lastVolatileUpdateNumber() != volatileUpdateNumber_)
	{
		cache.lastVolatileUpdateNumber( volatileUpdateNumber_ );

		if (this->volatileInfo().hasVolatile( lodPriority ))
		{
			const bool isReliable = hasEventsToSend;

			if (cache.isAlwaysDetailed() || (cache.isPrioritised() && CellAppConfig::sendDetailedPlayerVehicles()) )
			{
				this->writeVolatileDetailedDataToBundle( bundle,
						cache.idAlias(), isReliable );
			}
			else
			{
				hasAddedReliableRelativePosition =
					this->writeVolatileDataToBundle( bundle, basePos,
						cache.idAlias(), lodPriority, isReliable );
			}

			hasSelectedEntity = true;

			oldSize = bundle.size();
			g_nonVolatileBytes += (oldSize - initSize);
#if ENABLE_WATCHERS
			pEntityType_->stats().countVolatileSentToOtherClients(
					oldSize - initSize );
#endif
		}
	}

	if (hasEventsToSend)
	{
		if (!hasSelectedEntity)
		{
			cache.addEntitySelectMessage( bundle );
			hasSelectedEntity = true;
		}

		while (eventIter != eventHistory_.rbegin())
		{
			eventIter--;

			HistoryEvent & event = **eventIter;

			if (event.shouldSend( lodPriority, cache.detailLevel() ))
			{
				if (event.pName())
				{
					g_totalPublicClientStats.trackEvent( pEntityType_->name(),
						event.pName()->c_str(), event.msgLen(),
						event.msgStreamSize() );
				}

				++numEvents;

				event.addToBundle( bundle );
			}
		}
	}

	cache.lastEventNumber( this->lastEventNumber() );

	int nonVolatileSize = bundle.size() - oldSize;

#if ENABLE_WATCHERS
	pEntityType_->stats().countNonVolatileSentToOtherClients( nonVolatileSize );
#endif

	g_nonVolatileBytes += nonVolatileSize;

	if (nonVolatileSize > CellAppConfig::entitySpamSize())
	{
		WARNING_MSG( "Entity::writeClientUpdateDataToBundle(%u): "
							"[%s] added %d bytes with %d events\n",
						id_,
						pEntityType_->name(),
						bundle.size() - oldSize,
						numEvents );
	}

	hasSelectedEntity |= cache.updateDetailLevel( bundle, lodPriority,
		hasSelectedEntity );

	// see if we thought no-one had noticed us before
	if (periodsWithoutWitness_)
	{
		if (this->isReal())
		{
			// The callback generated from this call will be delayed until the
			// end of the update loop.
			const_cast<Entity*>( this )->witnessed();
		}
		else
		{
			// tell the real
			Mercury::ChannelSender sender( pRealChannel_->channel() );

			// TODO: Better handling of prefixed empty messages
			sender.bundle().startMessage( CellAppInterface::witnessed );
			sender.bundle() << id_;

			// (it will usually only have told us that it has inc'd the
			// unwitnessed level when higher than 1, i.e. if it has no
			// witnesses of its own)
		}
		periodsWithoutWitness_ = 0;
	}

	return hasAddedReliableRelativePosition;
}


/**
 *	This method writes a vehicle change message to the bundle if the vehicle
 *	of the entity in the cache, has changed.
 */
void Entity::writeVehicleChangeToBundle( Mercury::Bundle & bundle,
		EntityCache & cache ) const
{
	AUTO_SCOPED_THIS_ENTITY_PROFILE;

	// if our record of this entity being on a vehicle doesn't match
	// its current state of vehicular activity, then tell the client
	if (cache.vehicleChangeNum() != this->vehicleChangeNum())
	{
		BaseAppIntInterface::setVehicleArgs & args =
			BaseAppIntInterface::setVehicleArgs::start( bundle );
		args.passengerID = this->id();
		args.vehicleID = this->vehicleID();
		cache.vehicleChangeNum( this->vehicleChangeNum() );
	}
}

/**
 *	This static method returns the corresponding avatar update message.
 *	Use a function to encapsulate the static declaration instead
 *	of scattering outside any function encapsulation. The first 
 *	call of this function will initialize this static array. So
 *	the initialization moment is deterministic.
 *
 *	@param index	The index corresponding to the avatar update message.
 *	            	It must be within legal range as indicated below.
 *
 *	@return     	The pointer to the InterfaceElement.
 */
const Mercury::InterfaceElement * Entity::getAvatarUpdateMessage( int index )
{

#define BEGIN_AV_UPD_MESSAGES()											\
	static const Mercury::InterfaceElement * s_avatarUpdateMessage[] =	\
	{

#define AV_UPD_MESSAGE( TYPE ) &BaseAppIntInterface::avatarUpdate##TYPE,

#define END_AV_UPD_MESSAGES()	};

	BEGIN_AV_UPD_MESSAGES()
		AV_UPD_MESSAGE( NoAliasFullPosYawPitchRoll )
		AV_UPD_MESSAGE( NoAliasFullPosYawPitch )
		AV_UPD_MESSAGE( NoAliasFullPosYaw )
		AV_UPD_MESSAGE( NoAliasFullPosNoDir )

		AV_UPD_MESSAGE( NoAliasOnGroundYawPitchRoll )
		AV_UPD_MESSAGE( NoAliasOnGroundYawPitch )
		AV_UPD_MESSAGE( NoAliasOnGroundYaw )
		AV_UPD_MESSAGE( NoAliasOnGroundNoDir )

		AV_UPD_MESSAGE( NoAliasNoPosYawPitchRoll )
		AV_UPD_MESSAGE( NoAliasNoPosYawPitch )
		AV_UPD_MESSAGE( NoAliasNoPosYaw )
		AV_UPD_MESSAGE( NoAliasNoPosNoDir )

		AV_UPD_MESSAGE( AliasFullPosYawPitchRoll )
		AV_UPD_MESSAGE( AliasFullPosYawPitch )
		AV_UPD_MESSAGE( AliasFullPosYaw )
		AV_UPD_MESSAGE( AliasFullPosNoDir )

		AV_UPD_MESSAGE( AliasOnGroundYawPitchRoll )
		AV_UPD_MESSAGE( AliasOnGroundYawPitch )
		AV_UPD_MESSAGE( AliasOnGroundYaw )
		AV_UPD_MESSAGE( AliasOnGroundNoDir )

		AV_UPD_MESSAGE( AliasNoPosYawPitchRoll )
		AV_UPD_MESSAGE( AliasNoPosYawPitch )
		AV_UPD_MESSAGE( AliasNoPosYaw )
		AV_UPD_MESSAGE( AliasNoPosNoDir )
	END_AV_UPD_MESSAGES()

	MF_ASSERT( 0 <= index && index < 24 );

	return s_avatarUpdateMessage[ index ];
}


/**
 *	This method is used to add a volatile position update message to the
 *	bundle.
 *
 *	@param bundle		The Bundle to write the message to
 *	@param basePosition	The space position the update is relative to
 *	@param idAlias		The IDAlias of this entity on the relevant
 *				witness, or NO_ID_ALIAS if the client doesn't
 *				have one.
 *	@param priorityThreshold	The distance from this entity to the
 *					witness
 *	@param isReliable	True if this message is being used to select
 *				this entity for the event-history messages
 *				streamed on next.
 *
 *	@return		True if we wrote a reliable relative position message
 *				to the stream, false otherwise.
 *
 *	@see Entity::writeVolatileDetailedDataToBundle
 */
bool Entity::writeVolatileDataToBundle( Mercury::Bundle & bundle,
		const Vector3 & basePosition,
		IDAlias idAlias,
		float priorityThreshold,
		bool isReliable ) const
{
	const bool sendAlias    = (idAlias != NO_ID_ALIAS);
	const bool sendOnGround = this->isOnGround();
	const bool shouldSendPosition = volatileInfo_.shouldSendPosition();
	const float scale = CellAppConfig::packedXZScale();

	// Calculate the relative position.
#if VOLATILE_POSITIONS_ARE_ABSOLUTE
	const bool isRelative = false;
	const Vector3 relativePos = localPosition_;
#else /* VOLATILE_POSITIONS_ARE_ABSOLUTE */
	const bool isRelative = (pVehicle_ == NULL);
	const Vector3 relativePos = isRelative ?
		localPosition_ - basePosition : localPosition_;
#endif /* VOLATILE_POSITIONS_ARE_ABSOLUTE */

	if (shouldSendPosition)
	{
		// If we cannot represent the given relative position
		// with a PackedXZ or PackedXYZ (as appropriate) send
		// a detailed volatile position update instead.
		const float maxLimit = sendOnGround ?
			PackedXZ::maxLimit( scale ) :
			PackedXYZ::maxLimit( scale );

		const float minLimit = sendOnGround ?
			PackedXZ::minLimit( scale ) :
			PackedXYZ::minLimit( scale );

		if ((relativePos.x < minLimit) || (maxLimit <= relativePos.x) ||
			(relativePos.z < minLimit) || (maxLimit <= relativePos.z))
		{
			this->writeVolatileDetailedDataToBundle( bundle,
				idAlias, isReliable );
			return false;
		}
	}

	const int posType = shouldSendPosition ? sendOnGround : 2;
	const int dirType = volatileInfo_.dirType( priorityThreshold );

	const int index = sendAlias * 12 + posType * 4 + dirType;

	// We can't actually send an avatarUpdate*NoPosNoDir message, we
	// would never have entered this method.
	// TODO: Remove those messages.
	MF_ASSERT( dirType < 3 || posType < 2 );

	if (isReliable)
	{
		BaseAppIntInterface::makeNextMessageReliableArgs::start(
			bundle ).isReliable = true;
	}

	bundle.startMessage( *getAvatarUpdateMessage( index ) );

	if (sendAlias)
	{
		bundle << idAlias;
	}
	else
	{
		bundle << this->id();
	}

	if (shouldSendPosition)
	{
		if (sendOnGround)
		{
			bundle << PackedXZ( relativePos.x, relativePos.z, scale );
		}
		else
		{
			bundle << PackedXYZ( relativePos.x, relativePos.y, relativePos.z,
					scale );
		}
	}

	// const Direction3D & dir = this->direction();
	const Direction3D & dir = localDirection_;

	switch (dirType)
	{
		case 0:
			bundle << YawPitchRoll( dir.yaw, dir.pitch, dir.roll );
			break;

		case 1:
			bundle << YawPitch( dir.yaw, dir.pitch );
			break;

		case 2:
			bundle << Yaw( dir.yaw );
			break;
	}

	return isReliable && isRelative;
}


/**
 *	This method is used to add a volatile position update message to the
 *	bundle.
 *
 *	Entity::writeVolatileDataToBundle will call through to this instead if
 *	given a position it cannot send through its normal update method.
 *
 *	@param bundle		The Bundle to write the message to
 *	@param idAlias		The IDAlias of this entity on the relevant
 *				witness, or NO_ID_ALIAS if the client doesn't
 *				have one.
 *	@param isReliable	True if this message is being used to select
 *				this entity for the event-history messages
 *				streamed on next.
 *
 *	@see Entity::writeVolatileDataToBundle
 */
void Entity::writeVolatileDetailedDataToBundle( Mercury::Bundle & bundle,
		IDAlias idAlias,
		bool isReliable ) const
{
	AUTO_SCOPED_THIS_ENTITY_PROFILE;

	const bool sendAlias = (idAlias != NO_ID_ALIAS);

	if (isReliable)
	{
		BaseAppIntInterface::makeNextMessageReliableArgs::start(
			bundle ).isReliable = true;
	}

	if (sendAlias)
	{
		bundle.startMessage( BaseAppIntInterface::avatarUpdateAliasDetailed );
		bundle << idAlias;
	}
	else
	{
		bundle.startMessage( BaseAppIntInterface::avatarUpdateNoAliasDetailed );
		bundle << this->id();
	}

	bundle << localPosition_;
	bundle << localDirection_;
}


/**
 *	This method sets this entity into a dead state. We cannot delete it straight
 *	away since other things may be referencing it.
 */
void Entity::destroy()
{
	if (inDestroy_)
		return;

	inDestroy_ = true;

	// Just in case it is deleted while setting dead.
	EntityPtr pThis = this;

	MF_ASSERT( !isDestroyed_ );
	// Moved to after the onDestroy call.
	// this->setDestroyed();

	// Remember the recording state for later.
	const bool wasReal = this->isReal();
	Cell & cell = this->cell();

	if (this->isReal())
	{
		// TODO: We may want to add an option so that we do not always send the
		// database information to the base entity if it is not going to write
		// it to the database.
		this->callback( "onDestroy" );

		if (this->hasBase())
		{
			if (!this->sendCellEntityLostToBase())
			{
				ERROR_MSG( "Entity::destroy(%u): "
					"Unable to gracefully inform base entity of destroy.\n",
						id_ );

				// Inform base entity ungracefully.
				Mercury::Bundle & bundle = pReal_->channel().bundle();
				BaseAppIntInterface::setClientArgs setClientArgs = { id_ };
				bundle << setClientArgs;
				bundle.startMessage( BaseAppIntInterface::cellEntityLost );
				pReal_->channel().send();
			}
		}

		this->setDestroyed();

		pReal_->deleteGhosts();
		this->cell().entityDestroyed( this );
		this->convertRealToGhost();

		// if this is a cell only entity then add the id to our recycled id
		// list. Else, let the base take care of it.
		if (!this->hasBase())
		{
			shouldReturnID_ = true;
		}

		// This shouldn't be in there but is a sanity check.
		population_.forgetRealChannel( id_ );
	}
	else
	{
		this->callback( "onGhostDestroyed" );

		// Remember where the ghost thought the real is in case a message is
		// received for this entity.
		if (pRealChannel_)
		{
			population_.rememberRealChannel( id_, *pRealChannel_ );
		}
	}

	this->setDestroyed();

	pEntityDelegate_ = IEntityDelegatePtr();

	Entity::callbacksPermitted( false ); // {

	bw_safe_delete( pControllers_ );

	pSpace_->removeEntity( this );
	pSpace_ = NULL;

	if (pChunk_ != NULL)
	{
		CellChunk::instance( *pChunk_ ).removeEntity( this );
		pChunk_ = NULL;
	}

	// Get rid of any entity extras (for the first time)
	for (uint i = 0; i < s_entityExtraInfo().size(); i++)
	{
		delete extras_[i];
		extras_[i] = NULL;
	}

	// Clear out pRealChannel_, otherwise we may have a dangling pointer to a
	// destroyed CellAppChannel in the event of CellApp death.
	pRealChannel_ = NULL;

	this->clearPythonProperties();

	inDestroy_ = false;

	Entity::callbacksPermitted( true ); // }

	if (wasReal && cell.pReplayData())
	{
		cell.pReplayData()->deleteEntity( id_ );
	}

	// If this has any create ghost messages
	CellApp::instance().bufferedGhostMessages().
		playNewLifespanFor( this->id() );
}


void Entity::setDestroyed()
{
	if (!isDestroyed_)
	{
		isDestroyed_ = true;
		--g_numEntities;
	}
}


/**
 *	This method creates our RealEntity
 */
void Entity::createReal()
{
	MF_ASSERT( !this->isReal() );
	pReal_ = new RealEntity( *this );

	if (pEntityDelegate_)
	{
		pEntityDelegate_->onEntityPositionUpdatable( /* isUpdatable */ true );
	}
}


/**
 *	This method cleans up our RealEntity before our own destruction
 */
void Entity::destroyReal()
{
	MF_ASSERT( this->isReal() );
	pReal_->destroy( NULL );
	// RealEntity::destroy calls 'delete this'
	pReal_ = NULL;

	if (pEntityDelegate_)
	{
		pEntityDelegate_->onEntityPositionUpdatable( /* isUpdatable */ false );
	}
}


/**
 *	This method cleans up our RealEntity for offload to nextRealAddr_
 */
void Entity::offloadReal()
{
	// This is the one time where we have both a pRealChannel_ and a pReal_
	MF_ASSERT( pReal_ != NULL && pRealChannel_ != NULL );
	pReal_->destroy( &(nextRealAddr_) );
	// RealEntity::destroy calls 'delete this'
	pReal_ = NULL;

	if (pEntityDelegate_)
	{
		pEntityDelegate_->onEntityPositionUpdatable( /* isUpdatable */ false );
	}
}

//serp: todo: use this
/*static int DelegateEntityInit(PyObject *self, PyObject *args, PyObject *kwds)
{
	initproc superInit = self->ob_type->tp_base->tp_init;
	int result = superInit == NULL : 0 : (*superInit)(self, args, kwds);
	if (result != 0)
	{
		return result;
	}

	Entity* pEntity = static_cast<Entity*>(self);
	pEntity->callDelegateInit();

	return 0;
}
*/

/**
 *	This method attempts to populate pEntityDelegate_ with defaulted values
 *	This must be done while not Real, as initReal and convertGhostToReal
 *	both update state that the IEntityDelegate must know about.
 */
void Entity::createEntityDelegate()
{
	MF_ASSERT( !this->isReal() );
	MF_ASSERT( pEntityDelegate_ == NULL );

	IEntityDelegateFactory * pDelegateFactory = getEntityDelegateFactory();

	if (!pDelegateFactory)
	{
		return;
	}

	TRACE_MSG( "Entity::createEntityDelegate: "
		"Creating entity delegate for '%s' entity #%d in space #%d\n",
		this->pType()->name(), this->id(), this->spaceID());

	pEntityDelegate_ = pDelegateFactory->createEntityDelegate(
		this->pType()->description(), this->id(), this->spaceID() );


/*	if (pEntityDelegate_ && Entity::s_type_.tp_init == NULL)
	{
		Entity::s_type_.tp_init = DelegateEntityInit;
	}*/
}


/**
 *	This method attempts to populate pEntityDelegate_ with a template, and
 *	update localPosition_, isOnGround_ and localDirection_ from that template.
 *
 *	This must be done while not Real, as initReal and convertGhostToReal
 *	both update state that the IEntityDelegate must know about.
 */
void Entity::createEntityDelegateWithTemplate( const BW::string & templateID )
{
	MF_ASSERT( !this->isReal() );
	MF_ASSERT( pEntityDelegate_ == NULL );

	IEntityDelegateFactory * pDelegateFactory = getEntityDelegateFactory();

	if (!pDelegateFactory)
	{
		return;
	}

	TRACE_MSG( "Entity::createEntityDelegateWithTemplate: "
		"Creating entity delegate for '%s' entity #%d in space #%d from template '%s'\n",
		this->pType()->name(), this->id(), this->spaceID(), templateID.c_str());

	pEntityDelegate_ = pDelegateFactory->createEntityDelegateFromTemplate(
		this->pType()->description(), this->id(), this->spaceID(), templateID );

	// The template may override localPosition_, isOnGround_ and localDirection_
	if (pEntityDelegate_)
	{
		pEntityDelegate_->getSpatialData(
			localPosition_, isOnGround_, localDirection_);
	}
}


void Entity::onPositionChanged()
{
	if (!pEntityDelegate_)
	{
		return;
	}

	pEntityDelegate_->onEntityPositionUpdated();
}


/**
 *	This method clears the Python properties associated with this entity.
 */
void Entity::clearPythonProperties()
{
	// detach all our properties
	for (uint i = 0; i < properties_.size(); ++i)
	{
		ScriptObject pObject = properties_[i];
		if (pObject)
		{
			pEntityType_->propIndex( i )->dataType()->detach( pObject );
		}
	}

	properties_.clear();
}


/**
 *	This method adjusts this entity's base address to the appropriate backup
 *	BaseApp when a BaseApp dies.
 */
void Entity::adjustForDeadBaseApp( const BackupHash & backupHash )
{
	AUTO_SCOPED_THIS_ENTITY_PROFILE;

	baseAddr_ = backupHash.addressFor( id_ );

	if (this->pReal())
	{
		this->pReal()->channel().reset( baseAddr_ );
		if (this->pReal()->pWitness() != NULL)
		{
			this->pReal()->disableWitness();
		}
		// This serves a dual purpose, to make sure the base is up-to date with
		// the entity's position and to generate traffic to see if we get a response
		this->informBaseOfAddress( CellApp::instance().interface().address(),
								   this->space().id(), true );
	}
}


/**
 *	This method informs the base entity of the address of the cell entity.
 */
void Entity::informBaseOfAddress( const Mercury::Address & addr,
		SpaceID spaceID, bool shouldSendNow )
{
	AUTO_SCOPED_THIS_ENTITY_PROFILE;

	// TODO: Move this to RealEntity
	MF_ASSERT( this->isReal() );

	if (this->hasBase())
	{
		Mercury::Bundle & bundle = pReal_->channel().bundle();

		BaseAppIntInterface::setClientArgs & setClientArgs =
			BaseAppIntInterface::setClientArgs::start( bundle );
		setClientArgs.id = id_;

		// Our base knowing where we are is considered to be critical.  In
		// particular, if the base doesn't know about this real it may try to
		// restore this entity somewhere else following a cellapp crash which
		// will cause the !pOtherGhost->isReal() assertion.
		BaseAppIntInterface::currentCellArgs & currentCellArgs =
			BaseAppIntInterface::currentCellArgs::start(
				bundle, Mercury::RELIABLE_CRITICAL );

		currentCellArgs.newSpaceID = spaceID;
		currentCellArgs.newCellAddr = addr;

		if (shouldSendNow)
		{
			pReal_->channel().send();
		}
	}
}


/**
 *	This helper method is used by Entity::sendMessageToReal.
 *
 *	@see sendMessageToReal
 *
 *	@return True if successful, otherwise false.
 */
bool Entity::writeCellMessageToBundle( Mercury::Bundle & bundle,
		const MethodDescription * pDescription,
		ScriptTuple args ) const
{
	bundle.startMessage( CellAppInterface::runScriptMethod );
	bundle << id_;
	bundle << uint16(pDescription->internalIndex());

	EntityID sourceEntityID;
	ScriptTuple remainingArgs = pDescription->extractSourceEntityID(
		args, sourceEntityID );

	ScriptDataSource source( remainingArgs );

	bool succeeded = pDescription->addToServerStream( source, bundle, 
		sourceEntityID );

	if (!succeeded)
	{
		CRITICAL_MSG( "Entity::writeCellMessageToBundle(%u): "
				"Failed to add to stream for method %s.%s\n",
			id_, this->pType()->name(), pDescription->name().c_str() );

		// TODO: If adding to the stream fails, we should not leave the bundle
		// in a bad state.
	}

	return succeeded;
}


/**
 *	This method returns the address of the cell that contains the real entity
 *	of this entity.  This may be the address of this CellApp.
 *
 *	@see Entity::pRealCell
 */
const Mercury::Address & Entity::realAddr() const
{
	// TODO: Should this check for isDestroyed_?
	return pRealChannel_ ?
		pRealChannel_->addr() :
		CellApp::instance().interface().address();
}


/**
 *  This method checks if the entity is a zombie or will be zombied by a CellApp
 *  death. The zombie is recovered if possible.
 *
 *  @param dyingAddr	Address of dying CellApp that could zombify a ghost.
 *
 *	@return	True if entity is a zombie or will be zombied, false if entity
 *			is destroyed, a real or a ghost.
 */
bool Entity::checkIfZombied( const Mercury::Address & dyingAddr )
{
	if (this->isDestroyed() || this->isReal())
	{
		return false;
	}

	// Zombied if next real is dying or dead.
	if (nextRealAddr_ != Mercury::Address::NONE &&
			(nextRealAddr_ == dyingAddr ||
			CellAppChannels::instance().hasRecentlyDied( nextRealAddr_ )))
	{
		return true;
	}

	// Check if current real is healthy
	if (pRealChannel_ && pRealChannel_->addr() != dyingAddr)
	{
		return false;
	}
	else if (nextRealAddr_ != Mercury::Address::NONE)
	{
		INFO_MSG( "Entity::checkIfZombied: "
				"Recovering zombie (%u) by updating current real %s "
				"to next real %s\n",
			id_,
			pRealChannel_ ?
				pRealChannel_->addr().c_str() : Mercury::Address::NONE.c_str(),
			nextRealAddr_.c_str() );

		pRealChannel_ = CellAppChannels::instance().get( nextRealAddr_ );

		return false;
	}

	return true;
}


/**
 *	This method sends a message to the real entity corresponding to this ghost
 *	entity.
 *
 *	@param pDescription The description of the method.
 *	@param args		A Python tuple containing the arguments.
 */
bool Entity::sendMessageToReal( const MethodDescription * pDescription,
		ScriptTuple args )
{
	AUTO_SCOPED_THIS_ENTITY_PROFILE;
	//TRACE_MSG( "Entity::sendMessageToReal: Method = %s. Entity = %d\n",
	//		pDescription->name().c_str(), this->id() );

	MF_ASSERT( !this->isRealToScript() );

	// Since this entity might actually be real but we are just pretending it's
	// a ghost (because of treatAllOtherEntitiesAsGhosts), we may need to send
	// the message on a channel to this app.
	Mercury::UDPChannel & channel = pRealChannel_ ?
		pRealChannel_->channel() :
		CellApp::getChannel( CellApp::instance().interface().address() );

	Mercury::ChannelSender sender( channel );

	return this->writeCellMessageToBundle(
		sender.bundle(), pDescription, args );
}


/**
 *	This helper method is used by Entity::sendToClientViaReal.
 *
 *	@see sendMessageToReal
 *
 *	@return True if successful, otherwise false.
 */
bool Entity::writeClientMessageToBundle( Mercury::Bundle & bundle,
		EntityID entityID, const MethodDescription & description,
		MemoryOStream & argstream, int callingMode ) const
{
	bundle.startMessage( CellAppInterface::callClientMethod );
	bundle << id_;
	bundle << entityID;
	bundle << uint8( callingMode );
	bundle << uint16( description.internalIndex() );

	bundle.transfer( argstream, argstream.remainingLength() );
	return true;
}


/**
 *	This method sends a message to the client via real entity corresponding to
 *  the ghost entity.
 *
 *	@param entityID		The id of the entity to send to.
 *	@param description	The description of the method.
 *	@param argStream		A MemoryOStream containing the destination entity
 *						ID and arguments for the method.
 *	@param isForOwn		A boolean flag indicates whether message should be
 *						sent to the client associated with the entity.
 *	@param isForOthers	A boolean flag indicates whether message should be
 *						send to all other clients apart from the client
 *						associated with the entity.
 */
bool Entity::sendToClientViaReal( EntityID entityID,
		const MethodDescription & description, MemoryOStream & argStream,
		bool isForOwn, bool isForOthers,
		bool isExposedForReplay )
{
	//TRACE_MSG( "Entity::sendToClientViaReal: Method = %s. Entity = %d\n",
	//		pDescription.name().c_str(), this->id() );

	MF_ASSERT( !this->isReal() );

	int callingMode = 0;

	if (isForOwn)
	{
		callingMode = MSG_FOR_OWN_CLIENT;
	}

	if (isForOthers)
	{
		callingMode |= MSG_FOR_OTHER_CLIENTS;
	}

	if (isExposedForReplay)
	{
		callingMode |= MSG_FOR_REPLAY;
	}

	return this->writeClientMessageToBundle( pRealChannel_->bundle(),
		entityID, description, argStream, callingMode );
}


/**
 *	This method determines whether or not the given movement is
 *	physically possible. In space transitions, the move will be tried
 *	in two parts, and propMove will be < 1 for the first part.
 *	(the state is left such that it can be = 1 for the second part)
 */
bool Entity::physicallyPossible( const Position3D & newPosition, Entity * pVehicle,
	float propMove )
{
	// first see if the vehicles involved are ok with this
	if ((pVehicle != pVehicle_) &&
			!this->validateAvatarVehicleUpdate( pVehicle ))
	{
		return false;
	}

	// find out where we are moving from and to
	Vector3 srcPos = globalPosition_;
	srcPos.y += 0.1f;

	Vector3 lPos( newPosition.x, newPosition.y, newPosition.z );
	Vector3 dstPos = (pVehicle == NULL) ? lPos :
		Passengers::instance( *pVehicle ).transform().applyPoint( lPos );
	dstPos.y += 0.1f;

	// make sure that we are not moving too fast

	uint64 nowTime = timestamp();
	uint64 delTime = std::min( stampsPerSecond(),
								nowTime - physicsLastValidated_ );
	physicsLastValidated_ = nowTime;			// must move every 1s
	double delTimeD = stampsToSeconds( delTime ) * propMove;

	if (pVehicle_ == pVehicle)
	{
		float delDistSqr = 0.f;
		float delVertDist = 0.f;

		if (topSpeedY_ > 0.f)
		{
			Vector3 a( localPosition_ );
			Vector3 b( lPos );

			// the y-axis distance
			delVertDist = fabs( b.y - a.y );

			a.y = 0.f;
			b.y = 0.f;

			// this now only takes into account X-Z plane distance
			delDistSqr = ServerUtil::distSqrBetween( a, b );

		}
		else
		{
			// this is distance is against all axes (XYZ)
			delDistSqr = ServerUtil::distSqrBetween( localPosition_, lPos );
		}

		float maxDist = topSpeed_ * delTimeD;
		float maxVertDist = topSpeedY_ * delTimeD;

		// allow a small physics debt to cope with network jitter
		if (delDistSqr > maxDist * maxDist)
		{
			// avoid doing the square roots unless necessary
			float exceededBy = sqrtf( delDistSqr ) - maxDist;
			physicsNetworkJitterDebt_ += exceededBy / topSpeed_;
			if (physicsNetworkJitterDebt_ >
					CellAppConfig::maxPhysicsNetworkJitter())
			{
				return false;
			}
		}
		else if (topSpeedY_ > 0.f && delVertDist > maxVertDist)
		{
			float exceededBy = delVertDist - maxVertDist;
			physicsNetworkJitterDebt_ += exceededBy / topSpeedY_;
			if (physicsNetworkJitterDebt_ >
					CellAppConfig::maxPhysicsNetworkJitter())
			{
				return false;
			}
		}
		else if (physicsNetworkJitterDebt_ > 0.f)
		{
			// avoid doing the square roots unless necessary
			float exceededBy = sqrtf( delDistSqr ) - maxDist;
			physicsNetworkJitterDebt_ += exceededBy / topSpeed_;
			// (but make sure the account does not go into credit)
			if (physicsNetworkJitterDebt_ < 0.f)
			{
				physicsNetworkJitterDebt_ = 0.f;
			}
		}
	}

	// (if we are not moving the full distance, move time on such that only
	// the specified proportion of it has appeared to pass)
	if (propMove < 1.f)
	{
		physicsLastValidated_ -= int64(double(delTime) * (1.0-propMove));
	}

	// see if the game wants to reject it (before we get tricky with chunks)
	if (g_customPhysicsValidator &&
			!(*g_customPhysicsValidator)( this, lPos, pVehicle, delTimeD ))
	{
		return false;
	}

	if (pChunk_ == NULL)
	{
		// They may be in a client-only space.
		// TODO: Should look up the space data to determine which areas are
		// mapped, and determine whether this is valid.
		// However, we allow it for now because otherwise player-controlled
		// entities with topSpeed set aren't able to move in client-only
		// spaces.
		return true;
	}

	if (!this->pChunkSpace()) {
		ERROR_MSG("Entity::physicallyPossible(%u): "
			"Not supported in physical space of this entity.",
			id_);
		return false;
	}

	if (!pChunk_->contains( srcPos ))
	{
		Chunk * pCChunk = this->pChunkSpace()->findChunkFromPoint( srcPos );
		if (pCChunk == NULL)
		{
			ERROR_MSG("Entity::physicallyPossible(%u): "
				"OMG! We are lost in the (chunk) space. Cannot find starting "
				"point (%f,%f,%f) in any chunks. Abort!!",
				id_, srcPos.x, srcPos.y, srcPos.z );
			return false;
		}
		else if (pCChunk != pChunk_)
		{
			ERROR_MSG("Entity::physicallyPossible: "
				"Entity %u starting point (%f,%f,%f) is not in current "
				"chunk %s but in chunk %s\n", id_,
				srcPos.x, srcPos.y, srcPos.z,
				pChunk_->identifier().c_str(),
				pCChunk->identifier().c_str());
		}
	}

	// make sure that their line is in the same chunk...
	if (!pChunk_->hasInternalChunks() && pChunk_->contains( dstPos ))
	{
		return true;
	}

	Chunk * pNewChunk = pChunk_->space()->findChunkFromPointExact( dstPos );
	if (pNewChunk == pChunk_)
	{
		return true;
	}

	//dprintf( "Changed chunks\n" );

	// ...or that it passes through a portal into another chunk
	Chunk * pLChunk = pChunk_;
	srcPos = pLChunk->transformInverse().applyPoint( srcPos );
	dstPos = pLChunk->transformInverse().applyPoint( dstPos );

	BW::vector< Chunk * > previousChunks;

	/* ok, ok let's just do a proper BFS to find out we can
	 * traverse through multiple chunks
	 */
	if (traverseChunks( pLChunk, pNewChunk, srcPos, dstPos, previousChunks ))
	{
		return true;
	}
	srcPos = pLChunk->transform().applyPoint( srcPos );
	dstPos = pLChunk->transform().applyPoint( dstPos );

	// There are rare extreme cases where at a common vertex point
	// shared by four adjacent chunks, floating point error may cause a
	// previous validated position becomes invalid. In this case, we give
	// entity a small nudge.
	// only for entity not on a vehicle now.
	// TODO: find a better solution?
	if (pVehicle == NULL &&
			dstPos == srcPos)
	{
		Vector3 ldir;
		ldir.setPitchYaw( localDirection_.pitch,
			localDirection_.yaw + 0.25 * MATH_PI );
		ERROR_MSG( "Entity::physicallyPossible(%u): unable to move. "
				"Nudge it bit.\n", id_ );
		localPosition_ += ldir * 0.01;
		this->updateGlobalPosition();
		// force a position correction by return false
	}
	else
	{
		DEBUG_MSG( "Entity::physicallyPossible(%u): "
			"entity attempted the impossible (%f,%f,%f) to (%f,%f,%f)\n",
			id_, srcPos.x, srcPos.y, srcPos.z, dstPos.x, dstPos.y, dstPos.z );
	}
	return false;
}

bool Entity::traverseChunks( Chunk * pCurChunk, const Chunk * pDstChunk,
			Vector3 cSrcPos, Vector3 cDstPos,
			BW::vector< Chunk * > & visitedChunks )
{
	bool status = false;
	bool revisitPreviousChunks = false;
	BW::vector< Chunk * > pendingChunks;

	if (pDstChunk == NULL)
		return false;

	if (pCurChunk == pDstChunk)
		return true;

	Chunk::piterator pit = pCurChunk->pbegin();
	Chunk::piterator pnd = pCurChunk->pend();
	for (; pit != pnd; pit++)
	{
		revisitPreviousChunks = false;
		const ChunkBoundary::Portal & portal = (*pit);

		if (!portal.hasChunk() || !portal.pChunk->isBound())
		{
			//dprintf( "Portal has no corresponding chunk or chunk is "
			//			"not bound\n";
			continue;
		}

		if (!portal.permissive)
		{
			//dprintf( "Rejected since not permissive\n" );
			continue;
		}

		// see if the line actually goes through this portal
		const PlaneEq & plane = portal.plane;
		float dsrc = plane.distanceTo( cSrcPos );
		float ddst = plane.distanceTo( cDstPos );

		if (dsrc*ddst > 0) continue;	// equiv test to above I hope

		Vector3 ipt3D = plane.intersectRay( cSrcPos, cDstPos-cSrcPos );
		ipt3D -= portal.origin;
		Vector2 ipt2D( portal.uAxis.dotProduct( ipt3D ),
			portal.vAxis.dotProduct( ipt3D ) );

		Vector2 fpt = portal.points.back();
		uint32 npts = portal.points.size();
		uint i;
		for (i = 0; i < npts; i++)
		{
			Vector2 tpt = portal.points[i];

			// see if ipt2D is 'inside' ftp -> tpt
			Vector2 perpdir( tpt.y-fpt.y, -(tpt.x-fpt.x) );
			if (perpdir.dotProduct( ipt2D-fpt ) > 0.f)
				 break;

			fpt = tpt;
		}
		if (i < npts)
		{
			/*
			dprintf( "Rejected since (%f,%f) on wrong side of edge %d\n",
				ipt2D.x, ipt2D.y, i );
			for (i = 0; i < npts; i++)
			{
				Vector2 tpt = portal.points[i];
				Vector2 perpdir( tpt.y-fpt.y, -(tpt.x-fpt.x) );
				dprintf( "fpt (%f,%f) tpt (%f,%f) => %f\n",
					fpt.x, fpt.y, tpt.x, tpt.y,
					perpdir.dotProduct( ipt2D-fpt ) );
				fpt = tpt;
			}
			*/
			continue;	// we didn't go through that portal, keep looking
		}

		// ok we went through that portal, excellent

		// make sure we are allowed to do so
		// TODO: could call a callback attached to this portal here
		//dprintf( "Accepted!\n" );

		// that is a normal transition then, we are ok
		if (portal.pChunk != pDstChunk)	// not there yet
		{
			Chunk * pNChunk = portal.pChunk;

			for (BW::vector<Chunk*>::iterator i = visitedChunks.begin();
					i != visitedChunks.end(); i++)
			{
				if (pNChunk == *i)
				{
					dprintf("hit previously visited chunk %s\n",
							pNChunk->identifier().c_str());
					revisitPreviousChunks = true;
					break;
				}
			}
			if (!revisitPreviousChunks)
			{
				pendingChunks.push_back( pNChunk );
			}
		}
		else
		{
			dprintf("Can reach target chunk %s from %s\n",
				pDstChunk->identifier().c_str(),
				pCurChunk->identifier().c_str());
			status = true;
			break;
		}
	}

	if (!(status || pendingChunks.empty()))
	{
		Vector3 srcPos;
		Vector3 dstPos;
		Chunk * pNChunk = NULL;

		visitedChunks.push_back( pCurChunk );

		for (BW::vector<Chunk*>::iterator iter = pendingChunks.begin();
				iter != pendingChunks.end(); iter++)
		{
			pNChunk = *iter;

			srcPos = pCurChunk->transform().applyPoint( cSrcPos );
			dstPos = pCurChunk->transform().applyPoint( cDstPos );

			srcPos = pNChunk->transformInverse().applyPoint( srcPos );
			dstPos = pNChunk->transformInverse().applyPoint( dstPos );

			dprintf( "traversing portal from %s to %s,target chunk %s",
				pCurChunk->identifier().c_str(),
				pNChunk->identifier().c_str(),
				pDstChunk->identifier().c_str());

			if (traverseChunks( pNChunk, pDstChunk, srcPos, dstPos, visitedChunks ))
			{
				status = true;
				break;
			}
		}
		visitedChunks.pop_back();
	}
	return status;
}

/*~	callback Entity onPassengerAlightAttempt
 *  @components{ cell }
 *	If present, this method is called on a vehicle when a entity
 *	controlled by a client attempts to get off it, if physics checking
 *	is enabled for that entity. If the method returns True, then the
 *	operation is permitted. If the method returns anything else,
 *	including an exception, then the operation is not permitted and
 *	a physics correction will be sent.
 *	The method may be called on a ghost vehicle, so it must take care
 *	to rely on only ghosted properties. Also, the position of the
 *	entity making the attempt must not be interfered with - all the
 *	method should do is return True or False.
 *	Note that the operation can still be failed at a number of points
 *	by later physics checks, even if your method returns True.
 */
/*~	callback Entity onPassengerBoardAttempt
 *  @components{ cell }
 *	If present, this method is called on a vehicle when a entity
 *	controlled by a client attempts to board it, if physics checking
 *	is enabled for that entity. If the method returns True, then the
 *	operation is permitted. If the method returns anything else,
 *	including an exception, then the operation is not permitted and
 *	a physics correction will be sent.
 *	The method may be called on a ghost vehicle, so it must take care
 *	to rely on only ghosted properties. Also, the position of the
 *	entity making the attempt must not be interfered with - all the
 *	method should do is return True or False.
 *	Note that the operation can still be failed at a number of points
 *	by later physics checks, even if your method returns True.
 *
 *	This method is not a good place to synchronise boarding of a
 *	single-seated vehicle, since it may be called simultaneously
 *	from two different cells. If you need to enforce such a rule,
 *	you should have a separate reservation step which sets a ghosted
 *	property to the (id of the) entity that has permission to ride it.
 *	This method can then check that the entity attempting to board
 *	does indeed hold the reservation.
 */
/**
 *	This method validates a change in vehicles coming from a client
 *	in an avatarUpdate message. It is only consulted when physics checks
 *	are enabled, and when the vehicles differ.
 */
bool Entity::validateAvatarVehicleUpdate( Entity * pNewVehicle )
{
	if (pVehicle_ != NULL)
	{
		Entity::nominateRealEntity( *pVehicle_ );
		PyObject * pMethod =
			PyObject_GetAttrString( pVehicle_, "onPassengerAlightAttempt" );
		if (pMethod == NULL)
		{
			PyErr_Clear();
			Entity::nominateRealEntityPop();
		}
		else
		{
			PyObject * result = Script::ask(
				pMethod, Py_BuildValue( "(O)", (PyObject*)this ),
				"Entity::validateAvatarVehicleUpdate(alight): " );
			Entity::nominateRealEntityPop();

			bool valid = false;
			Script::setAnswer( result, valid,
				"Entity::validateAvatarVehicleUpdate(alight) result" );
			if (!valid) return false;
		}
	}

	if (pNewVehicle != NULL)
	{
		Entity::nominateRealEntity( *pNewVehicle );
		PyObject * pMethod =
			PyObject_GetAttrString( pNewVehicle, "onPassengerBoardAttempt" );
		if (pMethod == NULL)
		{
			PyErr_Clear();
			Entity::nominateRealEntityPop();
		}
		else
		{
			PyObject * result = Script::ask(
				pMethod, Py_BuildValue( "(O)", (PyObject*)this ),
				"Entity::validateAvatarVehicleUpdate(board): " );
			Entity::nominateRealEntityPop();

			bool valid = false;
			Script::setAnswer( result, valid,
				"Entity::validateAvatarVehicleUpdate(board) result" );
			if (!valid) return false;
		}
	}

	return true;
}


/**
 *	Record the fact that the position has changed.
 *	Notify ghosts that our position and location have changed.
 *	Then notify the RealEntity object about it.
 *	And finally shuffle to the correct spot in the range list.
 *
 *	Note: The entity could be deleted by the time this call returns,
 *	if precautions against script calls are not taken. See the note
 *	about updateInternalsForNewPosition below.
 */
void Entity::updateInternalsForNewPositionOfReal( const Vector3 & oldPosition,
		bool isVehicleMovement )
{
	MF_ASSERT( this->isReal() );

	++volatileUpdateNumber_;

	CellAppInterface::ghostPositionUpdateArgs ghostArgs;

	ghostArgs.pos = Position3D( localPosition_ );
	ghostArgs.isOnGround = isOnGround_;

	// TODO: We could store the compressed pitch and yaw.
	ghostArgs.dir.set(
		localDirection_.yaw, localDirection_.pitch, localDirection_.roll );
	ghostArgs.updateNumber = volatileUpdateNumber_;

	RealEntity::Haunts::iterator iter = pReal_->hauntsBegin();

	while (iter != pReal_->hauntsEnd())
	{
		Mercury::Bundle & bundle = iter->bundle();

		CellAppInterface::ghostPositionUpdateArgs & rGhostPositionUpdate =
			CellAppInterface::ghostPositionUpdateArgs::start( bundle, id_ );

		rGhostPositionUpdate = ghostArgs;

		++iter;
	}

	Entity::callbacksPermitted( false ); // onNoise may be called.
	// Tell the real about it for velocity calcns, etc.
	pReal_->newPosition( globalPosition_ );

	if (this->cell().pReplayData())
	{
		this->cell().pReplayData()->queueEntityVolatile( *this );
	}

	// Update internals for new position
	this->updateInternalsForNewPosition( oldPosition, isVehicleMovement );
	Entity::callbacksPermitted( true );
}

/**
 *	This method is called when an Entity's position changes for any reason,
 *	and regardless of whether the Entity is a real or a ghost.
 *
 *	Note: Anything could happen to the entity over this call if it is a real,
 *  since its movement could trip a trigger, which could call script and
 *  offload or destroy this entity (or cancel any controller).
 */
void Entity::updateInternalsForNewPosition( const Vector3 & oldPosition,
		bool isVehicleMovement )
{
	MF_ASSERT_DEV( isValidPosition( globalPosition_ ) );

	START_PROFILE( SHUFFLE_ENTITY_PROFILE );

	// Make sure that no controllers are cancelled while doing this.
	// (And that no triggers are added/deleted/modified!)
	Entity::callbacksPermitted( false );

	// check if upper triggers should move first or lower ones
	bool increaseX = (oldPosition.x < globalPosition_.x);
	bool increaseZ = (oldPosition.z < globalPosition_.z);

	// shuffle the leading triggers
	for (Triggers::iterator it = triggers_.begin(); it != triggers_.end(); it++)
	{
		(*it)->shuffleXThenZExpand( increaseX, increaseZ,
									oldPosition.x, oldPosition.z );
	}

	// shuffle the entity
	pRangeListNode_->shuffleXThenZ( oldPosition.x, oldPosition.z );

	// shuffle the trailing triggers
	for (Triggers::reverse_iterator it = triggers_.rbegin();
			it != triggers_.rend(); it++)
	{
		(*it)->shuffleXThenZContract( increaseX, increaseZ,
										oldPosition.x, oldPosition.z );
	}
	// TODO: Even with this sorting this is broken if there is >1 trigger
	// with the same range :(

	// Note: trigger event calls must not modify any triggers.
	// Script calls can easily be delayed by use of the callbacksPermitted
	// mechanism invoked above (and below) ... just gotta make sure that
	// the C++ functions do not fiddle around with them.

	// Update the passenger's position and shuffle its triggers too.
	// There is a chance that the passenger could go out of and back
	// into the triggers of the vehicle (and vice versa), but since we
	// need to keep the range list in sorted order with all its virtual
	// functions, we have no other choice at this stage. It's no big deal
	// for the AoI but for proximity controllers it might be a pain.
	// Ideally it would go:
	// 1. expand all triggers including those of passengers
	// 2. move self and passengers
	// 3. contract all triggers including those of passengers
	if (Passengers::instance.exists( *this ))
	{
		Passengers::instance( *this ).
			updateInternalsForNewPosition( oldPosition );
	}

	// Check that the range list is sorted every so often
	static uint64 lastSortedCheck = timestamp();
	if (timestamp() - lastSortedCheck > stampsPerSecond())
	{
		lastSortedCheck = timestamp();
		if (!this->space().rangeList().isSorted())
		{
			ERROR_MSG( "Entity::updateInternalsForNewPosition(%u): "
				"Range list is not sorted after spot check\n",
					id_	);
			this->space().debugRangeList();
			MF_ASSERT( !"range list unsorted" );
		}
	}

	STOP_PROFILE( SHUFFLE_ENTITY_PROFILE );

	// check if you've crossed into a new Chunk
	this->checkChunkCrossing();

	// add a history event if we're not volatile at any distance, or if we
	// have a non-volatile position that has been updated
	if (!isVehicleMovement && (!this->volatileInfo().hasVolatile( 0.f ) ||
				(!this->volatileInfo().shouldSendPosition() &&
			 		(oldPosition != globalPosition_))))
	{
		this->addDetailedPositionToHistory( /* isLocalOnly */ true );
	}

	Entity::callbacksPermitted( true );

	this->onPositionChanged();
}


/**
 * This method checks if the entity has changed chunks. It checks if the
 * current position is still in the current chunk. If not it checks the
 * chunks that are connected to the current one through portals and finally
 * if it still doesn't find the chunk, it does a search through the hull tree
 * for the relevant column.
 */
void Entity::checkChunkCrossing()
{
	//ToDo: if chunks are not supported,
	//this function should not be called at all?
	if (!this->pChunkSpace()) {
		return;
	}

	const Vector3 offsetPosition( globalPosition_.x, globalPosition_.y + 0.1f,
		globalPosition_.z );

	// give it its first chunk if it isn't yet in one
	if (pChunk_ == NULL)
	{
		MF_ASSERT( this->nextInChunk() == NULL );
		MF_ASSERT( this->prevInChunk() == NULL );

		pChunk_ = this->pChunkSpace()->findChunkFromPointExact( offsetPosition );

		if (pChunk_ != NULL)
			CellChunk::instance( *pChunk_ ).addEntity( this );

		return;
	}

	bool stillInChunk = pChunk_->contains( offsetPosition );

	if (stillInChunk && !pChunk_->hasInternalChunks())
	{
/*		DEBUG_MSG( "(%f, %f, %f) is still in chunk %s\n",
				offsetPosition.x, offsetPosition.y, offsetPosition.z,
				pChunk_->identifier().c_str() );
*/
		return;
	}
	// new chunk that we want to be in
	Chunk * pNewChunk = NULL;

	//always check portals
	bool throughPortal = false;

	//this is to hold possible candidates that are outside
	//since indoor chunks have priority
	Chunk * pOutsideChunk = NULL;

	// ChunkBoundary::Portal * portalPassed = NULL;
	for (Chunk::piterator iter = pChunk_->pbegin();
		iter != pChunk_->pend();
		iter++)
	{
		ChunkBoundary::Portal & portal = (*iter);
		if (portal.hasChunk())
		{
			Chunk * chunk = portal.pChunk;
			if (chunk->contains( offsetPosition ))
			{
				throughPortal = true;
				// portalPassed = &portal;
				if (chunk->isOutsideChunk())
				{
					pOutsideChunk = chunk;
				}
				else
				{
					/*
					DEBUG_MSG( "Changed Chunks: Entity %d gone through portal [%s] from %s to inside %s (%6.3f, %6.3f, %6.3f)\n",
						(int)this->id(),
						portal.label.c_str(),
						pChunk_->identifier().c_str(),
						chunk->identifier().c_str(),
						globalPosition_.x,
						globalPosition_.y,
						globalPosition_.z
					);
					*/
					pNewChunk = chunk;
					break;
				}
			}
		}
	}
	if (throughPortal)
	{
		// found outside chunk but no inside chunks
		if (pNewChunk == NULL && pOutsideChunk != NULL)
		{
			pNewChunk = pOutsideChunk;
		}
		/*
		DEBUG_MSG( "Changed Chunks: Entity %d gone through portal [%s] from %s to %s (%6.3f, %6.3f, %6.3f)\n",
			(int)this->id(),
			portalPassed->label.c_str(),
			pChunk_->identifier().c_str(),
			pOutsideChunk->identifier().c_str(),
			globalPosition_.x,
			globalPosition_.y,
			globalPosition_.z
		);
		*/
	}
	else if(!stillInChunk)
	{
		// TODO: do this a better way.
		//if not found check the whole universe
		MF_ASSERT( pNewChunk == NULL );	// for sanity
		pNewChunk = this->pChunkSpace()->findChunkFromPointExact(
				offsetPosition );
		/*
		DEBUG_MSG( "Changed Chunks: Entity %d teleported or cheated from %s to %s (%6.3f, %6.3f, %6.3f)\n",
						(int)this->id(),
						pChunk_->identifier().c_str(),
						chunk->identifier().c_str(),
						globalPosition_.x,
						globalPosition_.y,
						globalPosition_.z
				);
		*/
#if 0
		if (pNewChunk == NULL)
		{
			ERROR_MSG("Entity::checkChunkCrossing: Attempt to cross into"
						"a non-existing chunk. Physics checking should be"
						"turned on for entity %d.\n", id_);
			return;
		}
#endif
	}

	// change chunks if they are different
	if (pNewChunk != NULL && pNewChunk != pChunk_)
	{
		CellChunk::instance( *pChunk_ ).removeEntity( this );
		pChunk_ = pNewChunk;
		CellChunk::instance( *pChunk_ ).addEntity( this );
	}
}


/**
 *	This method tell the entity entras that we have been relocated
 */
void Entity::relocated()
{
	// tell all our extras about this
	for (uint i = 0; i < s_entityExtraInfo().size(); i++)
	{
		if (extras_[i] != NULL) extras_[i]->relocated();
	}
}


/**
 *	This method handles the case where this entity is identified that this
 *	entity is a zombie and should be destroyed.
 */
void Entity::destroyZombie()
{
	if (this->isReal())
	{
		WARNING_MSG( "Entity::destroyZombie(%u): The zombie is real.\n", 
				this->id() );

		this->convertRealToGhost();
	}

	this->destroy();
}


/**
 *	This method reloads the script related to this entity.
 *	DEPRECATED
 */
void Entity::reloadScript()
{
	pEntityType_ = EntityType::getType( pEntityType_->typeID() );
	MF_ASSERT( pEntityType_ );

	if (PyObject_SetAttrString( this, "__class__",
			(PyObject *)pEntityType_->pPyType() ) == -1)
	{
		ERROR_MSG( "Entity::reloadScript(%u): "
			"Setting __class__ failed\n", id_ );
		PyErr_Print();
	}
}


/**
 *	This method migrates this entity to the new version of resources,
 *	in particular it switches it to use the new script class.
 */
bool Entity::migrate()
{
	AUTO_SCOPED_THIS_ENTITY_PROFILE;

	// get the new type and make sure it is the same id
	EntityTypePtr pOldType = pEntityType_;
	EntityTypePtr pNewType = EntityType::getType( pOldType->name() );
	if (!pNewType || (pNewType->typeID() != pOldType->typeID()))
	{
		WARNING_MSG( "Entity::migrate(%u): "
			"[type %s] has been made obsolete "
			"(absent or type id mismatch %d => %d)\n",
			id_, pOldType->name(), pOldType->typeID(),
				pNewType?pNewType->typeID():-1 );
		return false;
	}

	// Check that we're not simply reloading scripts.
	if (pNewType != pOldType)
	{
		// set our type, and set the old type for the new one if not yet done
		pEntityType_ = pNewType;
		pNewType->old( pOldType );
	}

	//TRACE_MSG( "Entity::migrate(%lu): Old PyType %08X New PyType %08X\n",
	//	id_, pOldType->pPyType(), pNewType->pPyType() );

	// see if we can't smoothly set in the script type
	PyObject * pNewEntity = this;
	if (PyObject_SetAttrString( this, "__class__",
			(PyObject *)pEntityType_->pPyType() ) == -1)
	{
		// set the class by some other means ... hmmm,
		// I think I need to ask Murf about this one
		// possibly create a new entity and replace our entry with it...
		// but for now just delete the entity (!)
		WARNING_MSG( "Entity::migrate(%u): "
			"[type %s] has been made obsolete "
			"(could not change __class__ to new class)\n",
			id_, pEntityType_->name() );
		PyErr_Print();
		pEntityType_ = pOldType;
		return false;
	}

	// TODO: could traverse properties and migrate them before or after migrate
	// call, or maybe provide a method to do so, and/or do it if no onMigrate
	// method.

	// call the migrate method if it has one
	Entity::nominateRealEntity( *(Entity*)pNewEntity );
	Script::call( PyObject_GetAttrString( pNewEntity, "onMigrate" ),
		Py_BuildValue("(O)",(PyObject*)this), "onMigrate", true/*okIfFnNull*/ );
	Entity::nominateRealEntityPop();

	// and we are done! sweet!
	return true;
}

/**
 *	This method is called when all entities have been migrated
 */
void Entity::migratedAll()
{
	AUTO_SCOPED_THIS_ENTITY_PROFILE;

	if (isDestroyed_) return;

	// call the onMigratedAll method if it has one
	Entity::nominateRealEntity( *this );
	Script::call( PyObject_GetAttrString( this, "onMigratedAll" ),
		PyTuple_New(0), "onMigratedAll", true/*okIfFnNull*/ );
	Entity::nominateRealEntityPop();
}


// -----------------------------------------------------------------------------
// Section: Entity - Message handlers
// -----------------------------------------------------------------------------

/**
 *	This method is called to handle an avatarUpdate message.
 */
void Entity::avatarUpdateImplicit(
	const CellAppInterface::avatarUpdateImplicitArgs & args )
{
	// TODO: There is a race-condition here. The baseapp is sent the modward
	// message when the controlledByRef is changed but there can still be stuff
	// on the wire which either has or will make it past the baseapp before
	// wards has been changed. We probably should put a check there, but we
	// have insufficient information in this context.
	//if (pReal_->controlledByRef().id != senderID)
	//{
	//	return;
	//}
	if (pReal_->controlledByRef().id == 0)
	{
		return;
	}

	// wait for the corrections to be acked
	if (physicsCorrections_ > 0)
	{
		return;
	}

	if (!isValidPosition( args.pos ))
	{
		ERROR_MSG( "Entity::avatarUpdateImplicit(%u): "
				"(%f,%f,%f) is not a valid position for this entity\n",
			id_, args.pos.x, args.pos.y, args.pos.z );
		return;
	}

	// if physics checking is enabled then make sure that move is possible
	if (!isZero( topSpeed_ ) &&
		!this->physicallyPossible( args.pos, pVehicle_ ))
	{
		// ok that made no sense, so tell the client where to go
		pReal_->sendPhysicsCorrection();
		return;
	}

	// now do common update
#if VOLATILE_POSITIONS_ARE_ABSOLUTE
	this->avatarUpdateCommon( args.pos, args.dir, isOnGround_, 0 );
#else /* VOLATILE_POSITIONS_ARE_ABSOLUTE */
	this->avatarUpdateCommon( args.pos, args.dir, isOnGround_, args.refNum );
#endif /* VOLATILE_POSITIONS_ARE_ABSOLUTE */
}


/**
 *	This method is called to handle an avatarUpdateOffGround message.
 */
void Entity::avatarUpdateExplicit(
		const CellAppInterface::avatarUpdateExplicitArgs & args )
{
	EntityID curVehicleID = this->vehicleID();

	if (!isValidPosition( args.pos ))
	{
		ERROR_MSG( "Entity::avatarUpdateExplicit(%u): "
				"(%f,%f,%f) is not a valid position for this entity\n",
			id_, args.pos.x, args.pos.y, args.pos.z );
		return;
	}

	const bool onGround = (args.flags & AVATAR_UPDATE_EXPLICT_FLAG_ONGROUND);

	// see comment in avatarUpdateImplicit
	//if (pReal->controlledByRef().id != senderID) return;
	if (pReal_->controlledByRef().id == 0)
	{
		return;
	}

	if (physicsCorrections_ > 0)
	{
		return;
	}

	bool physicsError = false;

	// if physics checking is enabled then make sure the move is possible
	if (!isZero( topSpeed_ ))
	{
		Entity * pUseVehicle = pVehicle_;
		if (args.vehicleID != curVehicleID)
			pUseVehicle = CellApp::instance().findEntity( args.vehicleID );
		physicsError = !this->physicallyPossible( args.pos, pUseVehicle );
	}

	// Change vehicles if it wants to be in a different vehicle
	if (!physicsError && args.vehicleID != curVehicleID)
	{
		// get off any vehicle we are currently on
		if (pVehicle_ != NULL)
		{
			if (!PassengerExtra::instance( *this ).alightVehicle( false ))
			{
				// just print it out
				PyErr_Print();
				PyErr_Clear();
				physicsError = true;
			}
		}

		// get on any vehicle we should now be on
		if (args.vehicleID != 0)
		{
			Entity::nominateRealEntity( *this );
			if (!PassengerExtra::instance( *this ).boardVehicle(
				args.vehicleID, false ))
			{
				// just print it out
				PyErr_Print();
				PyErr_Clear();
				physicsError = true;
			}
			Entity::nominateRealEntityPop();
		}

		if (this->pReal() && this->pReal()->pWitness())
		{
			this->pReal()->pWitness()->vehicleChanged();
		}
	}

	// if there was a problem with that move (including changing vehicles),
	// then let the client know where it stands (or rides...)
	if (physicsError)
	{
		pReal_->sendPhysicsCorrection();
		return;
	}

	// Then perform the common pose update
#if VOLATILE_POSITIONS_ARE_ABSOLUTE
	this->avatarUpdateCommon( args.pos, args.dir, onGround, 0 );
#else /* VOLATILE_POSITIONS_ARE_ABSOLUTE */
	this->avatarUpdateCommon( args.pos, args.dir, onGround, args.refNum );
#endif /* VOLATILE_POSITIONS_ARE_ABSOLUTE */
}


/**
 *	This method returns whether the position of this entity is volatile.
 */
bool Entity::hasVolatilePosition() const
{
	// TODO: This is true if -only direction- is volatile
	return this->volatileInfo().hasVolatile( 0.f );
}


/**
 *	This method is the common avatar update method. It is only called on real
 *	entities.
 */
void Entity::avatarUpdateCommon( const Position3D & pos, const YawPitchRoll & dir,
	bool onGround, uint8 refNum )
{
	static ProfileVal localProfile( "avatarUpdate" );
	START_PROFILE( localProfile );

	// if we're not volatile at any distance
	if (!this->hasVolatilePosition())
	{
		WARNING_MSG( "Entity::avatarUpdateCommon(%u): "
				"Moving an entity without volatile position. Should %s have a "
				"volatile position?\n",
			id_, this->pType()->name() );
	}

	// Record the information in this message
	localPosition_.set( pos.x, pos.y, pos.z );
	dir.get( localDirection_.yaw, localDirection_.pitch, localDirection_.roll );
	isOnGround_ = onGround;

#if VOLATILE_POSITIONS_ARE_ABSOLUTE
	// Update ghosts
	this->updateGlobalPosition();

#else /* VOLATILE_POSITIONS_ARE_ABSOLUTE */
	// Update ghosts
	bool isInGlobalCoords = this->updateGlobalPosition();

	// Notify of entity witness of new relative position reference
	if (this->pReal()->pWitness())
	{
		if (isInGlobalCoords)
		{
			this->pReal()->pWitness()->updateReferencePosition( refNum );
		}
		else
		{
			this->pReal()->pWitness()->cancelReferencePosition();
		}
	}
#endif /* VOLATILE_POSITIONS_ARE_ABSOLUTE */

	STOP_PROFILE_WITH_DATA( localProfile, pReal_->numHaunts() );
}


/**
 *	This method handles a message from the client. It indicates that the client
 *	has received and processed a forcedPosition message.
 */
void Entity::ackPhysicsCorrection()
{
	IF_NOT_MF_ASSERT_DEV( physicsCorrections_ > 0 )
	{
		ERROR_MSG( "Entity::ackPhysicsCorrection(%u): "
				"Unexpected physics correction acknowledgement from client "
				"or integer wraparound.\n",
			id_ );
		return;
	}

	if (--physicsCorrections_ == 0)
	{
		physicsLastValidated_ = timestamp();
		physicsNetworkJitterDebt_ = CellAppConfig::maxPhysicsNetworkJitter();
	}
}


/**
 *	This method is called to handle a message sent by an adjacent cell. It is
 *	called on a ghost entity when the adjacent cell wants to give ownership of a
 *	real entity from itself to this cell.
 */
void Entity::onload( const Mercury::Address & srcAddr,
		const Mercury::UnpackedMessageHeader & header,
		BinaryIStream & data )
{
	static ProfileVal localProfile( "onloadEntity" );
	SCOPED_PROFILE( TRANSIENT_LOAD_PROFILE );
	START_PROFILE( localProfile );

#ifdef DEBUG_FAULT_TOLERANCE
	if (g_crashOnOnload)
	{
		EntityPopulation::const_iterator iter = Entity::population().begin();

		while (iter != Entity::population().end())
		{
			Entity * pEntity = iter->second;
			DEBUG_MSG( "Entity::onload: "
					"id = %lu, isReal = %d, isDestroyed = %d, realAddr = %s\n",
				pEntity->id(),
				pEntity->isReal(),
				pEntity->isDestroyed(),
				pEntity->realAddr().c_str() );

			iter++;
		}

		MF_ASSERT( !"Entity::onload: Crash on onload" );
	}
#endif

	bool teleportFailure = false;
	data >> teleportFailure;

	int dataSize = data.remainingLength();

	MF_ASSERT( !this->isReal() );

	this->callback( "onEnteringCell" );

	this->convertGhostToReal( data, teleportFailure ? &srcAddr : NULL );

//	DEBUG_MSG( "Entity::onload( %d )\n", id_ );

	STOP_PROFILE( localProfile );

	uint64 duration = localProfile.lastTime();

	if (duration > g_profileOnloadTimeLevel)
	{
		outputEntityPerfWarning( "Profile onload/timeWarningLevel exceeded",
				*this, stampsToSeconds( duration ), dataSize );
	}
	else if (dataSize > g_profileOnloadSizeLevel)
	{
		outputEntityPerfWarning( "Profile onload/sizeWarningLevel exceeded",
				*this, stampsToSeconds( duration ), dataSize );
	}

	if (teleportFailure)
	{
		// Remove the haunt to the failed destination as the ghost did not get
		// created.
		RealEntity::Haunts::iterator iHaunt = pReal_->hauntsBegin();
		while (iHaunt != pReal_->hauntsEnd())
		{
			if (iHaunt->channel().addr() == srcAddr)
			{
				pReal_->delHaunt( iHaunt );
				break;
			}
			++iHaunt;
		}

		if (this->cell().pReplayData())
		{
			this->cell().pReplayData()->addEntityState( *this );
		}

		this->callback( "onTeleportFailure" );
	}
}


/**
 *	This method adds a createGhost message to a bundle.
 */
void Entity::createGhost( Mercury::Bundle & bundle )
{
	bundle.startMessage( CellAppInterface::createGhost );
	bundle << this->cell().space().id();
	this->writeGhostDataToStream( bundle );
}


/**
 *	This method converts a ghost entity into a real entity.
 */
void Entity::convertGhostToReal( BinaryIStream & data,
		const Mercury::Address * pBadHauntAddr )
{
	++numTimesRealOffloaded_;

	// Make sure the entity doesn't have a zero refcount when between lists.
	// Also if the entity destroys itself in this call. Actually, let's stop
	// script callbacks too so it can't do anything potentially even worse.
	EntityPtr pCopy = this;
	Entity::callbacksPermitted( false );

	// Throw away the channel to the (former) real entity
	MF_ASSERT( pRealChannel_ );
	pRealChannel_ = NULL;

	// do the work of creating the real entity
	this->readRealDataFromStreamForOnload( data, pBadHauntAddr );
	MF_ASSERT( this->isReal() );

	// add it to the cell's collection of reals
	this->cell().addRealEntity( this, /*shouldSendNow:*/true );

	if ((globalPosition_ != INVALID_POSITION) && this->cell().pReplayData())
	{
		// Teleports with invalid positions will be handled later after
		// onTeleportSuccess() has been called.

		this->cell().pReplayData()->addEntityState( *this );
	}

	this->relocated();

	// We want this to happen before other callbacks have a chance to run.
	{
		// We don't want to stop callbacks here but we also do not yet want to
		// replay any queued callbacks yet.
		s_callbackBuffer_.enableHighPriorityBuffering();

		this->callback( "onEnteredCell" );

		s_callbackBuffer_.disableHighPriorityBuffering();
	}

	// let the scripting environment have its way with the entity -
	// it could destroy it, offload (teleport) it, whatever.
	Entity::callbacksPermitted( true );
}


/**
 *	This method removes the information related to a real entity from the data
 *	stream.
 */
void Entity::readRealDataFromStreamForOnload( BinaryIStream & data,
		const Mercury::Address * pBadHauntAddr )
{
	CompressionIStream compressionStream( data );
	this->readRealDataFromStreamForOnloadInternal( compressionStream,
			pBadHauntAddr );
}


/**
 *	This method is called by readRealDataFromStreamForOnload once the decision
 *	on whether or not to uncompress has been made.
 */
void Entity::readRealDataFromStreamForOnloadInternal( BinaryIStream & data,
		const Mercury::Address * pBadHauntAddr )
{
	// We need to make sure that we remove the information from the bundle in
	// the same order as offload. First we do the Entity, then the script
	// related stuff and then the real stuff.

	// Now read off the real properties
	//pType->convertToRealScript( this, data );
	MF_ASSERT( properties_.size() == pEntityType_->propCountGhost() );
	properties_.insert( properties_.end(),
		pEntityType_->propCountGhostPlusReal()-properties_.size(),
		ScriptObject() );
	for (uint i = pEntityType_->propCountGhost(); i < properties_.size(); ++i)
	{
		DataDescription * pDataDescr = pEntityType_->propIndex( i );

		// TODO - implement component property processing logic here
		MF_ASSERT( !pDataDescr->isComponentised() );

		DataType & dt = *pDataDescr->dataType();
		ScriptDataSink sink;
		MF_VERIFY( dt.createFromStream( data, sink,
			/* isPersistentOnly */ false ) );
		ScriptObject value = sink.finalise();
		properties_[i] = dt.attach( value, &propertyOwner_, i );
		MF_ASSERT( properties_[i] );
	}

	TOKEN_CHECK( data, "RealProps" );

	// Finally take off the real entity data, and use that to create
	// a RealEntity instance
	this->createReal();

	MF_VERIFY(!pReal_->init( data, CREATE_REAL_FROM_OFFLOAD,
			/* channelVersion = */ Mercury::SEQ_NULL,
			pBadHauntAddr ));

	// Read any BASE_AND_CLIENT data exposed for recording.
	if (!this->readBasePropertiesExposedForReplayFromStream( data ))
	{
		ERROR_MSG( "Entity::readRealDataFromStreamForOnloadInternal"
				"(%u): Error streaming off exposed base entity "
				"properties\n", id_ );
	}

	// Start the real controllers now that pReal_ has been set
	this->startRealControllers();
}


/**
 *	This method calls this given callback on this entity with no arguments.
 */
void Entity::callback( const char * methodName )
{
	SCOPED_PROFILE( SCRIPT_CALL_PROFILE );
	AUTO_SCOPED_THIS_ENTITY_PROFILE;

	Entity::callback(
		methodName, PyTuple_New( 0 ), methodName, true /* okIfFnNull */ );
}


/**
 *	This method handles a message from an adjacent cell to update the state of
 *	a ghost.
 */
void Entity::ghostPositionUpdate(
		const CellAppInterface::ghostPositionUpdateArgs & args )
{
	AUTO_SCOPED_PROFILE( "ghostPositionUpdate" );

	volatileUpdateNumber_++;

	MF_ASSERT( !this->isReal() );

	localPosition_.set( args.pos.x, args.pos.y, args.pos.z );
	isOnGround_ = args.isOnGround;
	args.dir.get(
		localDirection_.yaw, localDirection_.pitch, localDirection_.roll );
	volatileUpdateNumber_ = args.updateNumber;

	this->updateGlobalPosition();
}



/**
 *	This method handles a message from the real entity associated with this
 *	entity. It adds a history event to this ghost.
 */
void Entity::ghostHistoryEvent( BinaryIStream & data )
{
	MF_ASSERT( !this->isReal() );

	++lastEventNumber_;
	EventNumber newNumber = this->eventHistory().addFromStream( data );

	MF_ASSERT( newNumber == lastEventNumber_ );
}


/**
 *	This method is called by an adjacent cell. It is used to notify this ghost
 *	that the location of the real entity has changed.
 */
void Entity::ghostSetReal( const CellAppInterface::ghostSetRealArgs & args )
{
	AUTO_SCOPED_PROFILE( "ghostSetReal" );
	NumTimesRealOffloadedType expected = numTimesRealOffloaded_ + 1;

	if (args.numTimesRealOffloaded != expected)
	{
		WARNING_MSG( "Entity::ghostSetReal( %u ): "
					"Invalid subsequence id. Expected %d. Got %d\n",
				id_, expected, args.numTimesRealOffloaded );

		BufferedGhostMessages & bufferedMessages =
			CellApp::instance().bufferedGhostMessages();
		BufferedGhostMessage * pMsg =
			BufferedGhostMessageFactory::createGhostSetRealMessage( id_, args );

		bufferedMessages.delaySubsequence( id_, args.owner, pMsg );
		return;
	}

	numTimesRealOffloaded_ = args.numTimesRealOffloaded;

	MF_ASSERT( !this->isReal() );
	MF_ASSERT( nextRealAddr_ == args.owner );

	pRealChannel_ = CellAppChannels::instance().get( args.owner );
	nextRealAddr_ = Mercury::Address::NONE;

	this->relocated();
}


/**
  *  This method is called by the real when it is about to offload.  It is used
  *  as an explicit sign-off prior to offloading so that each ghost can be sure
  *  when it is safe to start processing messages from the next real.
  */
void Entity::ghostSetNextReal(
	const CellAppInterface::ghostSetNextRealArgs & args )
{
	MF_ASSERT( nextRealAddr_ == Mercury::Address::NONE );
	MF_ASSERT( args.nextRealAddr != Mercury::Address::NONE );

	// This is the last message from our current real, so now we will only
	// accept GHOST_ONLY messages from nextRealAddr_.
	nextRealAddr_ = args.nextRealAddr;

	// Play any buffered messages for this ghost.
	CellApp::instance().bufferedGhostMessages().playSubsequenceFor( id_,
		   nextRealAddr_ );
}


/**
 *	This method handles a message from an adjacent cell. It used to delete a
 *	ghost.
 */
void Entity::delGhost()
{
	AUTO_SCOPED_PROFILE( "deleteGhost" );
	SCOPED_PROFILE( TRANSIENT_LOAD_PROFILE );

	MF_ASSERT( !this->isReal() );

	this->destroy();
}


/**
 *	This method handles a message from the real that informs us that our
 *	volatile info has changed.
 */
void Entity::ghostVolatileInfo(
		const CellAppInterface::ghostVolatileInfoArgs & args )
{
	volatileInfo_ = args.volatileInfo;
}


/**
 *	This method handles a message from the real that informs us that things
 *	have changed with the existence of our ghost controllers.
 */
void Entity::ghostControllerCreate( BinaryIStream & data )
{
	pControllers_->createGhost( data, this );
}


/**
 *	This method handles a message from the real that informs us that things
 *	have changed with the existence of our ghost controllers.
 */
void Entity::ghostControllerDelete( BinaryIStream & data )
{
	pControllers_->deleteGhost( data, this );
}


/**
 *	This method handles a message from the real that informs us that the
 *	data for one of our controllers has changed.
 */
void Entity::ghostControllerUpdate( BinaryIStream & data )
{
	pControllers_->updateGhost( data );
}


/**
 *	This method provides our controller collection with a visitor to be
 *	called for every controller associated with this entity.
 */
bool Entity::visitControllers( ControllersVisitor & visitor )
{
	AUTO_SCOPED_THIS_ENTITY_PROFILE;

	return pControllers_->visitAll( visitor );
}


/**
 *	This method handles a message used to set the capacity of a witness.
 */
void Entity::witnessCapacity(
		const CellAppInterface::witnessCapacityArgs & args )
{
	// TODO:PM For the moment, the only witness is the proxy.
	MF_ASSERT( args.witness == this->id() );

	if (pReal_ != NULL && pReal_->pWitness() != NULL)
	{
		pReal_->pWitness()->setWitnessCapacity( args.witness, args.bps );
	}
}


/**
 *	This method handles a message that is used to change property data on a
 *	ghost.
 */
void Entity::ghostedDataUpdate( BinaryIStream & data )
{
	MF_ASSERT( !this->isReal() );

	uint32 dataSize = data.remainingLength();
	int32 propertyIndex;
	data >> propertyIndex;

	const DataDescription * pDescription = 
		pEntityType_->description().property( propertyIndex );
	MF_ASSERT( pDescription != NULL );

	if (pDescription->isComponentised())
	{
		// TODO: Handle component stream
		CRITICAL_MSG( "Entity::ghostedDataUpdate: "
				"Unable to handle component update\n" );
	}
	else
	{
		ScriptObject pOldValue = ScriptObject::none();
		ScriptList pChangePath = ScriptList();

		bool isSlice = false;

		bool success = propertyOwner_.setPropertyFromInternalStream( data,
				&pOldValue, &pChangePath,
				pDescription->localIndex(),
				&isSlice );

		pDescription->stats().countReceived( dataSize );

		if (!success)
		{
			ERROR_MSG( "Entity::ghostedDataUpdate: Failed for %s.%s id = %d\n",
					pEntityType_->name(),
					pDescription->name().c_str(),
					id_ );
			return;
		}

		pDescription->callSetterCallback(
			ScriptObject( this, ScriptObject::FROM_BORROWED_REFERENCE ),
			pOldValue, pChangePath, isSlice );
	}
}


/**
 *	This method is called on a ghost entity when the aoiUpdateSchemeID of the
 *	real entity has been modified.
 */
void Entity::aoiUpdateSchemeChange(
			const CellAppInterface::aoiUpdateSchemeChangeArgs & args )
{
	aoiUpdateSchemeID_ = args.scheme;
}


/**
 *	This method is called on a real entity when an entity controlling us goes
 *	away.
 */
void Entity::delControlledBy(
		const CellAppInterface::delControlledByArgs & args )
{
	MF_ASSERT( this->isReal() );

	if (pReal_)
	{
		pReal_->delControlledBy( args.deadController );
	}
}


/**
 *  This method handles a Base entity packet that was forwarded here by a
 *  ghost.
 */
void Entity::forwardedBaseEntityPacket( BinaryIStream & data )
{
	Mercury::Address srcAddr;
	data >> srcAddr;
	CellApp::instance().interface().processPacketFromStream( srcAddr, data );
}


/**
 *	This method is called when this entity's base part has been offloaded to another
 *	BaseApp as part of BaseApp retirement.
 */
void Entity::onBaseOffloaded(
	const CellAppInterface::onBaseOffloadedArgs & args )
{
	// This message is REAL_ONLY
	MF_ASSERT( pReal_ );

	baseAddr_ = args.newBaseAddr;

	pReal_->channel().setAddress( baseAddr_ );

	DEBUG_MSG( "Entity( %s %u )::onBaseOffloaded: Now on baseapp %s\n",
			   pEntityType_->name(), id_, baseAddr_.c_str() );

	for (RealEntity::Haunts::iterator iter = pReal_->hauntsBegin();
		 iter != pReal_->hauntsEnd();
		 ++iter)
	{
		CellAppInterface::onBaseOffloadedForGhostArgs::start(
			iter->bundle(), this->id() ).newBaseAddr = baseAddr_;
	}
}


/**
 *	This method is called when this entity's real has been told that this
 *	entity's base part has been offloaded to another BaseApp as part of BaseApp
 *	retirement.
 */
void Entity::onBaseOffloadedForGhost(
	const CellAppInterface::onBaseOffloadedForGhostArgs & args )
{
	// This message is GHOST_ONLY
	MF_ASSERT( !pReal_ );

	baseAddr_ = args.newBaseAddr;
}


/**
 *	This method handles a message telling us to teleport to another space
 *	indicated by a cell mailbox.
 */
void Entity::teleport( const CellAppInterface::teleportArgs & args )
{
	MF_ASSERT( pReal_ );

	EntityPtr pThis = this;
	pThis->callback( "onTeleport" );

	if (pReal_)
	{
		pReal_->teleport( args.dstMailBoxRef );
	}
	else
	{
		ERROR_MSG( "Entity::teleport( %u ): "
					"No longer real after onTeleport callback\n",
				id_ );
	}
}


/**
 *	This method is called when this entity has successfully teleported.
 */
void Entity::onTeleportSuccess( Entity * pNearbyEntity )
{
	EntityPtr pThis = this;

	MF_ASSERT( s_pRealEntityArg == NULL );
	s_pRealEntityArg = pNearbyEntity;

	this->callback( "onTeleportSuccess",
			Py_BuildValue( "(O)", pNearbyEntity ? pNearbyEntity : Py_None ),
			"onTeleportSuccess: ", true );

	s_pRealEntityArg = NULL;

	// Destroyed by onTeleportSuccess
	if (!pThis->isReal())
	{
		return;
	}

	if (pThis->position() == Entity::INVALID_POSITION)
	{
		MF_ASSERT( pNearbyEntity );

		WARNING_MSG( "Entity::onTeleportSuccess( %u ): "
					"Entity position was not set in onTeleportSuccess. "
					"Setting to position of nearby entity %u\n",
				id_, pNearbyEntity->id() );
		this->setGlobalPosition( pNearbyEntity->position() );
	}
}


/**
 *	This method adds a history event with a detailed position.
 *
 *	@param isLocalOnly	If true, this history event will not be propagated as
 *						all ghosts will generate a local version. If false, then
 *						this history event is propagated to all haunts.
 *						May only be false if called on the Real entity.
 */
void Entity::addDetailedPositionToHistory( bool isLocalOnly )
{
	MF_ASSERT( isLocalOnly || this->isReal() );

	MemoryOStream stream;
	stream << localPosition_ << localDirection_;

	const Mercury::InterfaceElement & ie =
		ClientInterface::detailedPosition;

	const int msgStreamSize = ie.streamSize();

	if (isLocalOnly)
	{
		this->addHistoryEventLocally( ie.id(), stream,
			pEntityType_->detailedPositionDescription(), msgStreamSize );
	}
	else
	{
		pReal_->addHistoryEvent( ie.id(), stream,
			pEntityType_->detailedPositionDescription(), msgStreamSize );
	}

	// also tell ownClient directly if it isn't controlling us
	if ((pReal_ != NULL) &&
			(pReal_->pWitness() != NULL) &&
			(pReal_->controlledByRef().id != id_))
	{
		pReal_->pWitness()->sendToClient( this->id(), ie.id(), stream,
			msgStreamSize );

		pReal_->pWitness()->onSendReliablePosition( localPosition_,
			localDirection_ );
	}
}


/**
 *	This method is called to inform us that an entity is witnessing us. This
 *	can be called by a ghost to tell us that it is being witnessed or called
 *	internally by the real when we clear periodsWithoutWitness_.
 */
void Entity::witnessed()
{
	MF_ASSERT( this->isReal() );

	if (periodsWithoutWitness_ == NOT_WITNESSED_THRESHOLD)
	{
		Py_INCREF( Py_True );
		// ok script thought we were unwitnessed
		this->callback( "onWitnessed", Py_BuildValue( "(O)", Py_True ),
			"onWitnessed(True) callback: ", true );
	}

	periodsWithoutWitness_ = 0;
}

/**
 *	This method is called by the real to tell us that it is not being witnessed by any entity.
 *	This ghost entity now checks whether anything is witnessing it and will inform the real
 *	entity, if so.
 */
void Entity::checkGhostWitnessed()
{
	MF_ASSERT( !this->isReal() );
	periodsWithoutWitness_ = 2;
}


/**
 *	This method handles calls from other server component to run a method of
 *	this entity.
 */
void Entity::runScriptMethod( const Mercury::Address & srcAddr,
		const Mercury::UnpackedMessageHeader & header,
		BinaryIStream & data )
{
	uint16 methodID;
	data >> methodID;

	this->runMethodHelper( data, methodID, false, header.replyID, &srcAddr );
}

/**
 *  This method handles calls from a BaseViaCellMailBox. It calls the
 *  requested method on a base entity.
 */
void Entity::callBaseMethod( BinaryIStream & data )
{
	uint16 methodIndex;
	data >> methodIndex;

	if (baseAddr_.ip != 0)
	{
		PyEntityMailBox * pBaseEntityMailBox =
			new BaseEntityMailBox( pEntityType_, baseAddr_, id_ );
		if (pBaseEntityMailBox == NULL)
		{
			ERROR_MSG( "Entity::callBaseMethod(%u): "
						"Unable to create a base entity mailbox.\n", id_ );
			data.finish();
			return;
		}

		const MethodDescription * pDescription =
			pEntityType_->description().base().internalMethod( methodIndex );

		if (pDescription != NULL)
		{
			BinaryOStream * pBOS = pBaseEntityMailBox->getStream(
					*pDescription );
			if (pBOS == NULL)
			{
				ERROR_MSG( "Entity::callBaseMethod(%u): "
							"Failed to get stream on base method %s.\n",
							id_, pDescription->name().c_str() );
				data.finish();
				Py_DECREF( pBaseEntityMailBox );
				return;
			}
			pDescription->stats().countSentToBase( data.remainingLength() );
			pBOS->transfer( data, data.remainingLength() );
			pBaseEntityMailBox->sendStream();
		}
		Py_DECREF( pBaseEntityMailBox );
	}
	else
	{
		ERROR_MSG( "Entity::callBaseMethod(%u): "
				"No base address set.\n", id_ );
	}
}

/**
 *  This method handles calls from a ClientViaCellMailBox. It calls the
 *  requested method on a client entity.
 */
void Entity::callClientMethod( BinaryIStream & data )
{
	EntityID entityID;
	uint8 callingMode;
	uint16 methodIndex;

	data >> entityID;
	data >> callingMode;
	data >> methodIndex;

	const MethodDescription * pDescription =
		pEntityType_->description().client().internalMethod( methodIndex );

	if (pDescription != NULL)
	{
		MemoryOStream stream;
		stream.transfer( data, data.remainingLength() );

		this->sendToClient( entityID, *pDescription, stream,
			bool( callingMode & MSG_FOR_OWN_CLIENT ),
			bool( callingMode & MSG_FOR_OTHER_CLIENTS ),
			bool( callingMode & MSG_FOR_REPLAY ) );
	}
	else
	{
		ERROR_MSG( "Entity::callClientMethod(%u): "
					"No such client method index %hu for type %s.\n",
					id_, methodIndex, this->pType()->name() );
	}
}


/**
 *	This method handles the recording of client methods called from the base
 *	entity.
 */
void Entity::recordClientMethod( BinaryIStream & data )
{
	uint16 methodIndex = 0;
	data >> methodIndex;

	const MethodDescription * pDescription = pEntityType_->description().
		client().internalMethod( methodIndex );

	if (!pDescription)
	{
		ERROR_MSG( "Entity::recordClientMethod( %s %d ): "
				"Unknown method with internal index %hu\n",
			this->pType()->name(), id_, methodIndex );

		data.finish();
		return;
	}

	if (this->cell().pReplayData())
	{
		this->cell().pReplayData()->addEntityMethod( *this, *pDescription,
			data );
	}
	else
	{
		data.finish();
	}
}



/**
 *	This method handles the request of recording of exposed for
 *	replay BASE_AND_CLIENT properties from the base entity.
 */
void Entity::recordClientProperties( BinaryIStream & data )
{
	const EntityDescription & edesc = pEntityType_->description();

	exposedForReplayClientProperties_ = ScriptDict::create();

	edesc.readStreamToDict( data, EntityDescription::BASE_DATA,
			exposedForReplayClientProperties_ );

	MF_ASSERT( exposedForReplayClientProperties_.size() );

	ReplayDataCollector * pReplayData = this->cell().pReplayData();
	if (pReplayData)
	{
		pReplayData->addEntityProperties( * this,
				exposedForReplayClientProperties_ );
	}
}


/**
 *	This method handles calls from the client to run an exposed method.
 */
void Entity::runExposedMethod( BinaryIStream & data )
{
	MF_ASSERT( this->isReal() );

	uint8 methodID;
	data >> methodID;

	this->runMethodHelper( data, methodID, true );
}


/**
 *	This method is used to run a method on this entity that has come from the
 *	network.
 *
 *	@param data			Contains the parameters for the method call.
 *	@param methodID		The index number of the method.
 *	@param isExposed	Whether the methodID refers to the exposed subset.
 */
void Entity::runMethodHelper( BinaryIStream & data, int methodID,
		bool isExposed, int replyID, const Mercury::Address * pReplyAddr )
{
	static ProfileVal localProfile( "scriptMessage" );
	START_PROFILE( localProfile );

	MF_ASSERT( Entity::callbacksPermitted() );

	EntityID sourceID = 0;
	const MethodDescription * pMethodDescription = NULL;

	if (isExposed)
	{
		data >> sourceID;

		const ExposedMethodMessageRange & range =
			BaseAppExtInterface::Range::cellEntityMethodRange;

		pMethodDescription =
			pEntityType_->description().cell().exposedMethodFromMsgID( methodID,
					data, range );

		if (pMethodDescription != NULL &&
				pMethodDescription->isExposedToOwnClientOnly())
		{
			if (id_ != sourceID)
			{
				WARNING_MSG( "Entity::runMethodHelper. Blocked method %s "
							"on entity %u from client %u. Method is exposed "
							"as OWN_CLIENT.\n",
							pMethodDescription ?
							pMethodDescription->name().c_str() : "<not found>",
							id_, sourceID );

				data.finish();

				STOP_PROFILE( localProfile )
				return;
			}
		}
	}
	else
	{
		pMethodDescription =
			pEntityType_->description().cell().internalMethod( methodID );

		if (pMethodDescription->isExposed())
		{
			data >> sourceID;
		}
	}

	if (pMethodDescription != NULL)
	{
		Entity::nominateRealEntity( *this );
		{
			if (pMethodDescription->isComponentised())
			{
				MF_ASSERT( pEntityDelegate_ != NULL );
				pEntityDelegate_->handleMethodCall( *pMethodDescription,
						data, sourceID );
			}
			else
			{
				SCOPED_PROFILE( SCRIPT_CALL_PROFILE );
				pMethodDescription->callMethod(
						ScriptObject( this, ScriptObject::FROM_BORROWED_REFERENCE ),
						data, sourceID, replyID, pReplyAddr,
						&CellApp::instance().interface() );
			}
		}
		Entity::nominateRealEntityPop();
	}
	else
	{
		ERROR_MSG( "Entity::runMethodHelper(%u): "
					"Do not have%s method with type %d\n",
				id_,
				isExposed ? " exposed" : "",
				methodID );

				data.finish();
	}

	STOP_PROFILE_WITH_CHECK( localProfile )
	{
		WARNING_MSG( "\tin Entity::runExposedMethod(%u). Method = %s\n",
			id_, pMethodDescription ?
				pMethodDescription->name().c_str() : "<not found>" );
	}
}


/**
 *	This method is called by the proxy to add or delete a witness on
 *	this entity, so that updates will be sent to the client from its
 *	point of view.
 */
void Entity::enableWitness( const Mercury::Address & srcAddr,
		Mercury::UnpackedMessageHeader & header, BinaryIStream & data )
{
	MF_ASSERT( pReal_ != NULL );

	pReal_->enableWitness( data, header.replyID );
}


/**
 *	This method is called by the client in order to request an update for an
 *	entity that has recently entered its AoI.
 */
void Entity::requestEntityUpdate( BinaryIStream & data )
{
	MF_ASSERT( pReal_ != NULL );

	if (pReal_ != NULL && pReal_->pWitness() != NULL)
	{
		EntityID id;
		data >> id;

		int length = data.remainingLength();

		pReal_->pWitness()->requestEntityUpdate( id,
				(EventNumber *)data.retrieve( length ),
				length/sizeof(EventNumber) );
	}
}


// -----------------------------------------------------------------------------
// Section: Entity script related methods
// -----------------------------------------------------------------------------


/**
 *	This method returns whether or not this entity is real to scripts,
 *	i.e. whether or not script-related functions should treat this entity
 *	as real.
 */
bool Entity::isRealToScript() const
{
	return this->isReal() &&
		(!CellAppConfig::treatAllOtherEntitiesAsGhosts() ||
			 (s_pTreatAsReal == this) ||
			 (s_pTreatAsReal == NULL) ||
			 (s_pRealEntityArg == this));
}


/**
 *	This method is called when a property owned by this entity changes. It may
 *	be a top-level property, a property nested inside an array etc, or even a
 *	change in a slice of an array.
 */
bool Entity::onOwnedPropertyChanged( PropertyChange & change )
{
	const DataDescription * pDescription =
		pEntityType_->propIndex( change.rootIndex() );
	return this->onOwnedPropertyChanged( pDescription, change );
}

bool Entity::onOwnedPropertyChanged( const DataDescription * pDescription,
	PropertyChange & change )
{
	if (!this->isRealToScript())
	{
		PyErr_Format( PyExc_AttributeError, 
			"Can't change defined property %s.%s on ghost %d\n",
			this->pType()->name(), pDescription->name().c_str(), id_ );
		return false;
	}

	if (this->cell().pReplayData())
	{
		this->cell().pReplayData()->addEntityProperty( *this, *pDescription,
			change );
	}

	if (pDescription->isGhostedData())
	{
		// If the data is for other clients, add an event to our history.
		if (pDescription->isOtherClientData())
		{
			if (pDescription->shouldSendLatestOnly() &&
					change.isNestedChange())
			{
				WARNING_MSG( "Entity::onOwnedPropertyChanged(%u): "
						"%s.%s has SendLatestOnly enabled and was partially "
						"changed. Sending full property.\n",
					id_, this->pType()->name(),
					pDescription->name().c_str() );

				MF_VERIFY_DEV(
						propertyOwner_.changeOwnedProperty(
							properties_[ pDescription->localIndex() ],
							properties_[ pDescription->localIndex() ],
							*pDescription->dataType(),
							pDescription->localIndex(),
							/* forceChange: */ true ) );

				return true;
			}

			MemoryOStream stream;
			int streamSize = 0;

			Mercury::MessageID msgID =
				this->addChangeToExternalStream( change, stream,
						*pDescription, &streamSize );

			if (!pDescription->checkForOversizeLength( stream.size(), id_ ))
			{
				return false;
			}

			g_publicClientStats.trackEvent( pEntityType_->name(),
				pDescription->name(), stream.size(), streamSize );

			// Add history event for clients
			HistoryEvent * pEvent =
				pReal_->addHistoryEvent( msgID, stream,
					*pDescription, streamSize, pDescription->detailLevel() );

			propertyEventStamps_.set( *pDescription, pEvent->number() );
		}

		// Send the new data to all our ghosts
		RealEntity::Haunts::iterator iter = pReal_->hauntsBegin();

		while (iter != pReal_->hauntsEnd())
		{
			Mercury::Bundle & bundle = iter->bundle();
#if ENABLE_WATCHERS
			int oldBundleSize = bundle.size();
#endif
			bundle.startMessage( CellAppInterface::ghostedDataUpdate );

			bundle << this->id();
			bundle << int32( pDescription->index() );

			change.addToInternalStream( bundle );

#if ENABLE_WATCHERS
			pDescription->stats().countSentToGhosts( bundle.size() - oldBundleSize );
			pEntityType_->stats().countSentToGhosts( bundle.size() - oldBundleSize );
#endif

			++iter;
		}
	}

	// If the data is for our own client, add it to our bundle
	if (pDescription->isOwnClientData() && pReal_->pWitness() != NULL)
	{
		MemoryOStream stream;
		int streamSize = 0;

		Mercury::MessageID messageID = this->addChangeToExternalStream( change,
				stream, *pDescription, &streamSize );

		if (!pDescription->checkForOversizeLength( stream.size(), id_ ))
		{
			return false;
		}

		g_privateClientStats.trackEvent( pEntityType_->name(),
			pDescription->name(), stream.size(), streamSize );

#if ENABLE_WATCHERS
		pDescription->stats().countSentToOwnClient( stream.size() );
		pEntityType_->stats().countSentToOwnClient( stream.size() );
#endif
		pReal_->pWitness()->sendToClient( this->id(), messageID, stream,
				streamSize );
	}

	return true;
}


/**
 *	This method adds a property change for sending to a client.
 */
Mercury::MessageID Entity::addChangeToExternalStream(
		const PropertyChange & change, BinaryOStream & stream,
		const DataDescription & dataDescription, int * pStreamSize ) const
{
	const ExposedPropertyMessageRange & messageRange =
		ClientInterface::Range::entityPropertyRange;

	int16 msgID = messageRange.msgIDFromExposedID(
			dataDescription.clientServerFullIndex() );

	if ((msgID == -1) || change.isNestedChange())
	{
		const Mercury::InterfaceElement & ie =
			change.isSlice() ?
				ClientInterface::sliceEntityProperty :
				ClientInterface::nestedEntityProperty;

		msgID = ie.id();

		*pStreamSize = ie.streamSize();

		change.addToExternalStream( stream,
			dataDescription.clientServerFullIndex(),
			pEntityType_->propCountClientServer() );
	}
	else
	{
		*pStreamSize = dataDescription.streamSize();
		change.addValueToStream( stream );
	}

	return Mercury::MessageID( msgID );
}


/**
 *	This method is called recursively by child properties to find the top-level
 *	owner. If the entity is a ghost, NULL is returned.
 */
bool Entity::getTopLevelOwner( PropertyChange & change,
			PropertyOwnerBase *& rpTopLevelOwner )
{
	if (!this->isRealToScript())
	{
		DataDescription * pDescription =
			pEntityType_->propIndex( change.rootIndex() );

		PyErr_Format( PyExc_TypeError,
				"Modifying property %s on a ghost %s is not allowed",
				pDescription->name().c_str(),
				this->pType()->name() );
		return false;
	}

	rpTopLevelOwner = &propertyOwner_;
	return true;
}


/**
 *	This method gets the maximum value of a property index in this Entity
 *	instance. It is used to encode ChangePaths efficiently. -1 should be
 *	returned for an unknown number of divisions (use a self-formatting code).
 */
int Entity::getNumOwnedProperties() const
{
	//return pEntityType_->propCountGhostPlusReal();
	// this is only used when sending messages to the client.
	// therefore, we want to use its property space
	return pEntityType_->propCountClientServer();
}


/**
 *	This method is called to find out who the subowner with the given ref is.
 */
PropertyOwnerBase * Entity::getChildPropertyOwner( int ref ) const
{
	if (size_t( ref ) >= properties_.size())
	{
		return NULL;
	}

	PropertyOwnerBase * pVassal =
		pEntityType_->propIndex( ref )->dataType()->asOwner( properties_[ref] );
	return pVassal;
}


/**
 *	This method is called to change the given property.
 */
ScriptObject Entity::setOwnedProperty( int ref, BinaryIStream & data )
{
	DataDescription* pDataDesc = pEntityType_->propIndex( ref );
	DataType & dt = *pDataDesc->dataType();

	// reconstruct the python value from the stream
	ScriptDataSink sink;
	if (!dt.createFromStream( data, sink, /* isPersistentOnly */ false ) )
	{
		return ScriptObject();
	}

	ScriptObject pNewValue = sink.finalise();

	if (!pNewValue)
	{
		return pNewValue;
	}

	// detach the old value and attach the new one
	ScriptObject & pSlotRef = properties_[ref];
	ScriptObject pOldValue = pSlotRef;
	if (pSlotRef != pNewValue)		// hey, it could happen!
	{
		dt.detach( pSlotRef );
		pSlotRef = dt.attach( pNewValue, &propertyOwner_, ref );
	}

	return pOldValue;
}


/**
 *	Quick property access method.
 */
ScriptObject 
Entity::propertyByDataDescription( const DataDescription * pDataDescr ) const
{
	MF_ASSERT( pDataDescr != NULL );

	if (pDataDescr->isComponentised())
	{
		MF_ASSERT( pEntityDelegate_ );

		ScriptObject result = PyComponent::getAttribute( 
				*pEntityDelegate_,
				*pDataDescr,
				/* isPersistentOnly */ false );

		if (!result)
		{
			Script::printError();
		}

		return result;
	}

	int localIndex = pDataDescr->localIndex();
	if (uint(localIndex) < properties_.size())
	{
		return properties_[localIndex];
	}

	return ScriptObject();
}


/**
 *	This method is responsible for getting script attributes associated with
 *	this object.
 */
ScriptObject Entity::pyGetAttribute( const ScriptString & attrObj )
{
	PROFILER_SCOPED( Entity_pyGetAttribute );
	const char * attr = attrObj.c_str();

	if (isDestroyed_ &&
		strcmp( attr, "isDestroyed" ) != 0 &&
		strcmp( attr, "id" ) != 0 &&
		attr[0] != '_')
	{
		PyErr_Format( PyExc_TypeError, "Entity.%s cannot be accessed since "
			"%s entity id %d is destroyed", attr, pEntityType_->name(), id_ );
		return ScriptObject();
	}

	bool treatAsReal = this->isRealToScript();

	const MethodDescription * const pMethod =
		pEntityType_->description().cell().find( attr );

	if (pMethod)
	{
		// ghost methods return an object that calls the real
		if (!treatAsReal)
		{
			return ScriptObject( new RealCaller( this, pMethod ),
				ScriptObject::FROM_NEW_REFERENCE );
		}
		else if (pEntityDelegate_ && pMethod->isComponentised())
		{
			return ScriptObject( 
				new DelegateEntityMethod( pEntityDelegate_, pMethod, id_ ),
				ScriptObject::FROM_NEW_REFERENCE );
		}
	}

	// see if it is a property of this entity
	DataDescription * pDescription = pEntityType_->description( attr,
															/*component*/"");

	if (pDescription != NULL && !isDestroyed_ && pDescription->isCellData())
	{
		if (!treatAsReal && !pDescription->isGhostedData())
		{
			PyErr_Format( PyExc_TypeError, "Entity.%s cannot be accessed since "
				"%s entity id %d is a ghost", attr, pEntityType_->name(), id_);
			return ScriptObject();
		}

		int localIndex = pDescription->localIndex();
		if (uint(localIndex) < properties_.size())
		{
			return properties_[localIndex];
		}
	}

	// see if a mailbox is sought
	if ((streq( attr, "client" ) || streq( attr, "ownClient" )))
	{
		if (!pReal_ || pReal_->pWitness())
		{
			return ScriptObject( new PyClient( *this, *this, true, false ),
				ScriptObject::FROM_NEW_REFERENCE );
		}
		else
		{
			PyErr_Format( PyExc_AttributeError,
				"Entity %d does not have a witness and so no attribute %s",
				id_, attr );
			return ScriptObject();
		}
	}
	else if (streq( attr, "otherClients" ))
	{
		return ScriptObject( new PyClient( *this, *this, false, true ),
			ScriptObject::FROM_NEW_REFERENCE );
	}
	else if (streq( attr, "allClients" ))
	{
		return ScriptObject( new PyClient( *this, *this,
				/*isForOwn:*/ !pReal_ || pReal_->pWitness(),
				/* isForOthers:*/ true ),
			ScriptObject::FROM_NEW_REFERENCE );
	}

	if (streq( attr, "base" ) && (baseAddr_.ip != 0))
	{
		return ScriptObject(
			new BaseEntityMailBox( pEntityType_, baseAddr_, id_ ),
			ScriptObject::FROM_NEW_REFERENCE );
	}

	// Try the controller constructors
	if (treatAsReal && attr[0] == 'a' && attr[1] == 'd' && attr[2] == 'd')
	{
		PyObject * ret = Controller::factory( this, attr+3 );
		if (ret != NULL)
		{
			return ScriptObject( ret, ScriptObject::FROM_NEW_REFERENCE );
		}
	}

	ScriptObject pResult = PyObjectPlus::pyGetAttribute( attrObj );

	// Check if this is a ghost and a valid method to call.

	if (pResult && !this->isRealToScript() && PyMethod_Check( pResult.get() ))
	{
		bool isGhost = false;
		ScriptObject pIsGhost = pResult.getAttribute( "isGhost",
			ScriptErrorClear() );

		if (pIsGhost)
		{
			isGhost = pIsGhost.isTrue( ScriptErrorClear() );
		}

		if (!isGhost)
		{
			WARNING_MSG( "Entity::pyGetAttribute: "
						"Accessing method '%s' on ghost entity %d that is not "
						"decorated with @bwdecorators.callableOnGhost",
					attr, int( id_ ) );

			if (CellAppConfig::enforceGhostDecorators())
			{
				PyErr_Format( PyExc_AttributeError,
						"Method '%s' is not callable on ghost entity %d",
						attr, int( id_ ) );
				return ScriptObject();
			}
		}
	}

	return pResult;
}


/**
 *	This method is responsible for setting script attributes associated with
 *	this object.
 */
bool Entity::pySetAttribute( const ScriptString & attrObj,
	const ScriptObject & value )
{
	const char * attr = attrObj.c_str();
	if (!this->isRealToScript())
	{
		if (attr[0] != '_')
		{
			PyErr_Format( PyExc_TypeError,
				"Attempted to set Entity.%s of %s %s entity id %d ",
				attr,
				this->isDestroyed() ? "destroyed" : "ghost",
				pEntityType_->name(), id_ );
			return false;
		}

		// if it's not real then only let the base class do stuff.
		// not sure if we should even allow this in fact...
		// (required by updater to set __class__ on ghosts at the moment)
		return this->PyObjectPlus::pySetAttribute( attrObj, value );
	}

	// see if it's one of the entity's properties
	DataDescription * pDescription = pEntityType_->description( attr, 
															/*component*/"" );

	if (pDescription != NULL && pDescription->isCellData())
	{
		if (pDescription->isIdentifier())
		{
			NOTICE_MSG( "Identifier property %s.%s was changed for entity %d "
					"(this new value may already exist in the database, "
					"causing database writes to fail later)\n",
				pEntityType_->name(), pDescription->name().c_str(),
				id_ );
		}

		int cellIndex = pDescription->localIndex();
		ScriptObject pOldValue = properties_[cellIndex];

		DataType & dataType = *pDescription->dataType();

		if (!propertyOwner_.changeOwnedProperty(
				properties_[ cellIndex ],
				value, dataType, cellIndex ))
		{
			if (!PyErr_Occurred())
			{
				WARNING_MSG( "Entity::pySetAttribute(%d): "
						"Attempted to set %s.%s to incorrect type - %s.\n",
					id_,
					pEntityType_->name(),
					pDescription->name().c_str(),
					value.typeNameOfObject() );

				PyErr_Format( PyExc_TypeError,
					"Attempted to set Entity.%s of %s entity %d to wrong type",
					attr, pEntityType_->name(), id_ );
			}
			return false;
		}

		return true;
	}

	const StringSet<ConstPolicy> & attributeSet =
		s_buildEntityDictInitTimeJob.attributeSet();

	if (attributeSet.find( attr ) == attributeSet.end() &&
		!pEntityType_->description().isTempProperty( attr ))
	{
		WARNING_MSG( "Entity::pySetAttribute(%u): "
				"%s.%s is not a def file property or temp property. "
				"This will be lost if offloaded\n",
			id_, pEntityType_->name(), attr );
	}

	// finally let the base class have the scraps
	return this->PyObjectPlus::pySetAttribute( attrObj, value );
}


/**
 *	Get additional members of entities. i.e. extras
 */
void Entity::pyAdditionalMembers( const ScriptList & pList ) const
{
	if (this->isRealToScript())
	{
		pReal_->pyAdditionalMembers( pList );
	}
}

/**
 *	Get additional methods of entities. i.e. extras
 */
void Entity::pyAdditionalMethods( const ScriptList & pList ) const
{
	Controller::factories( pList, "add" );

	if (this->isRealToScript())
	{
		pReal_->pyAdditionalMethods( pList );
	}
}


/**
 *	This method returns the ID of the space this entity is currently in.
 */
SpaceID Entity::spaceID() const
{
	return pSpace_->id();
}


/**
 *	This method gets the id of the space this entity is currently in
 */
PyObject * Entity::pyGet_spaceID()
{
	if (pSpace_)
	{
		return Script::getData( this->spaceID() );
	}
	else
	{
		PyErr_Format( PyExc_TypeError, "%d has no space%s.",
				int(id_), isDestroyed_ ? ", it is destroyed": "" );
		return NULL;
	}
}


/**
 *	This method implements the script's get accessor for aoiUpdateScheme.
 */
PyObject * Entity::pyGet_aoiUpdateScheme()
{
	BW::string name;

	if (!AoIUpdateSchemes::getNameFromID( aoiUpdateSchemeID_, name ))
	{
		PyErr_Format( PyExc_ValueError,
				"Entity %d has invalid scheme id (%d)",
				id_, int( aoiUpdateSchemeID_ ) );
		return NULL;
	}

	return Script::getData( name );
}


/**
 *	This method implements the script's set accessor for aoiUpdateScheme.
 */
int Entity::pySet_aoiUpdateScheme( PyObject * value )
{
	if (!this->isRealToScript())
	{
		PyErr_SetString( PyExc_TypeError,
			"This attribute is only available on a real entity." );
		return -1;
	}

	BW::string schemeName;

	int retVal = Script::setData( value, schemeName, "aoiUpdateScheme" );

	if (retVal == 0)
	{
		if (!AoIUpdateSchemes::getIDFromName( schemeName, aoiUpdateSchemeID_ ))
		{
			PyErr_Format( PyExc_ValueError,
					"Invalid scheme name %s", schemeName.c_str() );
			return -1;
		}

		for (RealEntity::Haunts::iterator iter = pReal_->hauntsBegin();
			 iter != pReal_->hauntsEnd();
			 ++iter)
		{
			CellAppInterface::aoiUpdateSchemeChangeArgs::start(
				iter->bundle(), this->id() ).scheme = aoiUpdateSchemeID_;
		}
	}

	return retVal;
}

BW_END_NAMESPACE

#include "pyscript/script_math.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This class is used to expose an entity's position as a writable Python
 *	Vector3. If the returned Vector3's values are changed, the associated
 *	entity is updated appropriately.
 *
 *	For example, entity.position.y += 1.0 works.
 */
class PyEntityVector3 : public PyVectorRef< Vector3 >
{
public:
	PyEntityVector3( Entity * pOwner,
			Vector3 * pVector,
			void (Entity::*pF)( const Vector3 & ) ) :
		PyVectorRef< Vector3 >( pOwner, pVector ),
		pEntity_( pOwner ),
		pF_( pF )
	{
	}

	virtual bool setVector( const Vector3 & v )
	{
		if (!pEntity_->isReal()) // Includes destroyed
		{
			PyErr_SetString( PyExc_ValueError,
					"Can only move real entities" );
			return false;
		}

		(pEntity_.get()->*pF_)( v );
		return true;
	}

private:
	EntityPtr pEntity_;
	void (Entity::*pF_)( const Vector3 & );
};


/**
 *	This method implements the script's get accessor to position.
 */
PyObject * Entity::pyGet_position()
{
	return new PyEntityVector3( this, &globalPosition_,
			&Entity::setGlobalPosition );
}


/**
 *	This method implements the script's set accessor to position.
 */
int Entity::pySet_position( PyObject * value )
{
	if (!this->isRealToScript())
	{
		PyErr_SetString( PyExc_TypeError,
			"This attribute is only available on a real entity." );
		return -1;
	}

	Vector3 newPosition;

	int retVal = Script::setData( value, newPosition, "position" );

	if (retVal == 0)
	{
		if (isValidPosition( newPosition ))
		{
			this->setGlobalPosition( newPosition );
		}
		else
		{
			PyErr_SetString( PyExc_ValueError, "Invalid position." );
			retVal = -1;
		}
	}

	return retVal;
}


/**
 *	This utility method sets a new global position and updates related values.
 */
void Entity::setGlobalPosition( const Vector3 & newPosition )
{
	if (globalPosition_ != newPosition)
	{
		Vector3 oldPosition( globalPosition_ );
		globalPosition_ = newPosition;
		this->updateLocalPosition();
		this->updateInternalsForNewPositionOfReal( oldPosition );

		pReal_->sendPhysicsCorrection();
	}
}


/**
 *	This method implements the script's get accessor to direction.
 */
PyObject * Entity::pyGet_direction()
{
	return new PyEntityVector3( this,
			reinterpret_cast< Vector3 * >( &globalDirection_ ),
			&Entity::setGlobalDirection );
}


/**
 *	This method implements the script's set accessor to position.
 */
int Entity::pySet_direction( PyObject * value )
{
	if (!this->isRealToScript())
	{
		PyErr_SetString( PyExc_TypeError,
			"This attribute is only available on a real entity." );
		return -1;
	}

	Vector3 newValue;

	int retVal = Script::setData( value, newValue, "direction" );

	if (retVal == 0)
	{
		this->setGlobalDirection( Vector3( Angle( newValue.x ),
					Angle( newValue.y ), Angle( newValue.z ) ) );
	}

	return retVal;
}


/**
 *	This accessor sets the direction attribute for Python
 */
void Entity::setGlobalDirection( const Vector3 & newDir )
{
	if ((const Vector3&)globalDirection_ != newDir )
	{

		(Vector3&)globalDirection_ = newDir;
		// TODO: check that all components are in range
		// (especially when it's used by vehicles for a transform)

		this->updateLocalPosition();
		this->updateInternalsForNewPositionOfReal( globalPosition_ );

	}
}


/**
 *	This method sets the isOnGround flag. It propagates it to other entities
 *	but doesn't end up doing a shuffle so no script callbacks are made.
 */
void Entity::isOnGround( bool isOnGround )
{
	if (isOnGround_ != isOnGround)
	{
		isOnGround_ = isOnGround;
		this->updateInternalsForNewPositionOfReal( globalPosition_ );
	}
}

/**
 *	This method gets the velocity of a real entity for Python
 */
PyObject * Entity::pyGet_velocity()
{
	if (!this->isRealToScript())
	{
		PyErr_SetString( PyExc_TypeError,
			"This attribute is only available on a real entity." );
		return NULL;
	}

	return Script::getData( pReal_->velocity() );
}

/**
 *	This method is exposed to scripting. It should only be called by avatars
 *	that have an associated client. It sends a message to that client.
 *	Assumes that 'args' are valid according to the MethodDescription.
 *
 *	@param description	The description of the method to send.
 *	@param argStream	A MemoryOStream containing the destination entity
 *						ID and arguments for the method.
 *	@param isForOwn		A boolean flag indicates whether message should be
 *						sent to the client associated with the entity.
 *	@param isForOthers	A boolean flag indicates whether message should be
 *						send to all other clients apart from the client
 *						associated with the entity.
 *	@param isExposedForReply
 *						This determines whether the method call will be
 *						recorded, if a recording is active.
 */
bool Entity::sendToClient( EntityID entityID,
		const MethodDescription & description, MemoryOStream & argStream,
		bool isForOwn, bool isForOthers,
		bool isExposedForReplay )
{
	if (pReal_ == NULL)
	{
		return false;
	}

	if (isForOthers)
	{
		g_publicClientStats.trackEvent( pEntityType_->name(),
			description.name(), argStream.size(),
			description.streamSize( true ) );

		pReal_->addHistoryEvent( description.exposedMsgID(), argStream,
				description, description.streamSize( true ),
				description.priority() );
	}

	if (isForOwn)
	{
		if (pReal_->pWitness() == NULL)
		{
			return false;
		}

		argStream.rewind();
		g_privateClientStats.trackEvent( pEntityType_->name(),
			description.name(), argStream.size(),
			description.streamSize( true ) );
		description.stats().countSentToOwnClient( argStream.size() );
		pEntityType_->stats().countSentToOwnClient( argStream.size() );

		pReal_->pWitness()->sendToClient( entityID,
			description.exposedMsgID(), argStream,
			description.streamSize( true ) );
	}

	if (isExposedForReplay && this->cell().pReplayData())
	{
		argStream.rewind();
		this->cell().pReplayData()->addEntityMethod( *this, description,
			argStream );
	}

	return true;
}


/**
 *	This method is exposed to scripting. It is used by an entity to destroy
 *	itself.
 *
 *	@param args	An empty Python tuple
 */
PyObject * Entity::py_destroy( PyObject * /*args*/ )
{
	if (this->isDestroyed())
	{
		PyErr_Format( PyExc_TypeError,
				"Entity %d is already destroyed",
				int(this->id()) );
		return NULL;
	}

	if (!this->isRealToScript())
	{
		PyErr_Format( PyExc_TypeError,
				"Tried to destroy ghost entity %d",
				int(this->id()) );
		return NULL;
	}

	// If we are in __init__ do not allow destroy to be called.
	if (pReal_->removalHandle() == NO_ENTITY_REMOVAL_HANDLE)
	{
		PyErr_Format( PyExc_ValueError,
				"Tried to call Entity.destroy() on %d from inside __init__. "
					"Consider using a timer callback.",
				int(this->id()) );
		return NULL;
	}

	this->destroy();

	Py_RETURN_NONE;
}


/**
 *	This method is exposed to scripting. It is used by an entity to cancel
 *	a previously registered controller. The arguments below are passed via
 *	a Python tuple.
 *
 *	@param args		A tuple containing the ID of the previously registered controller.
 *
 *	@return		A new reference to PyNone on success, otherwise NULL.
 */
PyObject * Entity::py_cancel( PyObject * args )
{
	if (!this->isRealToScript())
	{
		PyErr_SetString( PyExc_TypeError,
				"Entity.cancel() not available on ghost entities" );
		return NULL;
	}

	return pControllers_->py_cancel( args, this );
}


/**
 *	This method is exposed to scripting. It returns whether or not this entity
 *	is real.
 */
PyObject * Entity::py_isReal( PyObject * /*args*/ )
{
	return PyBool_FromLong( this->isReal() );
}


/**
 *	This method is exposed to scripting. It returns whether or not this entity
 *	is real as far as scripting is concerned.
 */
PyObject * Entity::py_isRealToScript( PyObject * /*args*/ )
{
	return PyBool_FromLong( this->isRealToScript() );
}


/**
 *	This method is exposed to scripting. It returns an object that is used to
 *	call methods on an entity that resides on the client machine associated
 *	with this entity.
 */
PyObject * Entity::py_clientEntity( PyObject * args )
{
	if (!this->isRealToScript())
	{
		PyErr_SetString( PyExc_TypeError,
				"Entity.clientEntity not available on ghost entities" );
		return NULL;
	}

	EntityID destID;

	if (!PyArg_ParseTuple( args, "i", &destID ))
	{
		PyErr_SetString( PyExc_TypeError, "Expected 1 integer argument." );
		return NULL;
	}

	Entity * pDest = CellApp::instance().findEntity( destID );

	if (pDest == NULL)
	{
		PyErr_Format( PyExc_ValueError, "Could not find entity %d.",
				int( destID ) );
		return NULL;
	}

	return new PyClient( *this, *pDest, true, false );
}


/**
 *	This method is exposed to scripting. It changes the volatile info associated
 *	with this object.
 */
int Entity::pySet_volatileInfo( PyObject * value )
{
	if (!this->isRealToScript())
	{
		PyErr_SetString( PyExc_TypeError,
			"This attribute can only be set on a real entity." );
		return -1;
	}

	VolatileInfo newInfo;
	int retVal =
		Script::setData( value, newInfo, "volatileInfo" );

	if (retVal == 0)
	{
		this->setVolatileInfo( newInfo );
	}

	return retVal;
}


/**
 *	This method is used to change the volatile info associated with this entity.
 *	This includes changing it on the ghosts of this entity.
 */
void Entity::setVolatileInfo( const VolatileInfo & newInfo )
{
	MF_ASSERT( this->isReal());
	if (!this->isReal())
	{
		return;
	}

	if (newInfo == volatileInfo_)
	{
		return;
	}

	if (newInfo.isLessVolatileThan( volatileInfo_ ))
	{
		this->addDetailedPositionToHistory( /* isLocalOnly */ false );
	}

	volatileInfo_ = newInfo;

	RealEntity::Haunts::iterator iter = pReal_->hauntsBegin();

	while (iter != pReal_->hauntsEnd())
	{
		Mercury::Bundle & bundle = iter->bundle();

		CellAppInterface::ghostVolatileInfoArgs & rGhostVolatileInfo =
			CellAppInterface::ghostVolatileInfoArgs::start( bundle, id_ );

		rGhostVolatileInfo.volatileInfo = volatileInfo_;

		++iter;
	}
}


/**
 *	This method reduces the entity into a pickled form. In fact it makes
 *	it appear exactly like a mailbox.
 */
PyObject * Entity::pyPickleReduce()
{
	EntityMailBoxRef embr;
	PyEntityMailBox::reduceToRef( this, &embr );

	PyObject * pConsArgs = PyTuple_New( 1 );
	PyTuple_SET_ITEM( pConsArgs, 0,
		PyString_FromStringAndSize( (char*)&embr, sizeof(embr) ) );

	return pConsArgs;
}


/**
 *	This method handles a request to write this entity to the database.
 */
void Entity::writeToDBRequest( const Mercury::Address & srcAddr,
		Mercury::UnpackedMessageHeader & header, BinaryIStream & /*stream*/ )
{
	AUTO_SCOPED_PROFILE( "writeToDB" );
	this->sendDBDataToBase( &srcAddr, header.replyID );
}


/**
 *	This method is used to write this entity to the database. This is called
 *	from script.
 *
 *	@return True if successful, otherwise false. On failure, the Python error
 *	state is set.
 */
bool Entity::writeToDB()
{
	AUTO_SCOPED_THIS_ENTITY_PROFILE;

	if (!pEntityType_->description().isPersistent())
	{
		PyErr_Format( PyExc_TypeError,
			"Entities of type %s cannot be persisted."
				"See <Persistent> in %s.def.",
			pEntityType_->name(), pEntityType_->name() );
		return false;
	}

	if (!this->isRealToScript())
	{
		PyErr_SetString( PyExc_TypeError,
			"This method can only be called on a real entity." );
		return false;
	}

	if (!this->hasBase())
	{
		PyErr_SetString( PyExc_TypeError,
			"This method can only be called on a real entity with a base." );
		return false;
	}

	this->sendDBDataToBase();
	return true;
}


/**
 *	This method sends database information.
 *
 *	@param pReplyAddr If not NULL, this is assumed to be a reply to a
 *		writeToDBRequest call.
 *	@param replyID The reply id to use if pReplyAddr is not NULL.
 */
bool Entity::sendDBDataToBase( const Mercury::Address * pReplyAddr,
		Mercury::ReplyID replyID )
{
	MF_ASSERT( Entity::callbacksPermitted() );

	// TODO: Move this to RealEntity
	MF_ASSERT( this->pReal() );

	bool isToBaseEntity = !pReplyAddr || (*pReplyAddr == this->baseAddr());

	// If sending to the base entity, use the entity channel, otherwise just use
	// an anonymous channel.
	Mercury::ChannelSender sender( isToBaseEntity ?
		pReal_->channel() :	CellApp::getChannel( *pReplyAddr ) );

	Mercury::Bundle & bundle = sender.bundle();

	Entity::nominateRealEntity( *this );
	Script::call( PyObject_GetAttrString( this, "onWriteToDB" ),
			PyTuple_New( 0 ), "onWriteToDB", true );
	Entity::nominateRealEntityPop();

	if (pReplyAddr == NULL)
	{
		bundle.startMessage( BaseAppIntInterface::writeToDB );
	}
	else
	{
		bundle.startReply( replyID );
	}

	pReal_->writeBackupProperties( bundle );

	return true;
}


/**
 *	This method sends a cell entity lost message to the base.
 */
bool Entity::sendCellEntityLostToBase()
{
	// TODO: Move this to RealEntity
	MF_ASSERT( this->pReal() );

	Mercury::ChannelSender sender( pReal_->channel() );

	Mercury::Bundle & bundle = sender.bundle();

	bundle.startMessage( BaseAppIntInterface::cellEntityLost );

	const EntityDescription & description = pEntityType_->description();

	ScriptDict tempDict = this->createDictWithProperties( 
			EntityDescription::CELL_DATA );

	if (!tempDict)
	{
		CRITICAL_MSG( "Entity::sendCellEntityLostToBase(%u): "
				"Failed to add attributes to dictionary\n",
			id_ );
		return false;
	}

	if (!description.addDictionaryToStream(	tempDict, bundle,
			EntityDescription::CELL_DATA ))
	{
		CRITICAL_MSG( "Entity::sendCellEntityLostToBase(%u): "
				"Failed to add dictionary to stream\n",
			id_ );
		return false;
	}

	bundle << this->position() << this->direction() << this->space().id();

	return true;
}


/**
 *	This method is used to handle a message to destroy this entity.
 */
void Entity::destroyEntity(
		const CellAppInterface::destroyEntityArgs & /*args*/ )
{
	this->destroy();
}


// -----------------------------------------------------------------------------
// Section: Entity - range functions
// -----------------------------------------------------------------------------

/**
 *	This method collects the set of entities within a specified range. It
 *	searches through the range list until it reaches a node with a larger
 *	position.
 */
bool Entity::getEntitiesInRange( EntityVisitor & visitor,
		float range, ScriptObject pClass, ScriptObject pActualPos )
{
	float rangeSqr = range * range;
	float xDist = 0.f;

	EntityTypePtr pType = NULL;
	if (pClass)
	{
		if (PyString_Check( pClass.get() ))
		{
			char * pTypeStr = PyString_AsString( pClass.get() );
			pType = EntityType::getType( pTypeStr );

			if (pType == NULL)
			{
				PyErr_Format( PyExc_ValueError,
					"'%s' is not a valid entity type", pTypeStr );

				return false;
			}
		}
		else if (pClass.get() != Py_None)
		{
			PyErr_SetString( PyExc_TypeError,
				"Expected None or a string argument to indicate the entity type" );
			return false;
		}
	}

	Vector3 testPos;
	if (pActualPos && (pActualPos.get() != Py_None))
	{
		if (Script::setData( pActualPos.get(), testPos, "actualPosition" ) != 0)
		{
			return false;
		}
	}
	else
	{
		testPos = globalPosition_;
	}

	RangeListNode* pRLN = pRangeListNode_->nextX();

	while ((pRLN != NULL) && (xDist <= range))
	{
		if (pRLN->isEntity())
		{
			Entity * pEntity = EntityRangeListNode::getEntity( pRLN );
			Vector3 entityPos = pEntity->position();

			xDist = entityPos.x - testPos.x;
			if (!pType || (pType == pEntity->pType()))
			{
				if (ServerUtil::distSqrBetween( testPos, entityPos ) <= rangeSqr)
				{
					visitor.visit( pEntity );
				}
			}
		}
		pRLN = pRLN->nextX();
	}

	pRLN = pRangeListNode_->prevX();

	xDist = 0.f;
	while ((pRLN != NULL) && (xDist <= range))
	{
		if (pRLN->isEntity())
		{
			Entity * pEntity = EntityRangeListNode::getEntity( pRLN );
			Vector3 entityPos = pEntity->position();

			xDist = testPos.x - entityPos.x;
			if (!pType || (pType == pEntity->pType()))
			{
				if (ServerUtil::distSqrBetween( testPos, entityPos ) <= rangeSqr)
				{
					visitor.visit( pEntity );
				}
			}
		}
		pRLN = pRLN->prevX();
	}

	return true;
}


/**
 *	This method allow script to get a component
 */
PyObject * Entity::getComponent( const BW::string & name )
{
	if (!pEntityType_->description().hasComponent( name ))
	{
		PyErr_Format( PyExc_ValueError,
			"Unable to find component %s", name.c_str() );
		return NULL;
	}
	return new PyCellComponent( pEntityDelegate_, id_, name, 
		&pEntityType_->description() );
}

/**
 *	This method allows script to destroy a space.
 *
 *	@return Whether we were allowed to destroy the space
 */
bool Entity::destroySpace()
{
	AUTO_SCOPED_THIS_ENTITY_PROFILE;

	if ( CellAppConfig::useDefaultSpace() && this->space().id() == 1)
	{
		PyErr_Format( PyExc_ValueError,
			"destroySpace called on entity %d in default space", int(id_) );
		return false;
	}
	this->space().requestShutDown();
	return true;
}


/**
 *	This method allows script access to range queries.
 *
 *	@return A Python list of entities within range.
 */
PyObject * Entity::entitiesInRange( float range, ScriptObject pClass,
								   ScriptObject pActualPos )
{
	AUTO_SCOPED_PROFILE( "entitiesInRange" );

	if (isDestroyed_)
	{
		PyErr_Format( PyExc_TypeError,
			"entitiesInRange called on destroyed entity %d", int(id_) );
		return NULL;
	}

	class LocalVisitor : public EntityVisitor
	{
	public:
		LocalVisitor() :
			pList_( PyList_New( 0 ), ScriptObject::STEAL_REFERENCE ) {}
		virtual ~LocalVisitor() {}

		void visit( Entity * pEntity )
		{
			PyList_Append( pList_.get(), pEntity );
		}

		PyObject * getList()
		{
			Py_INCREF( pList_.get() );
			return pList_.get();
		}

	private:
		ScriptObject pList_;
	};

	LocalVisitor visitor;

	if (!this->getEntitiesInRange( visitor, range, pClass, pActualPos ))
	{
		return NULL;
	}

	return visitor.getList();
}


/**
 *	This methods adds the given trigger to those that move with us.
 */
void Entity::addTrigger( RangeTrigger * pTrigger )
{
	AUTO_SCOPED_THIS_ENTITY_PROFILE;

	Triggers::iterator it;

	for (it = triggers_.begin(); it != triggers_.end(); ++it)
	{
		if ((*it)->range() <= pTrigger->range())
		{
			break;
		}
	}

	triggers_.insert( it, pTrigger );
}


/**
 *	Notify us that the given trigger has had its range modified
 */
void Entity::modTrigger( RangeTrigger * pTrigger )
{
	// ideally would do a bubble-sort kind of operation
	this->delTrigger( pTrigger );
	this->addTrigger( pTrigger );
}


/**
 *	Remove the given trigger from those that move with us
 */
void Entity::delTrigger( RangeTrigger * pTrigger )
{
	AUTO_SCOPED_THIS_ENTITY_PROFILE;

	Triggers::iterator found = std::find(
		triggers_.begin(), triggers_.end(), pTrigger );
	MF_ASSERT( found != triggers_.end() );
	triggers_.erase( found );
}


// -----------------------------------------------------------------------------
// Section: Debugging
// -----------------------------------------------------------------------------


PyObject * Entity::py_debug( PyObject * /*args*/ )
{
	if (pRangeListNode_)
	{
		pRangeListNode_->debugRangeX();
		pRangeListNode_->debugRangeZ();
	}

	Py_RETURN_NONE;
}


void Entity::debugDump()
{
	INFO_MSG( "\tEntity %u. refcnt = %" PRIzd ":\n",
			this->id(), this->ob_refcnt );

	if (pReal_ != NULL)
	{
		pReal_->debugDump();
	}

	DEBUG_MSG( "chunk = %s, prev=0x%p, this=0x%p, next=0x%p\n",
				(pChunk_ ? pChunk_->identifier().c_str() : "Not in Chunk"),
				pPrevInChunk_,
				this,
				pNextInChunk_ );
}


/**
 *	This method associates a controller with this entity. It adds the controller
 *	to the entity and starts it.
 *	This method can only be called on a real entity.
 *
 *	@param pController	The controller to associate with this entity.
 *	@param userArg		An argument that is sent with the call when the
 *						controller is triggered.
 *
 *	@return A unique identifier for the controller that can be used in the call
 *		to delController.
 *	@see delController
 */
ControllerID Entity::addController( ControllerPtr pController, int userArg )
{
	AUTO_SCOPED_THIS_ENTITY_PROFILE;

	static ControllerID movementID =
		Controller::getExclusiveID( "Movement", false );

	// if we're not volatile at any distance, warn about movement controllers
	if (!this->hasVolatilePosition() &&
			(pController->exclusiveID() == movementID))
	{
		WARNING_MSG( "Entity::addController(%u): Adding movement controller to "
				"an entity without volatile position. Should %s have a "
				"volatile position?\n",
			id_, this->pType()->name() );
	}

	MF_ASSERT( this->isReal() );
	MF_ASSERT( !this->isDestroyed() );

	if (!this->isRealToScript())
	{
		WARNING_MSG( "Entity::addController(%u): "
				"Adding a %s (%d) to ghost entity.\n",
			id_, pController->typeName(), pController->type() );
	}

	IF_NOT_MF_ASSERT_DEV( !this->isDestroyed() )
	{
		// PyErr_Format( PyExc_TypeError,
		ERROR_MSG(
				"Entity::addController(%u): added to destroyed entity: "
				"controllerId = %d type=%s-%d, userArg = %d\n",
			id_, int(pController->id()),
			pController->typeName(), pController->type(), userArg );
		return 0;
	}

	return pControllers_->addController( pController, userArg, this );
}


/**
 *	This method updates this ghost part of the given controller on any
 *	ghosts that the entity may have.
 *	This method can only be called on a real entity.
 *
 *	@param pController		The controller to update
 */
void Entity::modController( ControllerPtr pController )
{
	AUTO_SCOPED_THIS_ENTITY_PROFILE;

	MF_ASSERT( this->isReal() );
	pControllers_->modController( pController, this );
}


/**
 *	This method removes the controller with the given id. It stops it first.
 *	This method can be called on either a real or a ghost entity.
 *
 *	@param id		The ID of the controller to remove.
 *	@param warnOnFailure If true, a warning will be generated on failure.
 *	@return			True if a controller was removed, false otherwise.
 */
bool Entity::delController( ControllerID id, bool warnOnFailure )
{
	AUTO_SCOPED_THIS_ENTITY_PROFILE;

	return pControllers_->delController( id, this, warnOnFailure );
}


/**
 *	This method is called on us by the cell chunk to let us know we have
 *	been removed from its chunk
 */
void Entity::removedFromChunk()
{
	pPrevInChunk_ = NULL;
	pNextInChunk_ = NULL;
	pChunk_ = NULL;
}


/**
 *	This method propagates noise outdoors
 */
bool Entity::outdoorPropagateNoise( float range,
								int event,
								int info )
{
	AUTO_SCOPED_THIS_ENTITY_PROFILE;

	class LocalVisitor : public EntityVisitor
	{
	public:
		virtual ~LocalVisitor() {}
		void visit( Entity * pEntity )
		{
			entities_.push_back( pEntity );
		}

		typedef BW::vector< EntityPtr > Entities;
		Entities entities_;
	};

	LocalVisitor visitor;

	if (!this->getEntitiesInRange( visitor, range ))
	{
		return false;
	}

	for (LocalVisitor::Entities::iterator iter = visitor.entities_.begin();
			iter != visitor.entities_.end();
			++iter)
	{
		Entity * pEntity = iter->get();

		float dist = (float)sqrt(ServerUtil::distSqrBetween( this->position(), pEntity->position()));
		if (dist < range)
		{
			pEntity->heardNoise( this, range, dist, event,  info );
		}
	}

	return true;
}


/**
 *	This method is exposed to scripting. It is used by an entity to
 *	make a noise which is propagated to all other entities in range.
 *
 *	@param noiseLevel	This is how much noise is made in relative energy.
 *	@param event		The event number that will be passed to onNoise script
 *						method.
 *	@param info			This is an optional info/event field.
 *
 *	@return false if this is not a real entity.
 *
 *	@note  The Python error state is set on failure
 */
bool Entity::makeNoise( float noiseLevel, int event, int info )
{
	AUTO_SCOPED_THIS_ENTITY_PROFILE;

	float propagationRange =
				noiseLevel * NoiseConfig::standardRange();
	if (this->isRealToScript())
	{
		if (pChunk_ != NULL)
		{
			if( pChunk_->isOutsideChunk())
			{
				this->outdoorPropagateNoise( propagationRange,event, info );
			}
			else
			{
				CellChunk::instance( *pChunk_ ).propagateNoise( this,
					propagationRange, globalPosition_,
					propagationRange, event, info );
			}
		}

		return true;
	}
	else
	{
		PyErr_SetString( PyExc_TypeError,
			"This method can only be called on a real entity." );
		return false;
	}

}


/**
 *	This method is called when a noise is heard by an entity.
 *	It calls the script callback "onNoise"
 *
 *	@param who This is the entity that made the noise
 *	@param propRange	This is how far the noise should propagate.
 *	@param distance This is how far away the noise was made
 *	@param event	The event argument that will be passed to onNoise.
 *	@param info This is an info field
 */
void Entity::heardNoise( const Entity* who,
						 float propRange,
						 float distance,
						 int event,
						 int info )
{
	/*
	DEBUG_MSG( "Entity:heardNoise Entity %d heard a noise by %d. "
		"Distance %f, propRange %f, event=%d, info=%d\n",
		id(), who->id(), distance, propRange, event, info );
	*/
	SCOPED_PROFILE( SCRIPT_CALL_PROFILE );
	AUTO_SCOPED_THIS_ENTITY_PROFILE;

	this->callback( "onNoise", Py_BuildValue( "(Offii)",
		who, propRange, distance, event, info ), "onNoise callback: ", true );
}


/**
 *	This method returns whether this entity is in an indoor chunk.
 *
 *	@return True if the entity is indoors, otherwise false if outdoors or in no
 *	chunk.
 */
bool Entity::isIndoors() const
{
	return pChunk_ && !pChunk_->isOutsideChunk();
}


/**
 *	This method returns whether this entity is in an outdoor chunk.
 *
 *	@return True if the entity is outdoors, otherwise false if indoors or in no
 *	chunk.
 *	@note It is possible for an outdoor chunk to be loaded before all indoor
 *	chunks are loaded. In this case, True may be returned when False is expected.
 */
bool Entity::isOutdoors() const
{
	return pChunk_ && pChunk_->isOutsideChunk();
}


/**
 *
 *	This method is exposed to scripting. It is used by an entity to work out
 *	its position on the ground. If there are no collidable objects or terrain
 *	below the entity, then the entity position is returned.
 *
 *	@return the location on the ground.
 */
const Position3D Entity::getGroundPosition() const
{
	if (this->pChunkSpace()) {
		return this->pChunkSpace()->findDropPoint( this->position() );
	}
	//ToDo: change when findDropPoint() is supported on all physical spaces
	WARNING_MSG( "Entity::getGroundPosition(): "
			"Physical space of this type does not support findDropPoint(), "
			"returning entity position\n");
	return this->position();
}


/**
 *	This method is exposed to scripting. It is used by an entity to register
 *	a timer function to be called with a given period. The arguments below
 *	are passed via a Python tuple.
 *
 *	@param entityId		ID of entity to track
 *	@param velocity		max turning velocity
 *	@param period		The period in ticks that the controller should
 *						recalculate.
 *	@param userArg		User data to be passed to the callback
 *
 *	@return		The integer ID of the newly created controller.
 */
PyObject * Entity::trackEntity( int entityId, float velocity,
		int period, int userArg )
{
	if (!this->isRealToScript())
	{
		PyErr_SetString( PyExc_TypeError,
			"This method can only be called on a real entity." );
		return NULL;
	}

	ControllerPtr pController =
		new FaceEntityController( entityId, velocity, period );
	ControllerID controllerID = this->addController(pController, userArg);

	return Script::getData( controllerID );
}


/**
 *	This method implements the Python method. See Python documentation for
 *	details.
 */
bool Entity::setPortalState( bool isOpen, WorldTriangle::Flags collisionFlags )
{
	if (!this->isRealToScript())
	{
		PyErr_SetString( PyExc_TypeError,
			"This method can only be called on a real entity." );
		return false;
	}

	ControllerPtr pController =
		new PortalConfigController( isOpen, collisionFlags );
	if (this->addController( pController, 0 ) == 0)
	{
		PyErr_SetString( PyExc_ValueError,
			"Failed to add PortalConfigController" );
		return false;
	}

	return true;
}


/**
 *	This method creates a dictionary representation of this entity's attributes.
 */
PyObject * Entity::getDict()
{
	ScriptDict pDict = ScriptDict::create();
	EntityTypePtr pType = this->pType();
	int count = pType->propCountGhostPlusReal();

	for (int i = 0; i < count; ++i)
	{
		const BW::string & propName = pType->propIndex( i )->name();

		// TODO: Can we do without creating a string?
		ScriptObject pValue = this->pyGetAttribute( 
			ScriptString::create( propName.c_str() ) );

		if (pValue)
		{
			pDict.setItem( propName.c_str(), pValue,
				ScriptErrorPrint( "Entity::getDict: " ) );
		}
		else
		{
			Script::clearError();
		}
	}

	return pDict.newRef();
}


/**
 *	This static method returns a vector of EntityExtraInfo.
 */
BW::vector<EntityExtraInfo*> & Entity::s_entityExtraInfo()
{
	static BW::vector<EntityExtraInfo*> entityExtraInfo;
	return entityExtraInfo;
}


/**
 *	Register a type of entity extra.
 *
 *	They must all be registered before the first entity is created.
 */
int Entity::registerEntityExtra(
	PyMethodDef * pMethods, PyGetSetDef * pAttributes )
{
	EntityExtraInfo * eei = NULL;
	if (pMethods || pAttributes)
	{
		eei = new EntityExtraInfo();
		eei->pMethods_ = pMethods;
		eei->pAttributes_ = pAttributes;
	}

	s_entityExtraInfo().push_back( eei );

	return s_entityExtraInfo().size()-1;
}


/**
 *	This method returns the cell that this entity resides on.
 */
Cell & Entity::cell()
{
	MF_ASSERT( pSpace_ != NULL );
	MF_ASSERT( pSpace_->pCell() != NULL );
	// TODO: Maybe put this back in. The problem with this is that it can be
	// called by an entity which is in the process of becoming real. This is
	// valid.
	// MF_ASSERT( this->isReal() );
	return *(pSpace_->pCell());
}


/**
 *	This method returns the cell that this entity resides on.
 */
const Cell & Entity::cell() const
{
	MF_ASSERT( (pSpace_ != NULL) && (pSpace_->pCell() != NULL) );
	MF_ASSERT( this->isReal() );
	return *(pSpace_->pCell());
}


/**
 *	This method returns the ChunkSpace that this entity is in.
 */
ChunkSpace * Entity::pChunkSpace() const
{
	return pSpace_->pChunkSpace() ? pSpace_->pChunkSpace().get() : NULL;
}


/**
 *	This method sets the vehicle that this entity is currently travelling on,
 *	or NULL for no vehicle.
 *
 *	@param pVehicle	The vehicle that this entity is boarding.
 *	@param keepWho	If KEEP_GLOBAL_POSITION, the global position is kept and the
 *		local position is recalculated. If KEEP_LOCAL_POSITION, the local
 *		position is kept and the global position is calculated from this.
 *		If IN_LIMBO, then the actual vehicle is no longer available.
 *		If anything else, then it is a Position3D
 */
void Entity::setVehicle( Entity * pVehicle, SetVehicleParam keepWho )
{
	if (pVehicle_ != pVehicle)
	{
		pVehicle_ = pVehicle;

		++vehicleChangeNum_;

		if (keepWho == KEEP_GLOBAL_POSITION)
		{
			// if we're in the range list already then keeping the global
			// position is a fine idea
			if (removalHandle_ != NO_SPACE_REMOVAL_HANDLE)
			{
				this->updateLocalPosition();

				// Notify the witness that the vehicle state has changed. This
				// is necessary to avoid an issue where dismounting from a
				// vehicle while being a server controlled entity doesn't push
				// a setVehicle update to the client.
				if (this->pReal() && this->pReal()->pWitness())
				{
					this->pReal()->pWitness()->vehicleChanged();
				}
			}
			// ... but if we're not in the list yet then we must be a ghost
			// that has just been created - in which case our local position
			// is in fact already correct but our global position is wrong
			else
			{
				// however we don't want to / can't call updateGlobalPosition
				// because it assumes that we're in the range list ... so just
				// perform the necessary calculations right here
				MF_ASSERT( Passengers::instance.exists( *pVehicle_ ) );
				const Matrix & m = Passengers::instance( *pVehicle_ ).transform();
				globalPosition_ = m.applyPoint( localPosition_ );
				// TODO: What do we do here? For now, just doing yaw.
				globalDirection_.yaw =
					Angle( localDirection_.yaw + pVehicle_->direction().yaw );
			}
		}
		else if (keepWho == KEEP_LOCAL_POSITION)
		{
			// MF_ASSERT( !this->isReal() );
			this->updateGlobalPosition( /* shouldUpdateGhosts */ false,
				/* isVehicleMovement */ true );
		}
		else /* keepWho >= IN_LIMBO */
		{
			MF_ASSERT( !this->isReal() );
		}

		this->onPositionChanged();
	}
}


/**
 *	This private method updates the local position and direction of this
 *	entity based on the global position and direction.
 */
void Entity::updateLocalPosition()
{
	// figure out where we stand now
	if (pVehicle_ == NULL)
	{
		localPosition_ = globalPosition_;
		localDirection_ = globalDirection_;
	}
	else
	{
		MF_ASSERT( Passengers::instance.exists( *pVehicle_ ) );
		Matrix m;
		m.invert( Passengers::instance( *pVehicle_ ).transform() );
		localPosition_ = m.applyPoint( globalPosition_ );
		// TODO: What do we do here? For now, just doing yaw.
		localDirection_.yaw =
			Angle( globalDirection_.yaw - pVehicle_->direction().yaw );
	}

	// and since the global position has not changed this is all we have to do
}

/**
 *	This private method updates the global position and direction of this
 *	entity based on the local position and direction.
 */
bool Entity::updateGlobalPosition( bool shouldUpdateGhosts,
		bool isVehicleMovement )
{
	const Vector3 oldPosition = globalPosition_;
	bool isInGlobalCoords = false;
	bool isInLimbo = false;

	// figure out where we stand now
	if (pVehicle_ == NULL && !PassengerExtra::isInLimbo( *this ))
	{
		globalPosition_ = localPosition_;
		globalDirection_ = localDirection_;

		isInGlobalCoords = true;
	}
	else if (pVehicle_ != NULL)
	{
		MF_ASSERT( Passengers::instance.exists( *pVehicle_ ) );
		const Matrix & m = Passengers::instance( *pVehicle_ ).transform();
		globalPosition_ = m.applyPoint( localPosition_ );
		// TODO: What do we do here? For now, just doing yaw.
		globalDirection_.yaw =
			Angle( localDirection_.yaw + pVehicle_->direction().yaw );
	}
	else
	{
		INFO_MSG( "Entity::updateGlobalPosition(%u): "
			"Not updating position since entity is in limbo\n", id_ );
		isInLimbo = true;
	}

	if (!isInLimbo)
	{
		// and shuffle to reseat ourselves in the range list
		if (this->isReal() && shouldUpdateGhosts)
		{
			this->updateInternalsForNewPositionOfReal( oldPosition,
				isVehicleMovement );
		}
		else
		{
			this->updateInternalsForNewPosition( oldPosition,
				isVehicleMovement );
		}
	}

	if (g_entityMovementCallback)
	{
		(*g_entityMovementCallback)( oldPosition, this );
	}

	return isInGlobalCoords;
}


/**
 *	This method is call whenever the vehicle we are riding on moves.
 */
void Entity::onVehicleMove()
{
	MF_ASSERT( pVehicle_ );

	// Update the global position because the vehicle has moved.
	this->updateGlobalPosition( /* shouldUpdateGhosts */ false,
		/* isVehicleMove */ true );
}


/**
 *	This method sets the position and direction of this entity and updates this
 *	information on its ghosts. It should really only be called on a real entity.
 *	@note The position and direction that this method takes is in local
 *	coordinates.
 */
void Entity::setPositionAndDirection( const Position3D & position,
		const Direction3D & direction )
{
	AUTO_SCOPED_THIS_ENTITY_PROFILE;

	if (!this->isReal())
	{
		ERROR_MSG( "Entity::setPositionAndDirection(%u): I am a ghost\n",
			id_ );
	}
	MF_ASSERT( this->isReal() );

	if (!isValidPosition( position ))
	{
		ERROR_MSG( "Entity::setPositionAndDirection(%u): "
				"(%f,%f,%f) is not a valid position\n",
			id_, position.x, position.y, position.z );
		return;
	}

	localPosition_ = position;
	localDirection_ = direction;
	this->updateGlobalPosition();
}


/**
 *	This method sets the global position and direction of the entity and will
 *	drop the Y position down to the ground while doing so.
 */
void Entity::setAndDropGlobalPositionAndDirection(
		const Position3D & position, const Direction3D & direction )
{
	SCOPED_PROFILE( DROP_POSITION );
	AUTO_SCOPED_THIS_ENTITY_PROFILE;

	IF_NOT_MF_ASSERT_DEV( pVehicle_ == NULL )
	{
		WARNING_MSG( "Entity::setAndDropGlobalPositionAndDirection(%u): "
				"Unable to set position while on vehicle %u\n",
			this->id(), pVehicle_->id() );
		return;
	}

	if (CellAppConfig::shouldNavigationDropPosition())
	{
		if (this->pChunkSpace()) {
			Position3D droppedPosition =
					this->pChunkSpace()->findDropPoint(	position );
			this->setPositionAndDirection( droppedPosition, direction );
		}
		else {
			//ToDo: change when findDropPoint() is supported
			//on all physical space types
			WARNING_MSG(
					"Entity::setAndDropGlobalPositionAndDirection(): "
					"Physical space of this type does not support "
					"findDropPoint(), using entity position\n");
			this->setPositionAndDirection( position, direction );
		}
	}
	else
	{
		this->setPositionAndDirection( position, direction );
	}
}


/**
 * This method checks if messages from an address should be buffered.
 */
bool Entity::shouldBufferMessagesFrom( const Mercury::Address & addr ) const
{
	MF_ASSERT( addr != Mercury::Address::NONE );

	// Reals and zombie ghosts don't buffer
	if (pRealChannel_ == NULL)
	{
		return false;
	}

	// If set, buffer message not from our next real.
	else if (nextRealAddr_ != Mercury::Address::NONE)
	{
		return nextRealAddr_ != addr;
	}

	// Otherwise, buffer messages not from our current real.
	else
	{
		return this->realAddr() != addr;
	}
}


/**
 *	This method trims the event history associated with this entity. It is called at a
 *	frequency so that all entities in an AoI would have been visited at least once over the
 *	period.
 *
 *	This method also calculates whether this entity is no longer being witnessed.
 *
 *	@param cleanUpTime	All events with a time less than this should be deleted.
 */
void Entity::trimEventHistory( GameTime /*cleanUpTime*/ )
{
	AUTO_SCOPED_THIS_ENTITY_PROFILE;

	// trim the event history
	eventHistory_.trim();

	// For real entities that are currently considered witnessed, check how long
	// it has been since they have been witnessed.
	if (this->isReal() &&
			(periodsWithoutWitness_ != NOT_WITNESSED_THRESHOLD))
	{
		++periodsWithoutWitness_;

		// No-one witnessed this real entity for entire period from 1 to 2.
		// Inform the ghosts to check whether they are being witnessed.
		if (periodsWithoutWitness_ == 2)
		{
			for (RealEntity::Haunts::iterator iter = pReal_->hauntsBegin();
				 iter != pReal_->hauntsEnd();
				 ++iter)
			{
				// TODO: Better handling of prefixed empty messages
				iter->bundle().startMessage(
					CellAppInterface::checkGhostWitnessed );
				iter->bundle() << id_;
			}
		}

		// No-one witnessed this real entity for entire period from 2 to 3. This
		// includes witnessing any ghost entities. This entity should no longer
		// be considered as being witnessed.
		if (periodsWithoutWitness_ == NOT_WITNESSED_THRESHOLD)
		{
			Py_INCREF( Py_False );
			this->callback( "onWitnessed", Py_BuildValue( "(O)", Py_False ),
				"onWitnessed(False) callback: ", true );
		}
	}
}


/**
 *	This method directs a callback to the entity. If callbacks are not
 *	allowed at the present time, the callback is buffered and executed
 *	when they are permitted again. Callbacks onto destroyed entities result
 *	in an error msg (for now - may be downgraded)
 *
 *	A reference to 'args' is consumed, just like for Script's call method
 *	(the ref is always consumed even if the function fails and returns false)
 */
bool Entity::callback( const char * funcName, PyObject * args,
	const char * errorPrefix, bool okIfFnNull )
{
	SCOPED_PROFILE( SCRIPT_CALL_PROFILE );
	AUTO_SCOPED_THIS_ENTITY_PROFILE;

	if (this->isDestroyed())
	{
		ERROR_MSG( "Entity::callback(%u): %sto %s entity %u is destroyed\n",
				   id_, (errorPrefix != NULL) ? errorPrefix : "", funcName, id_ );
		Py_DECREF( args );
		return false;
	}

	bool bufferIt = !(Entity::callbacksPermitted());

	// see if we are running in the context of another entity
	bool nominated = false;
	if (CellAppConfig::treatAllOtherEntitiesAsGhosts() && this->isReal())
	{
		if (s_pTreatAsReal == NULL)
		{
			Entity::nominateRealEntity( *this );
			nominated = true;
		}
		else if (s_pTreatAsReal != this)
		{
			Entity::nominateRealEntity( *this );
			nominated = true;
			//bufferIt = true;	// buffer it even if callbacks not prevented
			// except we can't do that because there won't be any call to
			// callbacksPermitted to execute the buffer.
			// TODO: Uncomment the line above we have a way around the
			// problem explained in the note above.
		}
		//else already nominated so follow normal path
	}

	// call it if we can
	PyObject * funcCallable = PyObject_GetAttrString(
		this, const_cast<char*>( funcName ) );

	if (funcCallable == NULL)
	{
		if (!okIfFnNull)
		{
			WARNING_MSG( "Entity::callback(%u): [%s %u] has no method %s\n",
				id_, this->pType()->name(), id_, funcName );
		}
		PyErr_Clear();
		if (nominated) Entity::nominateRealEntityPop();
		Py_DECREF( args );
		return okIfFnNull;
	}

	if (!bufferIt)
	{
		bool ret = Script::call(
			funcCallable, args, errorPrefix, false /* okIfFnNull */ );

		if (nominated) Entity::nominateRealEntityPop();
		return ret;
	}
	if (nominated) Entity::nominateRealEntityPop();

	// otherwise buffer it

	ScriptObject callable(
		funcCallable,
		ScriptObject::FROM_NEW_REFERENCE );

	ScriptArgs arguments(
		args,
		ScriptArgs::FROM_NEW_REFERENCE );

	s_callbackBuffer_.storeCallback(
		this, callable, arguments, errorPrefix );

	// assume it's not going to fail later
	return true;
}


/**
 *	This method populates all entity including delegate properties
 *	to a new dict object taking in to account the specified data domian. 
 */
ScriptDict Entity::createDictWithProperties( int dataDomains ) const
{
	ScriptDict result = createDictWithAllProperties( 
				pEntityType_->description(),
				ScriptObject( const_cast<Entity *>(this),
						ScriptObject::FROM_BORROWED_REFERENCE ),
				pEntityDelegate_.get(),
				dataDomains );
	if (!result)
	{
		ERROR_MSG( "populateDictWithLocalProperties: "
				"Could not add local properties, entity description %s\n",
				pEntityType_->description().name().c_str() );
	}
	return result;
}


/**
 *	This method populates all delegate properties
 *	to a new dict object taking in to account the specified data domian. 
 */
ScriptDict Entity::createDictWithComponentProperties( int dataDomains ) const
{
	ScriptDict result = createDictWithAllProperties( 
				pEntityType_->description(),
				ScriptObject(),
				pEntityDelegate_.get(),
				dataDomains );
	if (!result)
	{
		ERROR_MSG( "createDictWithComponentProperties: "
				"Could not add component properties, entity description %s\n",
				pEntityType_->description().name().c_str() );
	}
	return result;
}


/**
 *	This method populates all entity excluding delegate properties
 *	to a new dict object taking in to account the specified data domian. 
 */
ScriptDict Entity::createDictWithLocalProperties( int dataDomains ) const
{
	ScriptDict result = createDictWithAllProperties( 
				pEntityType_->description(),
				ScriptObject( const_cast<Entity *>(this),
						ScriptObject::FROM_BORROWED_REFERENCE ),
				NULL,
				dataDomains );
	if (!result)
	{
		ERROR_MSG( "populateDictWithLocalProperties: "
				"Could not add local properties, entity description %s\n",
				pEntityType_->description().name().c_str() );
	}
	return result;
}


/**
 *	Static method to change whether or not callbacks on entities are
 *	permitted
 */
void Entity::callbacksPermitted( bool permitted )
{
	if (permitted)
	{
		s_callbackBuffer_.leaveBufferedSection();
	}
	else
	{
		s_callbackBuffer_.enterBufferedSection();
	}
}


/// static initialisers
EntityCallbackBuffer Entity::s_callbackBuffer_;


static VectorNoDestructor<EntityPtr> s_nominatedEntities;

/**
 *	This static method nominates the given entity as the one that will
 *	appear real to script. All other entities will appear as ghosts,
 *	whether they actually be ghosts or not.
 */
void Entity::nominateRealEntity( Entity & e )
{
	if (!CellAppConfig::treatAllOtherEntitiesAsGhosts())
	{
		return;
	}

	s_nominatedEntities.push_back( &e );
	s_pTreatAsReal = &e;
}

/**
 *	This static method pops the most recently nominated entity off the
 *	stack of nominated entities.
 */
void Entity::nominateRealEntityPop()
{
	if (!CellAppConfig::treatAllOtherEntitiesAsGhosts())
	{
		return;
	}

	if (s_nominatedEntities.empty())
	{
		CRITICAL_MSG( "Entity::nominateRealEntityPop: "
			"Attempt to pop from empty stack\n" );
		return;
	}

	s_nominatedEntities.back() = NULL;	// it's a VectorNoDest
	s_nominatedEntities.pop_back();

	s_pTreatAsReal = s_nominatedEntities.empty() ?
		NULL : s_nominatedEntities.back().get();
}


/**
 *	The max network jitter to accept for the purposes of physics validation.
 *	A Swinburne uni paper (that I think I saw at ATNAC) found jitter and
 *	latency to be highly correlated, with jitter generally <= 20% of latency,
 *	even for very high latencies of say 600ms. The BaseApp and CellApp ticks
 *	probably introduce more jitter than the network. Thus the default is 200ms.
 */
// float Entity::s_maxPhysicsNetworkJitter_ = 0.2f;	// 200ms

CustomPhysicsValidator g_customPhysicsValidator = NULL;

BW_END_NAMESPACE

// entity.cpp
