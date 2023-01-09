// BigWorld Voice over IP (VoIP) DLL interface 1.0

#ifndef VOIP_HPP
#define VOIP_HPP

#include <cstddef>

#ifdef VOIP_EXPORTS
	#define VOIP_DLL_EXPORT __declspec(dllexport)
#else
	#define VOIP_DLL_EXPORT __declspec(dllimport)
#endif


/**
 *	This interface provides a simple interface to pairs of string values 
 *	for the passing additional information to the VoIP implementation. 
 */
class VOIPDataDict
{
public:
	/**
	 *	Returns the string value associated with the given key. 
	 *	If the key is not found NULL is returned.
	 *
	 *	@param	key			The key/name of the value to retrieve.
	 *
	 *	@return				A pointer to the value corresponding to the given key. NULL if not found.
	 */
	virtual const char * get( const char * key ) const = 0;

	/**
	 *	This function retrieves a key value pair based on a given 
	 *	index less than size(). The pairs need not be in any order. 
	 *	The intended purpose being to handle situations where the 
	 *	dictionary needs to be cloned into another format such as 
	 *	a python object.
	 *
	 *	@param	index		An index less than size() that uniquely 
	 *						identifies a key value pair.
	 *	@param	key			Output. On success this pointer will 
	 *						point to a string containing the key. 
	 *						This pointer will be left untouched on error.
	 *	@param	value		Output. On success this pointer will 
	 *						point to a string containing the value. 
	 *						This pointer will be left untouched on error.
	 *
	 *	@return				Returns true on success, false on failure
	 */
	virtual bool get( unsigned int index, const char ** key, const char ** value ) const = 0;

	/**
	 *	Returns the number key value pairs held by the object.
	 *
	 *	@return				The number of pairs.
	 */
	virtual size_t size() const = 0;
};


/**
 *	This class is an interface used by the VOIPClient to send callbacks
 *	to the BigWorld client and ultimately a python script.
 */
class VOIPResponse
{
public:
	/**
	 *	This function represents a callback available to the VoIP 
	 *	implementation to communicate with the client script code.
	 *
	 *	@param	message		An integer with a VoIP implementation defined meaning.
	 *	@param	data		A set of string value pairs that will be passed into 
	 *						the script as a python dictionary.
	 */
	virtual void response( int message, const VOIPDataDict * data ) = 0;
};


/**
 *	This class forms an interface to VoIP services implemented in the DLL voip.dll.
 *	The BigWorld client implements a python wrapper to this interface that can be 
 *	accessed in script though BigWorld.VOIP
 */
class VOIP_DLL_EXPORT VOIPClient
{
private:
	/**
	 *	Private constructor
	 *
	 *	VOIPClient objects should only be constructed by the createVOIPClient function
	 */
	VOIPClient();

	/**
	 *	Private destructor
	 *
	 *	VOIPClient objects should only be destroyed by the destroyVOIPClient function
	 */
	~VOIPClient();

public:
	/**
	 *	Creates a new instance of the VOIPClient
	 *	The returned VOIPClient should only be deleted using the destroyVOIPClient() method
	 *
	 *	@return				The new VOIPClient. NULL on error.
	 */
	static VOIPClient * createVOIPClient();

	/**
	 *	Deletes a VOIPClient previously created using createVOIPClient()
	 *
	 *	@param	client		The VOIPClient to be deleted.
	 */
	static void destroyVOIPClient( VOIPClient * client );

	/**
	 *	Returns whether the VOIPClient has already been initialised and not finalised.
	 *
	 *	@return				Returns true if the VOIPClient is in an initialised 
	 *						state ready login to the VoIP service.
	 */
	bool initialised() const;

	/**
	 *	Puts the VoIP client in an initialised state ready 
	 *	for connecting to the Voice service.
	 *
	 *	@param	data		Optional: Additional information in the form 
	 *						of string value pairs
	 */
	int initialise( const VOIPDataDict * data );

	/**
	 *	This function instructs the VoIP client to clean up all 
	 *	its connections and resources.
	 */
	int finalise();

	/**
	 *	A tick function called each frame to allow positional 
	 *	audio filtering and unthreaded implementations.
	 *	Note: The given location is in world space
	 *
 	 *	@param	x			The location of the player in on the X axis. +X is right
	 *	@param	y			The location of the player in on the Y axis. +Y is up
	 *	@param	z			The location of the player in on the Z axis. +Z is forward
	 *	@param	yaw			The current yaw of the player. 0.0f yaw is forward, +/- PI is backard
	 *	@param	pitch		The current pitch of the player. 0.0f pitch is level, +ve pitch is up (clockwise in Z-Y plane)
	 *	@param	roll		
 	 *	@param	spaceID		An integer uniquely identifying the space in which the player currently resides
	 */
	void tick( float x, float y, float z, float yaw, float pitch, float roll, int spaceID );

	/**
	 *	This is a generic function for handling implementation specific 
	 *	commands not exposed in the general VoIP interface.
	 *
	 *	@param	data		Optional: Additional information in the form of string value pairs
	 */
	void command( const VOIPDataDict * data );

	/**
	 *	Requests an implementation specific a statistics report that 
	 *	will be sent back to the application via the response callback mechanism.
	 */
	void getStatistics();

	/**
	 *	Sets the response callback handler.
	 *	The VOIPClient must not destroy this object.
	 *
	 *	@param	response		An object implementing the VOIPResponce 
	 *							interface. Passing NULL will clear the current handler.
	 */
	void setHandler( VOIPResponse * response );

