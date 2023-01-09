#include "voip.hpp"
#include <string>
#include <map>

/**
 * TODO: to be documented.
 */
class SimpleVOIPDataDict : public VOIPDataDict
{
public:
	virtual const char * get( const char * key ) const;
	virtual bool get( unsigned int index, const char ** key, const char ** value ) const;
	virtual size_t size() const;

	std::map<std::string, std::string> dict_;
};


const char * SimpleVOIPDataDict::get( const char * key ) const
{
	std::map<std::string, std::string>::const_iterator result;
	result = dict_.find( key );

	if( result != dict_.end() )
		return result->second.c_str();
	else
		return NULL;
}


bool SimpleVOIPDataDict::get( unsigned int index, const char ** key, const char ** value ) const
{
	if( index >= dict_.size() )
		return false;

	std::map<std::string, std::string>::const_iterator result = dict_.begin();

	std::advance( result, index );

	if( result != dict_.end() )
	{
		*key = result->first.c_str();
		*value = result->second.c_str();
		return true;
	}

	return false;
}


size_t SimpleVOIPDataDict::size() const
{
	return dict_.size();
}






VOIPClient::VOIPClient() :
	response_( NULL ),
	initialised_( false ),
	data_( NULL )
{
}


VOIPClient::~VOIPClient()
{
}


VOIPClient * VOIPClient::createVOIPClient()
{
	return new VOIPClient;
}


void VOIPClient::destroyVOIPClient( VOIPClient * client )
{
	delete client;
}


int VOIPClient::initialise( const VOIPDataDict * data )
{
	return 0;
}


int VOIPClient::finalise()
{
	return 0;
}


void VOIPClient::tick( float x, float y, float z, float yaw, float pitch, float roll, int spaceID )
{
}


void VOIPClient::command( const VOIPDataDict * data )
{
}


void VOIPClient::getStatistics()
{
}


void VOIPClient::setHandler( VOIPResponse* response )
{
	response_ = response;
}


void VOIPClient::login( const char * username, const char * password, const VOIPDataDict * data )
{
}


void VOIPClient::logout( VOIPDataDict* data )
{
}


void VOIPClient::createChannel( const char * channel, const char * password, const VOIPDataDict * data )
{
}


void VOIPClient::joinChannel( const char* channelID, const char* password, const VOIPDataDict * data )
{
}


void VOIPClient::leaveChannel( const char * channel, const VOIPDataDict * data )
{
}


void VOIPClient::inviteUser( const char * username, const char * channel, const VOIPDataDict * data )
{
}


void VOIPClient::kickUser( const char * username, const char * channel, const VOIPDataDict * data )
{
}


int VOIPClient::numberChannels() const
{
	return 0;
}


const char * VOIPClient::getChannel( unsigned int i ) const
{
	return "";
}


void VOIPClient::enableActiveChannel( const char * channelID, const VOIPDataDict * data )
{
}


void VOIPClient::disableActiveChannel( const char * channelID, const VOIPDataDict * data )
{
}


void VOIPClient::enablePositional( const VOIPDataDict * data )
{
}


void VOIPClient::disablePositional( const VOIPDataDict * data )
{
}


void VOIPClient::enableMicrophone( const VOIPDataDict * data )
{
}


void VOIPClient::disableMicrophone( const VOIPDataDict * data )
{
}


void VOIPClient::setMasterVolume( float attenuationDB, const VOIPDataDict * data )
{
}


void VOIPClient::setChannelVolume( const char * channelName, float attenuationDB, const VOIPDataDict * data )
{
}


void VOIPClient::setMicrophoneVolume( float attenuationDB, const VOIPDataDict * data )
{
}

// viop.cpp
