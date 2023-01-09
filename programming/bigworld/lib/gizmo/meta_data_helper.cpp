#include "pch.hpp"
#include "meta_data_helper.hpp"
#include "meta_data_define.hpp"
#include "undoredo.hpp"

BW_BEGIN_NAMESPACE

namespace MetaData
{

class PropertyOperation : public UndoRedo::Operation
{
public:
	PropertyOperation( MetaData& metaData,
		const BW::string& name ) :
		UndoRedo::Operation( 0 ),
		metaData_( metaData ),
		name_( name ),
		value_( metaData_[ name ]->get() )
	{}

protected:
	virtual void undo()
	{
		BW_GUARD;

		UndoRedo::instance().add( new PropertyOperation( metaData_, name_ ) );
		metaData_[ name_ ]->set( value_ );
	}
	virtual bool iseq( const UndoRedo::Operation & oth ) const
	{
		// these operations never replace each other
		return false;
	}
	MetaData& metaData_;
	BW::string name_;
	Any value_;
};

void updateCreationInfo( MetaData& metaData, time_t time, const BW::string& username )
{
	BW_GUARD;

	if (!metaData[ METADATA_CREATED_ON ] ||
		!metaData[ METADATA_CREATED_BY ] ||
		!metaData[ METADATA_MODIFIED_ON ] ||
		!metaData[ METADATA_MODIFIED_BY ])
	{
		// TODO: This should never happen. Make sure Xiaoming fixes this.
		WARNING_MSG( "Metadata::updateCreationInfo: metadata is empty.\n" );
		return;
	}

	if (!UndoRedo::instance().isUndoing())
	{
		metaData[ METADATA_CREATED_ON ]->set( time, true );
		metaData[ METADATA_CREATED_BY ]->set( username, true );
		metaData[ METADATA_MODIFIED_ON ]->set( time, true );
		metaData[ METADATA_MODIFIED_BY ]->set( username, true );
	}
}

void updateModificationInfo( MetaData& metaData, time_t time, const BW::string& username )
{
	BW_GUARD;

	static bool s_updating = false;

	if (!s_updating)
	{
		if (metaData[ METADATA_MODIFIED_ON ] && metaData[ METADATA_MODIFIED_BY ])
		{
			s_updating = true;

			if (!UndoRedo::instance().isUndoing())
			{
				UndoRedo::instance().add( new PropertyOperation( metaData, METADATA_MODIFIED_ON ) );
				UndoRedo::instance().add( new PropertyOperation( metaData, METADATA_MODIFIED_BY ) );
			}

			metaData[ METADATA_MODIFIED_ON ]->set( time );
			metaData[ METADATA_MODIFIED_BY ]->set( username );

			s_updating = false;
		}
		else
		{
			// TODO: This should never happen. Make sure Xiaoming fixes this.
			WARNING_MSG( "Metadata::updateModificationInfo: metadata is empty.\n" );
			return;
		}
	}
}

}//namespace MetaData
BW_END_NAMESPACE

