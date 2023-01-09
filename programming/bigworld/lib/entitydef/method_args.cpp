#include "pch.hpp"

#include "method_args.hpp"

#include "data_type.hpp"
#include "script_data_sink.hpp"
#include "script_data_source.hpp"

#include "cstdmf/binary_stream.hpp"
#include "cstdmf/md5.hpp"

#ifdef SCRIPT_PYTHON
#include "pyscript/pyobject_pointer.hpp"
#include "pyscript/script.hpp"
#endif // SCRIPT_PYTHON

#include "resmgr/datasection.hpp"


BW_BEGIN_NAMESPACE


MethodArgs::MethodArgs() :
	args_(),
	streamSize_(0)
{
}


/**
 *	This method parses a list of arguments from a data section.
 */
bool MethodArgs::parse( DataSectionPtr pSection, bool isOldStyle )
{
	DataSection::iterator iter = pSection->begin();

	args_.clear();
	streamSize_ = 0;

	BW::set< BW::string > argNames;

	while (iter != pSection->end())
	{
		DataSectionPtr pDS = *iter;

		if (!isOldStyle || (pDS->sectionName() == "Arg"))
		{
			DataTypePtr pDataType = DataType::buildDataType( pDS );

			if (!pDataType)
			{
				ERROR_MSG( "MethodArgs::parse: "
						"Return type #%" PRIzu " is invalid\n",
					args_.size() );
				args_.clear();
				return false;
			}

			BW::string argName;

			if (!isOldStyle)
			{
				argName = pDS->sectionName();

				if (argNames.count( argName ) > 0)
				{
					ERROR_MSG( "MethodArgs::parse: "
						"Duplicate argument name '%s'\n", argName.c_str() );

					// TODO: Always return false.
					if (argName != "Arg")
					{
						return false;
					}
				}

				argNames.insert( argName );
			}

			args_.push_back( std::make_pair( argName, pDataType ) );

			if (streamSize_ != -1)
			{
				const int dataTypeStreamSize = pDataType->streamSize();

				if (dataTypeStreamSize == -1)
				{
					streamSize_ = -1;
				}
				else
				{
					streamSize_ += dataTypeStreamSize;
				}
			}
		}

		++iter;
	}

	return true;
}


/**
 *	This method checks that a tuple of arguments are valid.
 *
 *	@param args A tuple of arguments.
 *	@param name The name of the associated method. This is used for error
 *		messages.
 *	@param firstOrdinaryArg The offset of where the first argument is in the
 *		tuple of arguments.
 */
bool MethodArgs::checkValid( ScriptTuple args,
		const char * name, int firstOrdinaryArg ) const
{
	int expectedArgs = static_cast<int>(args_.size()) + firstOrdinaryArg;

	// make sure we have the right number (no default arguments yet)
	// if (PySequence_Size( args ) - firstOrdinaryArg != (int)args_.size())
	if (args.size() != expectedArgs)
	{
#if defined( SCRIPT_PYTHON )
		PyErr_Format( PyExc_TypeError,
			"Method %s requires exactly %d argument%s; %d given",
			name,
			expectedArgs,
			(expectedArgs == 1) ? "" : "s",
			(int)args.size() );
#endif // SCRIPT_PYTHON

		return false;
	}

#if defined( SCRIPT_PYTHON )
	// check the exposed parameter if it is explicit
	if (firstOrdinaryArg == 1)
	{
		ScriptObject pExposed = args.getItem( 0 );

		if (!PyInt_Check( pExposed.get() ) && (pExposed.get() != Py_None))
		{
			PyObject * peid = PyObject_GetAttrString( pExposed.get(), "id" );

			if (peid == NULL || !PyInt_Check( peid ))
			{
				Py_XDECREF( peid );
				PyErr_Format( PyExc_TypeError,
					"Method %s requires None, an id, or an object with an "
						"id as its first argument", name );
				return false;
			}

			Py_DECREF( peid );
		}
	}
#endif // SCRIPT_PYTHON

	// check each argument
	int numArgs = static_cast<int>(args_.size());
	for (int i = 0; i < numArgs; i++)
	{
		ScriptObject pArg = args.getItem( i + firstOrdinaryArg );
		DataType * pType = args_[i].second.get();

		if (!pType->isSameType( pArg ))
		{
#if defined( SCRIPT_PYTHON )
			ScriptDataSink sink;
			bool result = pType->getDefaultValue( sink );
			ScriptObject pExample = sink.finalise();

			if (result && pExample->ob_type == pArg->ob_type)
			{
				PyObjectPtr pStr( PyObject_Str( pArg.get() ),
						PyObjectPtr::STEAL_REFERENCE );
				PyErr_Format( PyExc_TypeError,
					"Method %s, argument %d: Expected %s, got value %s",
					name,
					i+1,
					pType->typeName().c_str(),
					PyString_AsString( pStr.get() ) );
			}
			else
			{
				PyErr_Format( PyExc_TypeError,
					"Method %s, argument %d: Expected %s, %s found",
					name,
					i+1,
					pType->typeName().c_str(),
					pArg->ob_type->tp_name );
			}
#endif // SCRIPT_PYTHON

			return false;
		}
	}

	return true;
}