	/**
	 *	This function logs the client into the VoIP service.
	 *
	 *	@param	username	The string that uniquely identifies the user's account
	 *	@param	password	The corresponding password to access the account
	 *	@param	data		Optional: Additional information in the form of string value pairs
	 */
	void login( const char * username, const char * password, const VOIPDataDict * data = NULL );

	/**
	 *	This function logs the client out of the VoIP service.
	 *
	 *	@param	data		Optional: Additional information in the form of string value pairs
	 */
	void logout( VOIPDataDict * data = NULL );

	/**
	 *	Creates a new channel with the given name. 
	 *	The creating user automatically joins the newly created channel. 
	 *	The channel will be destroyed when all users leave the channel.
	 *	Default password is "".
	 *
	 *	@param	channel			The string that will uniquely identify the new channel
	 *	@param	password		Optional: The password that will be needed to access the channel
	 *	@param	data			Optional: Additional information in the form of string value pairs
	 */
    void createChannel( const char * channel, const char * password = "", const VOIPDataDict * data = NULL );

	/**
	 *	Joins an existing channel. Default password is "".
	 *
	 *	@param	channel			The string identifying the channel
	 *	@param	password		Optional: The password that will be needed to access the channel
	 *	@param	data			Optional: Additional information in the form of string value pairs
	 */
	void joinChannel( const char * channel, const char * password = "", const VOIPDataDict * data = NULL );

	/**
	 *	Leaves a channel that the client is already connected to.
	 *
	 *	@param	channel			The string identifying the channel
	 *	@param	data			Optional: Additional information in the form of string value pairs
	 */
	void leaveChannel( const char * channel, const VOIPDataDict * data = NULL );

	/**
	 *	Sends an invite to another VoIP user to join a channel.
	 *
	 *	@param	username		The string identifying the user's VOIP account
	 *	@param	channel			The string identifying the channel
	 *	@param	data			Optional: Additional information in the form of string value pairs
	 */
	void inviteUser( const char * username, const char * channel, const VOIPDataDict * data = NULL );

	/**
	 *	Kicks a user from a channel
	 *
	 *	@param	username		The string identifying the user's VOIP account
	 *	@param	channel			The string identifying the channel
	 *	@param	data			Optional: Additional information in the form of string value pairs
	 */
	void kickUser( const char * username, const char * channel, const VOIPDataDict * data = NULL );

	/**
	 *	Retrieves the number channels the user is connected to.
	 *
	 *	@return				The number of channels
	 */
	int numberChannels() const;

	/**
	 *	Retrieves the name of the ith channel the user is connected to.
	 *
	 *	@param	index		The index used to identify the channel
	 *	@return				The name of the channel
	 */
	const char * getChannel( unsigned int index ) const;

	/**
	 *	Enables voice transmission on the specified channel.
	 *
	 *	@param	channelID		The string identifying the channel
	 *	@param	data			Optional: Additional information in the form of string value pairs
	 */
	void enableActiveChannel( const char * channelID, const VOIPDataDict * data = NULL );

	/**
	 *	Disables voice transmission on the specified channel.
	 *
	 *	@param	channelID		The string identifying the channel
	 *	@param	data			Optional: Additional information in the form of string value pairs
	 */
	void disableActiveChannel( const char * channelID, const VOIPDataDict * data = NULL );

	/**
	 *	Enable positional audio filtering
	 *
	 *	@param	data			Optional: Additional information in the form of string value pairs
	 */
	void enablePositional( const VOIPDataDict * data = NULL );

	/**
	 *	Disable positional audio filtering
	 *
	 *	@param	data			Optional: Additional information in the form of string value pairs
	 */
	void disablePositional( const VOIPDataDict * data = NULL );

	/**
	 *	Enables microphone input.
	 *
	 *	@param	data			Optional: Additional information in the form of string value pairs
	 */
	void enableMicrophone( const VOIPDataDict * data = NULL );

	/**
	 *	Disables microphone input.
	 *
	 *	@param	data			Optional: Additional information in the form of string value pairs
	 */
	void disableMicrophone( const VOIPDataDict * data = NULL );

	/**
	 *	Sets the overall volume at which voice communications will be played.
	 *
	 *	@param	attenuationDB	The new speaker volume attenuation in decibels(dB)
	 *	@param	data			Optional: Additional information in the form of string value pairs
	 */
	void setMasterVolume( float attenuationDB, const VOIPDataDict * data = NULL );

	/**
	 *	Sets the volume at which voice communications on the given channel play.
	 *	This attenuation is in addition to that applied by the master volume.
	 *
 	 *	@param	channelName		The string identifying the channel
	 *	@param	attenuationDB	The new speaker volume attenuation in decibels(dB)
	 *	@param	data			Optional: Additional information in the form of string value pairs
	 */
	void setChannelVolume( const char * channelName, float attenuationDB, const VOIPDataDict * data = NULL );

	/**
	 *	Sets the attenuation applied to the microphone input.
	 *
	 *	@param	attenuationDB	The new microphone attenuation in decibels(dB)
	 *	@param	data			Optional: Additional information in the form of string value pairs
	 */
	void setMicrophoneVolume( float attenuationDB, const VOIPDataDict * data = NULL );

private:
	VOIPResponse * response_;
	bool initialised_;
	void * data_;
};




inline bool VOIPClient::initialised() const
{
	return initialised_;
}

#endif // VOIP_HPP

