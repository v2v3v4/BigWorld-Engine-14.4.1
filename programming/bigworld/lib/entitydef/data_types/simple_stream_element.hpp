#ifndef SIMPLE_STREAM_ELEMENT_HPP
#define SIMPLE_STREAM_ELEMENT_HPP

#include "entitydef/data_source.hpp"
#include "entitydef/data_sink.hpp"
#include "entitydef/data_type.hpp"

#include "cstdmf/binary_stream.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This template class provides a simple DataType::StreamElement
 *	implementation for use by the various primitive types
 */
template< typename TYPE >
class SimpleStreamElement : public DataType::StreamElement
{
public:
	SimpleStreamElement( const DataType & type ) :
		DataType::StreamElement( type )
	{};

	bool fromSourceToStream( DataSource & source, BinaryOStream & stream ) const
	{
		TYPE value = TYPE();

		bool result = source.read( value );
		stream << value;
		return result;
	}

	bool fromStreamToSink( BinaryIStream & stream, DataSink & sink ) const
	{
		TYPE value;
		stream >> value;

		if (stream.error())
		{
			ERROR_MSG( "SimpleStreamElement: Not enough data on stream to read "
				"value\n");
			return false;
		}
		return sink.write( value );
	}
};

BW_END_NAMESPACE

#endif // SIMPLE_STREAM_ELEMENT_HPP
