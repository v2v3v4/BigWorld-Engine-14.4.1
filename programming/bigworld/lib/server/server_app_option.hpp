#ifndef SERVER_APP_OPTION_HPP
#define SERVER_APP_OPTION_HPP

#include "bwconfig.hpp"

#include "cstdmf/debug.hpp"
#include "cstdmf/debug_message_categories.hpp"
#include "cstdmf/intrusive_object.hpp"
#include "cstdmf/watcher.hpp"


BW_BEGIN_NAMESPACE

/**
 *	This class is the base class for objects used to initialise a
 *	ServerAppOption. These are created instead of adding the functionality to
 *	ServerAppOption so that all options can sit closer in memory.
 */
class ServerAppOptionIniter :
	public IntrusiveObject< ServerAppOptionIniter >
{
public:
	ServerAppOptionIniter( const char * configPath,
			const char * watcherPath,
			Watcher::Mode watcherMode );

	virtual void init() = 0;
	virtual void print() = 0;

	static void initAll();
	static void printAll();
	static void deleteAll();

protected:
	const char * configPath_;
	const char * watcherPath_;
	Watcher::Mode watcherMode_;

	static Container * s_pContainer_;
};


/**
 *	This class is a ServerAppOptionIniter that is type specific.
 */
template <class TYPE>
class ServerAppOptionIniterT : public ServerAppOptionIniter
{
public:
	ServerAppOptionIniterT( TYPE & value,
			const char * configPath,
			const char * watcherPath,
			Watcher::Mode watcherMode ) :
		ServerAppOptionIniter( configPath, watcherPath, watcherMode ),
		value_( value ),
		defaultValue_( value )
	{
	}

	virtual void init()
	{
		if (configPath_[0])
		{
			if (!BWConfig::update( configPath_, value_ ))
			{
				CONFIG_WARNING_MSG( "ServerAppConfig::init: "
						"%s not read from bw.xml chain. Using default (%s)\n",
					configPath_, watcherValueToString( value_ ).c_str() );
			}
		}

		if (watcherPath_[0])
		{
			MF_WATCH( watcherPath_, value_, watcherMode_ );
		}
	}

	virtual void print();

private:
	TYPE & value_;
	TYPE defaultValue_;
};


template <class TYPE>
void ServerAppOptionIniterT<TYPE>::print()
{
	const char * name = configPath_;

	if ((name  == NULL) || (name[0] == '\0'))
	{
		return;
	}

	if (value_ == defaultValue_)
	{
		CONFIG_INFO_MSG( "  %-34s = %s\n",
			name, watcherValueToString( value_ ).c_str() );
	}
	else
	{
		CONFIG_INFO_MSG( "  %-34s = %s (default: %s)\n",
			name,
			watcherValueToString( value_ ).c_str(),
			watcherValueToString( defaultValue_ ).c_str() );
	}
}

template <>
void ServerAppOptionIniterT<float>::print();

template <>
void ServerAppOptionIniterT<double>::print();

/**
 *	This class is a ServerAppOptionIniter that is type specific.
 */
template <class TYPE>
class ServerAppOptionIniterGetSetT : public ServerAppOptionIniter
{
public:
	ServerAppOptionIniterGetSetT( TYPE (*getterMethod)(),
			void (*setterMethod)( TYPE value ),
			const char * configPath,
			const char * watcherPath ) :
		ServerAppOptionIniter( configPath, watcherPath,
			Watcher::WT_READ_WRITE ),
		getterMethod_( getterMethod ),
		setterMethod_( setterMethod ),
		defaultValue_( getterMethod() )
	{
	}

	virtual void init()
	{
		if (configPath_[0])
		{
			TYPE value =  defaultValue_;
			if (!BWConfig::update( configPath_, value ))
			{
				CONFIG_WARNING_MSG( "ServerAppConfig::init: "
						"%s not read from bw.xml chain. Using default (%s)\n",
					configPath_, watcherValueToString( value ).c_str() );
			}

			setterMethod_( value );
		}

		if (watcherPath_[0])
		{
			MF_WATCH( watcherPath_, getterMethod_, setterMethod_ );
		}
	}

	virtual void print();
private:
	TYPE (*getterMethod_)();
	void (*setterMethod_)( TYPE value );
	TYPE defaultValue_;
};

template <class TYPE>
void ServerAppOptionIniterGetSetT<TYPE>::print()
{
	const char * name = configPath_;
	TYPE value = getterMethod_();

	if ((name  == NULL) || (name[0] == '\0'))
	{
		return;
	}

	if (value == defaultValue_)
	{
		CONFIG_INFO_MSG( "  %-34s = %s\n",
			name, watcherValueToString( value ).c_str() );
	}
	else
	{
		CONFIG_INFO_MSG( "  %-34s = %s (default: %s)\n",
			name,
			watcherValueToString( value ).c_str(),
			watcherValueToString( defaultValue_ ).c_str() );
	}
}

template <>
void ServerAppOptionIniterGetSetT<float>::print();

template <>
void ServerAppOptionIniterGetSetT<double>::print();



/**
 *	This class is used to add a configuration option to an application. It
 *	handles reading the value from bw.xml, adding a watcher entry and printing
 *	out the value of the setting on startup.
 */
template <class TYPE>
class ServerAppOption
{
public:
	ServerAppOption( TYPE value,
			const char * configPath,
			const char * watcherPath,
			Watcher::Mode watcherMode = Watcher::WT_READ_WRITE ) :
		value_( value )
	{
		new ServerAppOptionIniterT< TYPE >( value_,
				configPath, watcherPath, watcherMode );
	}
	
	TYPE operator()() const	{ return value_; }
	void set( TYPE value ) { value_ = value; }
	TYPE & getRef()	{ return value_; }

private:
	TYPE value_;
};


/**
 *	This class is used to add a configuration option to an application. It
 *	handles reading the value from bw.xml, adding a watcher entry and printing
 *	out the value of the setting on startup.
 */
template <class TYPE>
class ServerAppOptionGetSet
{
public:
	ServerAppOptionGetSet( TYPE (*getterMethod)(),
			void (*setterMethod)( TYPE value ),
			const char * configPath,
			const char * watcherPath ) :
		getterMethod_( getterMethod ),
		setterMethod_( setterMethod )
	{
		new ServerAppOptionIniterGetSetT< TYPE >( getterMethod,
			setterMethod, configPath, watcherPath );
	}

	TYPE operator()() const
	{ 
		return getterMethod_();
	}


	void set( TYPE value ) 
	{
		setterMethod_( value );
	}

private:
	TYPE (*getterMethod_)();
	void (*setterMethod_)( TYPE value );
};

BW_END_NAMESPACE

#endif // SERVER_APP_OPTION_HPP
