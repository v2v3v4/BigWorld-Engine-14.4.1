#ifndef LOG_ENTRY_ADDRESS_READER_HPP
#define LOG_ENTRY_ADDRESS_READER_HPP

#include "log_entry_address.hpp"

#include "Python.h"


BW_BEGIN_NAMESPACE

class BinaryIStream;

#pragma pack( push, 1 )
class LogEntryAddressReader : public LogEntryAddress
{
public:
	bool isValid() const;

	bool fromPyTuple( PyObject *tuple );
};
#pragma pack( pop )

BW_END_NAMESPACE

#endif // LOG_ENTRY_ADDRESS_READER_HPP
