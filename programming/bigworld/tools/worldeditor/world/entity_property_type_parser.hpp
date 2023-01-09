#ifndef ENTITY_PROPERTY_TYPE_PARSER_HPP
#define ENTITY_PROPERTY_TYPE_PARSER_HPP


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"

BW_BEGIN_NAMESPACE

/**
 *	This interface is used by EditorChunkEntity to abstract a property's type
 *  when parsing the property's enums and/or widget. Extend it to add support
 *  for other data types and widgets.
 */
class EntityPropertyTypeParser : public ReferenceCount
{
public:
	// interface
	/**
	 *	Checks if a python object is of a valid type for the parser.
	 *
	 *	@param val		Python value to test
	 *	@return			true of val is a recognised type in the parser.
	 */
	virtual bool checkVal( PyObject* val ) = 0;
	
	/**
	 *	Adds a python ENUM entry to the parser's internal enum map, if it has
	 *  one, and returns the index of the enum corresponding to val.
	 *
	 *	@param val		Python value to be associated to an index.
	 *	@param index	Ascending index of the ENUM in order of appearance,
	 *					starting from 0, in the editor/<entity>.py file.
	 *	@return			index desired for the enum 'val'.
	 */
	virtual int addEnum( PyObject* val, int index ) = 0;
	
	/**
	 *	Adds a xml ENUM entry to the parser's internal enum map, if it has
	 *  one, and returns the index of the enum corresponding to val.
	 *
	 *	@param val		XML value to be associated to an index.
	 *	@param index	Ascending index of the ENUM in order of appearance,
	 *					starting from 0, in the alias.xml or def file.
	 *	@return			index desired for the enum 'val'.
	 */
	virtual int addEnum( DataSectionPtr val, int index ) = 0;
	
	/**
	 *	Creates a plain property according to the description of of the
	 *	data type and the parser, with no associated widgets.
	 *
	 *	@param props		Property helper the property is associated to.
	 *	@param propIndex	Index of the property in the list.
	 *	@param pDD			Entity property data description
	 *	@return				Resulting property.
	 */
	virtual GeneralProperty* plainProperty(
		BasePropertiesHelper* props,
		int propIndex,
		const Name& name,
		DataTypePtr dataType ) = 0;

	/**
	 *	Creates an enumerated property, according to the description of the
	 *  data type and the parser.
	 *
	 *	@param props		Property helper to associate the property to.
	 *	@param propIndex	Index of the property in the list.
	 *	@param pDD			Entity property data description.
	 *	@param choices		Data section that contains the ENUMs.
	 *	@return				The plain property of the parser
	 */
	virtual GeneralProperty* enumProperty(
		BasePropertiesHelper* props,
		int propIndex,
		const Name& name,
		DataTypePtr dataType,
		DataSectionPtr choices );
	
	/**
	 *	Creates a property with the Radius gizmo, according to the description
	 *  of the data type and the parser.
	 *
	 *	@param props		Property helper to associate the property to.
	 *	@param propIndex	Index of the property in the list.
	 *	@param pDD			Entity property data description.
	 *	@param pMP			Matrix proxy that stores the radius.
	 *	@param widgetColour	Radius widget colour.
	 *	@param widgetRadius	Radius of the widget.
	 *	@return				The plain property of the parser
	 */
	virtual GeneralProperty* radiusProperty(
		BasePropertiesHelper* props,
		int propIndex,
		const Name& name,
		DataTypePtr dataType,
		MatrixProxy* pMP,
		uint32 widgetColour,
		float widgetRadius );

	// statics
	class Factory: public ReferenceCount
	{
	public:
		virtual EntityPropertyTypeParserPtr create(
			const Name& name, DataTypePtr dataType ) = 0;
	};

	static void registerFactory( Factory * factory );
	static EntityPropertyTypeParserPtr create( const Name& name, DataTypePtr dataType );

private:
	static BW::vector<Factory *> s_factories_;
};

BW_END_NAMESPACE

#endif // ENTITY_PROPERTY_TYPE_PARSER_HPP
