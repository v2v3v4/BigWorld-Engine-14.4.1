#ifndef APP_CONFIG_HPP
#define APP_CONFIG_HPP


#include "resmgr/datasection.hpp"
#include "resmgr/dataresource.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This class provides access to the datasection of application
 *	configuration settings.
 */
class AppConfig
{
public:
	AppConfig();
	~AppConfig();

	bool init( DataSectionPtr configSection );

	DataSectionPtr pRoot() const			{ return pRoot_; }

	static AppConfig & instance();

private:
	AppConfig( const AppConfig& );
	AppConfig& operator=( const AppConfig& );

	DataSectionPtr	pRoot_;
};


#ifdef CODE_INLINE
#include "app_config.ipp"
#endif

BW_END_NAMESPACE

#endif // APP_CONFIG_HPP
