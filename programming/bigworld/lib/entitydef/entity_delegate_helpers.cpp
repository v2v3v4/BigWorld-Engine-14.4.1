#include "pch.hpp"

#include "cstdmf/debug.hpp"

#include "entity_delegate_helpers.hpp"
#include "script_data_sink.hpp"
#include "script_data_source.hpp"

#include "entitydef_script/py_component.hpp"
#include "delegate_interface/entity_delegate.hpp"
#include "script/script_object.hpp"


BW_BEGIN_NAMESPACE

/**
 * This function will create a dictionary with local or/and delegate properties
 * @param const ScriptObject & entity - is optional, could contain empty object 
 * 										to skip local properties 
 * @param IEntityDelegate * pEntityDelegate - is optional, could contain empty object 
 * 										to skip Delegate properties
 * @return ScriptDict which can hold empty object in case of error   
 * 
 * NOTE: an Entity class instance should be passed using the following wrapper:
 *   ScriptObject( pEntity, ScriptObject::FROM_BORROWED_REFERENCE )
 */
ScriptDict createDictWithAllProperties( 
		const EntityDescription & entityDesription,
		const ScriptObject & entity,
		IEntityDelegate * pEntityDelegate, 
		int dataDomains )
{
	class AddToDictFromPropertiesVisitor : public IDataDescriptionVisitor
	{
	public:
		AddToDictFromPropertiesVisitor(
				const char * entityName,
				const ScriptObject & entity,
				IEntityDelegate * pEntityDelegate,
				bool isPersistentOnly ) :
			entityName_( entityName ? entityName : "" ),
			entity_( entity ),
			pEntityDelegate_( pEntityDelegate ),
			isPersistentOnly_(isPersistentOnly)
		{
			dict_ = ScriptDict::create();
		}

		ScriptDict dict() const
		{
			return dict_;
		}
		
		virtual bool visit( const DataDescription& propDesc )
		{
			ScriptObject resultObj;
			if (!propDesc.isComponentised())
			{
				if (!entity_)
				{
					// Delegate property mode, skip local property
					return true;
				}
				
				resultObj = propDesc.getAttrFrom( entity_ );
				if (!resultObj)
				{
					ERROR_MSG( "AddToDictFromPropertiesVisitor: "
							"Could not get local property,"
							" entity description %s, property: %s\n",
							entityName_, propDesc.name().c_str() );
					
					Script::printError();
					return false;
				}
			}
			else
			{
				if (!pEntityDelegate_)
				{
					// Local property mode, skip delegate property
					return true;
				}
				
				resultObj = PyComponent::getAttribute( *pEntityDelegate_,
						propDesc, isPersistentOnly_ );
					
				if (!resultObj)
				{
					Script::printError();
					return false;
				}
			}
			
			// Insert to dict
			if (!propDesc.insertItemInto( dict_, resultObj ))
			{
				ERROR_MSG( "AddToDictFromPropertiesVisitor: "
						"Could not insert to dict,"
						" entity description %s, property: %s\n",
						entityName_, propDesc.name().c_str() );
				
				Script::printError();
				return false;
			}
			return true;
		}

	private:
		const char * entityName_;
		const ScriptObject & entity_;
		IEntityDelegate * pEntityDelegate_;
		bool isPersistentOnly_;
		ScriptDict dict_;
	};

	AddToDictFromPropertiesVisitor visitor( entityDesription.name().c_str(),
			entity, pEntityDelegate,
			(dataDomains & EntityDescription::ONLY_PERSISTENT_DATA) != 0 );
	if (!entityDesription.visit( dataDomains, visitor ))
	{
		ERROR_MSG( "createDictWithAllProperties: "
				"EntityDesription::visit() failed,"
				" entity description %s\n",
				entityDesription.name().c_str() );
		return ScriptDict();
	}
	return visitor.dict();
}

bool populateDelegateWithDict( 
		const EntityDescription & entityDescription,
		IEntityDelegate * pEntityDelegate, 
		const ScriptDict & dict,
		int dataDomains )
{
	class AddToDelegateFromDictVisitor : public IDataDescriptionVisitor
	{
	public:
		AddToDelegateFromDictVisitor( IEntityDelegate * pDelegate,
				const ScriptDict & pDict, bool isPersistentOnly ) :
			pDelegate_( pDelegate ),
			pDict_( pDict ),
			isPersistentOnly_( isPersistentOnly )
		{};

		virtual bool visit( const DataDescription& propDesc )
		{
			if (!propDesc.isComponentised())
			{
				// Skip non-component members
				return true;
			}

			ScriptObject pObject = propDesc.getItemFrom( pDict_ );
			if (pObject)
			{
				if (!PyComponent::setAttribute( *pDelegate_, propDesc, pObject,
												 isPersistentOnly_ ))
				{
					Script::printError();
					return false;
				}
			}

			return true;
		}

	private:
		IEntityDelegate * pDelegate_;
		ScriptDict pDict_;
		bool isPersistentOnly_;
	};

	AddToDelegateFromDictVisitor visitor( pEntityDelegate, dict,
		(dataDomains & EntityDescription::ONLY_PERSISTENT_DATA) != 0 );
	return entityDescription.visit( dataDomains, visitor );
}

BW_END_NAMESPACE