/**
 *	This method adds the arguments in a DataSource to a stream.
 *
 *	@param source			A DataSource containing the arguments to add. It
 *							must have been started as a sequence already.
 *	@param stream			The stream to add the arguments to
 *
 *	@return True if successful, false if a streaming error occurred. In this
 *			case, default values for the arguments are used on the stream,
 *			and an error is logged for the first argument that fails.
 */
bool MethodArgs::addToStream( DataSource & source,
	BinaryOStream & stream ) const
{
	size_t numArgs = args_.size();

	if (numArgs == 0)
	{
		return true;
	}

	bool isOkay = true;

	size_t numArgsInSource;

	isOkay &= source.beginSequence( numArgsInSource );

	if (isOkay && numArgsInSource == 0)
	{
		// If we don't know how many args are available,
		// assume it's correct.
		numArgsInSource = numArgs;
	}

	for (size_t i = 0; i < numArgs; i++)
	{
		if (i >= numArgsInSource)
		{
			// TODO: Better than this
			ScriptDataSink sink;
			args_[ i ].second->getDefaultValue( sink );
			ScriptDataSource source( sink.finalise() );
			args_[ i ].second->addToStream( source, stream,
				/* isPersistentOnly */ false );

			if (!isOkay)
			{
				ERROR_MSG( "MethodArgs::addToStream: "
						"Insufficient argument values.\n" );
			}
			isOkay = false;
			continue;
		}
		
		if (!source.enterItem( i ) && isOkay)
		{
			ERROR_MSG( "MethodArgs::addToStream: enterItem failed for arg %" PRIzu
					".\n",
				i );
			isOkay = false;
		}

		if (!args_[ i ].second->addToStream( source, stream,
			/* isPersistentOnly */ false ) && isOkay )
		{
			ERROR_MSG( "MethodArgs::addToStream: addToStream failed for arg %" PRIzu
					".\n",
				i );
			isOkay = false;
		}

		if (!source.leaveItem() && isOkay)
		{
			ERROR_MSG( "MethodArgs::addToStream: leaveItem failed for arg %" PRIzu
					".\n",
				i );
			isOkay = false;
		}
	}

	return isOkay;
}


/**
 *	This method writes a tuple of arguments from a stream into a DataSink.
 *
 *	@param data The stream to read from.
 *	@param sink The DataSink to write to.
 *	@param name The name of the method. Used for error messages.
 *	@param pImplicitSource If not NULL, this will be prepended as an extra
 *		argument.
 */
bool MethodArgs::createFromStream( BinaryIStream & data,
		DataSink & sink, const BW::string & name,
		EntityID * pImplicitSource ) const
{
	int numArgs = static_cast< int >( args_.size() );
	bool hasImplicitSourceID = (pImplicitSource != NULL);

	int tupleSize = numArgs + (hasImplicitSourceID ? 1 : 0);

	sink.beginTuple( NULL, tupleSize );

	int argAt = 0;

	if (hasImplicitSourceID)
	{
		if (!sink.enterItem( argAt++ ) ||
			!sink.write( *pImplicitSource ) ||
			!sink.leaveItem())
		{
			ERROR_MSG( "MethodArgs::createFromStream: "
					"Failed to write implicit source Entity ID for method %s "
					"from the stream.\n",
				name.c_str() );
			return false;
		}
	}

	for (int i = 0; i < numArgs; i++)
	{
		if (!sink.enterItem( argAt++ ))
		{
			ERROR_MSG( "MethodArgs::createFromStream: "
					"Failed to enter argument %d for method %s.\n",
				i, name.c_str() );
			return false;
		}
		
		DataTypePtr pType = args_[i].second;

		if (!pType->createFromStream( data, sink,
				/* isPersistentOnly */ false ) || data.error())
		{
			ERROR_MSG( "MethodArgs::createFromStream: "
					"Failed to get arg %d (of type %s) for method %s from "
					"the stream.\n",
				i, pType->typeName().c_str(), name.c_str() );
			return false;
		}

		if (!sink.leaveItem())
		{
			ERROR_MSG( "MethodArgs::createFromStream: "
					"Failed to leave argument %d for method %s.\n",
				i, name.c_str() );
			return false;
		}
		
	}

	// Check whether the entire stream is consumed
	// NOTE: This means multiple method calls/stream are not supported
	if (data.remainingLength() > 0)
	{
		ERROR_MSG( "MethodArgs::createFromStream: CHEAT: "
					 "Data still remains on stream after all args have been "
					 "streamed off! (%d bytes remaining)\n",
					 data.remainingLength() );
		return false;
	}

	return true;
}


