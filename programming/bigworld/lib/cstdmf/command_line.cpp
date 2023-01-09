#include "pch.hpp"

#include "command_line.hpp"

BW_BEGIN_NAMESPACE

/**
 *
 */
CommandLine::CommandLine( const char* commandLine )
{
	size_t offset = 0;

	while (commandLine[ offset ])
	{
		while (isspace( commandLine[ offset ] ))
		{
			++offset;
		}

		BW::string param;

		while (commandLine[ offset ])
		{
			if (commandLine[ offset ] == '\"')
			{
				++offset;

				while (commandLine[ offset ] && commandLine[ offset ] != '\"')
				{
					param += commandLine[ offset ];
					++offset;
				}

				if (commandLine[ offset ])
				{
					++offset;
				}
			}
			else if (!isspace( commandLine[ offset ] ))
			{
				param += commandLine[ offset ];
				++offset;
			}
			else
			{
				break;
			}
		}

		params_.push_back( param );
	}
}

/**
 *
 */
bool CommandLine::hasParam( const char* param, const char* commandSwitch ) const
{
	bool found = false;
	size_t switchNum = strlen( commandSwitch );

	for (Params::const_iterator iter = params_.begin();
		!found && iter != params_.end(); ++iter)
	{
		for (size_t i = 0; i < switchNum; ++i)
		{
			if (iter->c_str()[0] == commandSwitch[ i ] &&
				strcmp( iter->c_str() + 1, param ) == 0)
			{
				found = true;

				break;
			}
		}
	}

	return found;
}

/**
 *
 */
bool	CommandLine::hasSwitch( const char* param, const char* commandSwitch )
{
	// to have a switch param must have at least two symbols ( i.e. -s )
	if (strlen( param ) < 2)
	{
		return false;
	}
	size_t len = strlen( commandSwitch );
	for (size_t i = 0; i < len; ++i)
	{
		if (param[0] == commandSwitch[i])
		{
			return true;
		}
	}
	return false;
}

/**
 *
 */
const char*	CommandLine::getFullParam( const char* paramToFind, int paramIndex ) const
{
	if (paramIndex > 0)
	{
		if ((int)params_.size() >= paramIndex + 1 )
		{
			const char* param = params_[paramIndex].c_str();
			if (strcmp( param, paramToFind ) == 0)
			{
				if (paramIndex + 1 < (int)params_.size())
				{
					return params_[paramIndex + 1].c_str();
				}
				return "";
			}
		}
	}
	else
	{
		for (Params::const_iterator it = params_.begin();
			it != params_.end(); ++it)
		{
			const char* param = (*it).c_str();
			if (strcmp( param, paramToFind ) == 0)
			{
				++it;
				if (it != params_.end())
				{
					return  (*it).c_str();
				}
				return "";
			}
		}
	}

	return NULL;
}

/**
 *
 */
bool	CommandLine::hasFullParam( const char* paramToFind, int paramIndex ) const
{
	const char* param = this->getFullParam( paramToFind, paramIndex );
	return param != NULL;
}

/**
 *
 */
const char*	CommandLine::getParamByIndex( int paramIndex ) const
{
	if (paramIndex > 0 && (int)params_.size() >= paramIndex +1 )
	{
		return params_[paramIndex].c_str();
	}
	return NULL;
}

/**
 *
 */
const char* CommandLine::getParam( const char* param, const char* commandSwitch ) const
{
	bool found = false;
	size_t switchNum = strlen( commandSwitch );
	const char* result = "";

	for (size_t i = 0; i < switchNum && !found; ++i)
	{
		for (Params::const_iterator iter = params_.begin();
			!found && iter != params_.end(); ++iter)
		{
			const char* str = (*iter).c_str();
			if (str[0] == commandSwitch[ i ] &&
				strcmp( str + 1, param ) == 0)
			{
				found = true;
				++iter;

				if (iter != params_.end())
				{
					result = iter->c_str();
				}

				break;
			}
		}
	}

	return result;
}

/**
 *
 */
size_t CommandLine::size() const
{
	return params_.size();
}

/**
 *
 */
const BW::string& CommandLine::param( size_t index ) const
{
	return params_.at( index );
}

BW_END_NAMESPACE

// command_line.cpp
