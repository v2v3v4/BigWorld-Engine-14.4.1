#ifndef PY_DEBUG_MESSAGE_FILE_LOGGER_HPP
#define PY_DEBUG_MESSAGE_FILE_LOGGER_HPP 

#include "pyobject_plus.hpp"
#include "cstdmf/debug_message_file_logger.hpp"
#include "cstdmf/smartpointer.hpp"
#include "script/py_script_object.hpp"


BW_BEGIN_NAMESPACE

class DataSection;
typedef SmartPointer<DataSection> DataSectionPtr;

class PyDebugMessageFileLogger;
typedef ScriptObjectPtr<PyDebugMessageFileLogger> PyDebugMessageFileLoggerPtr;


/**
*	This class is a PyObjectPlus wrapper of DebugMessageFileLogger
*	It provides interfaces to config DebugMessageFileLogger
*	from python and from reading from xml file.
*/
class PyDebugMessageFileLogger: 
	public DebugMessageFileLogger,
	public PyObjectPlus
{
	Py_Header( PyDebugMessageFileLogger, PyObjectPlus )
public:
	PyDebugMessageFileLogger();

	static PyObject * _pyNew( PyTypeObject * pType,
								PyObject * args, PyObject * kwargs );

	PY_FACTORY_DECLARE();

	PY_KEYWORD_METHOD_DECLARE( py_config )

	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( bool, enable, enable )
	PY_RO_ATTRIBUTE_DECLARE( this->fileName(), fileName );
	PY_RO_ATTRIBUTE_DECLARE( this->openMode(), openMode );
	PY_RO_ATTRIBUTE_DECLARE( this->severities(), severities );
	PY_RO_ATTRIBUTE_DECLARE( this->sources(), sources );
	PY_RO_ATTRIBUTE_DECLARE( this->category(), category );

	void configFromDataSection( DataSectionPtr fileNameSection );

};


/**
*	This class is a simple wrapper of a list of file loggers
*	created by config xml files, calling BigWorld.configCreatedFileLoggers
*	can access those file loggers.
*/
class ConfigCreatedFileLoggers
{
public:
	const static size_t MAX_FILE_LOGGERS = 5;

	ConfigCreatedFileLoggers();
	bool addFileLogger( PyDebugMessageFileLoggerPtr fileLogger );
	PyDebugMessageFileLoggerPtr operator []( size_t i );
	size_t len()	{ return size_; }
private:
	PyDebugMessageFileLoggerPtr fileLoggers_[ MAX_FILE_LOGGERS ];
	size_t size_;
};

extern ConfigCreatedFileLoggers  configCreatedFileLoggers_;

BW_END_NAMESPACE

#endif // PY_DEBUG_MESSAGE_FILE_LOGGER_HPP