/**
 *	This method adds this object to an MD5.
 */
void MethodArgs::addToMD5( MD5 & md5 ) const
{
	Args::const_iterator iter = args_.begin();

	while (iter != args_.end())
	{
		iter->second->addToMD5( md5 );

		iter++;
	}
}


/**
 *	This method converts non-keyword and keyword arguments to a tuple of
 *	non-keyword arguments.
 */
ScriptObject MethodArgs::convertKeywordArgs( ScriptTuple args,
	ScriptDict kwargs ) const
{
	size_t fixedLen = args.size();
	size_t kwLen = kwargs.size();

	size_t size = fixedLen + kwLen;

	if (size != args_.size())
	{
#if defined( SCRIPT_PYTHON )
		PyErr_Format( PyExc_TypeError,
				"Method takes exactly %zd argument%s (%zd given)",
				args_.size(),
				(args_.size() != 1) ? "s" : "",
				size );
#endif // SCRIPT_PYTHON
		return ScriptObject();
	}

	ScriptTuple pResult = ScriptTuple::create( size );

	size_t i = 0;

	while (i < fixedLen)
	{
		ScriptObject pArg = args.getItem( i );
		pResult.setItem( i, pArg );

		++i;
	}

	while (i < size)
	{
		ScriptObject pArg = kwargs.getItem( args_[i].first.c_str(),
			ScriptErrorClear() );

		if (!pArg)
		{
#if defined( SCRIPT_PYTHON )
			PyErr_Format( PyExc_TypeError,
					"Failed to find argument '%s'",
					args_[i].first.c_str() );
#endif // SCRIPT_PYTHON
			return ScriptObject();
		}

		pResult.setItem( i, pArg );

		++i;
	}

	return pResult;
}


/**
 *	This method creates a dictionary from a tuple of arguments.
 */
ScriptDict MethodArgs::createDictFromTuple( ScriptTuple args ) const
{
	if (!ScriptTuple::check( args ) ||
			(args.size() != int( args_.size() )))
	{
#if defined( SCRIPT_PYTHON )
		PyErr_Format( PyExc_TypeError,
				"Argument is not a tuple length %d\n",
				int( args_.size() ) );
#endif // SCRIPT_PYTHON
		return ScriptDict();
	}

	int numArgs = static_cast<int>(args_.size());

	ScriptDict pDict = ScriptDict::create( numArgs );

	for (int i = 0; i < numArgs; ++i)
	{
		pDict.setItem( args_[i].first.c_str(), args.getItem( i ),
			ScriptErrorPrint( "MethodArgs::createDictFromTuple: " ) );
	}

	return pDict;
}


/**
 *	This method returns the safety level of arguments:
 *	PYTHON = unsafe
 *	MAILBOX = unusable
 */
int MethodArgs::clientSafety() const
{
	Args::const_iterator iter = args_.begin();

	int clientSafety = DataType::CLIENT_SAFE;

	while (iter != args_.end())
	{
		clientSafety |= iter->second->clientSafety();

		++iter;
	}

	return clientSafety;
}


/**
 *	This method returns a sequence of pairs that describe the argument types.
 */
ScriptObject MethodArgs::typesAsScript() const
{
	ScriptTuple pTuple = ScriptTuple::create( args_.size() );

	for (uint i = 0; i < args_.size(); ++i)
	{
		ScriptTuple pair = ScriptTuple::create( 2 );
		pair.setItem( 0, ScriptObject::createFrom( args_[i].first ) );
		pair.setItem( 1, ScriptObject::createFrom( args_[i].second->typeName() ) );
		pTuple.setItem( i, pair );
	}

	return pTuple;
}


/**
*	The comparison operator, relies on the fact that arguments of the same time 
*	refer to the same instance of DataType. See DataType::buildDataType and
*	DataType::findOrAddType.
*/
bool operator== ( const MethodArgs & left, const MethodArgs & right )
{
	return left.streamSize_ == right.streamSize_ && left.args_ == right.args_;
}


bool operator!= ( const MethodArgs & left, const MethodArgs & right )
{
	return !(left == right);
}


BW_END_NAMESPACE

// method_args.cpp
