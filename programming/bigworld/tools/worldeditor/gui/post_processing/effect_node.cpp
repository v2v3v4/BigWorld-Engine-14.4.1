
#include "pch.hpp"
#include "effect_node.hpp"
#include "post_processing/effect.hpp"

#include "resmgr/xml_section.hpp"

BW_BEGIN_NAMESPACE

/**
 *	Constructor.
 *
 *	@param pyEffect	Post-processing Effect python object.
 *	@param callback	Object wishing to receive notifications from the node.
 */
EffectNode::EffectNode( PostProcessing::Effect * pyEffect, NodeCallback * callback ) :
	BasePostProcessingNode( callback ),
	pyEffect_( pyEffect ),
	name_( "" )
{
	BW_GUARD;

	PyObjectPtr pyEffectName( PyObject_GetAttrString( pyEffect, "name" ), PyObjectPtr::STEAL_REFERENCE );
	name_ = pyEffectName ? PyString_AS_STRING( pyEffectName.get() ) : "<unknown_effect>";

	activeNoCallback( getBypass() != Vector4( 0.f, 0.f, 0.f, 0.f ) );
}


/**
 *	Destructor.
 */
EffectNode::~EffectNode()
{
	BW_GUARD;
}


/**
 *	This method returns whether or not the node is active.
 *
 *	@return Whether or not the node is active.
 */
bool EffectNode::active() const
{
	BW_GUARD;

	return BasePostProcessingNode::active();
}


/**
 *	This method sets whether or not the node is active and tells the callback
 *	object.
 *
 *	@param active Whether or not the node is active.
 */
void EffectNode::active( bool active )
{
	BW_GUARD;

	BasePostProcessingNode::active( active );

	Vector4 bypass = (BasePostProcessingNode::active() ? Vector4( 1.f, 1.f, 1.f, 1.f ) : Vector4( 0.f, 0.f, 0.f, 0.f ));

	PyObjectPtr pyBypass( Script::getData( bypass ), PyObjectPtr::STEAL_REFERENCE );

	PyObject_SetAttrString( pyEffect_.get(), "bypass", pyBypass.get() );
}


/**
 *	This method creates a new Effect from this Effect.
 *
 *	@return New Effect, of the same type and params as this Effect.
 */
EffectNodePtr EffectNode::clone() const
{
	BW_GUARD;

	DataSectionPtr tempSection = new XMLSection( "effectData" );
	pyEffect_->save( tempSection );
	PostProcessing::EffectPtr newPyEffect( new PostProcessing::Effect(), true );
	newPyEffect->load( tempSection->openSection( "Effect" ) );
	EffectNodePtr newEffect = new EffectNode( newPyEffect.get(), callback() );
	newEffect->active( active() );
	newEffect->name_ = name_;
	return newEffect;
}


/**
 *	This method retrieves all the properties of the Effect node for displaying
 *	them in the Properties area of the post-processing panel.
 *
 *	@param editor	Editor object that will receive and handle the properties.
 */
void EffectNode::edEdit( GeneralEditor * editor )
{
	BW_GUARD;

	STATIC_LOCALISE_NAME( s_propLabel, "WORLDEDITOR/GUI/PAGE_POST_PROCESSING/EFFECT/PROP_LABEL" );
	STATIC_LOCALISE_NAME( s_propLabelDesc, "WORLDEDITOR/GUI/PAGE_POST_PROCESSING/EFFECT/PROP_LABEL_DESC" );
	STATIC_LOCALISE_NAME( s_bypass, "WORLDEDITOR/GUI/PAGE_POST_PROCESSING/EFFECT/BYPASS" );
	STATIC_LOCALISE_NAME( s_bypassDesc, "WORLDEDITOR/GUI/PAGE_POST_PROCESSING/EFFECT/BYPASS_DESC" );

	TextProperty * propName = new TextProperty( s_propLabel,
			new GetterSetterProxy< EffectNode, StringProxy >(
				this, "name",
			&EffectNode::getName, 
			&EffectNode::setName ) );

	propName->UIDesc( s_propLabelDesc );

	editor->addProperty( propName );

	Vector4Property * propBypass = new Vector4Property( s_bypass,
		new GetterSetterProxy< EffectNode, Vector4Proxy >(
			this, "bypass",
			&EffectNode::getBypass, 
			&EffectNode::setBypass ) );

	propBypass->UIDesc( s_bypassDesc );

	editor->addProperty( propBypass );

	pyEffect_->edEdit( editor );

	BasePostProcessingNode::active( getBypass() != Vector4( 0.f, 0.f, 0.f, 0.f ) );
}


/**
 *	This method loads the properties of the node from a data section.
 *
 *	@param section	Data section containing the properties for the node.
 *	@return	True if successful, false if not.
 */
bool EffectNode::edLoad( DataSectionPtr pDS )
{
	return true;
}


/**
 *	This method saves the properties of the node to a data section.
 *
 *	@param section	Data section that will contain the properties for the node.
 *	@return	True if successful, false if not.
 */
bool EffectNode::edSave( DataSectionPtr pDS )
{
	return true;
}


/**
 *	This method returns the name of this node.
 *
 *	@return The name of this node.
 */
BW::string EffectNode::getName() const
{
	BW_GUARD;

	return name_;
}


/**
 *	This method sets the name for this node.
 *
 *	@param name		The new name for this node.
 *	@return	True if successful, false if not.
 */
bool EffectNode::setName( const BW::string & name )
{
	BW_GUARD;

	PyObjectPtr pyName( Script::getData( name ), PyObjectPtr::STEAL_REFERENCE );

	PyObject_SetAttrString( pyEffect_.get(), "name", pyName.get() );

	name_ = name;

	BasePostProcessingNode::changed( true );
	WorldManager::instance().userEditingPostProcessing( true );

	return true;
}


/**
 *	This method returns the bypass vector of this Effect.
 *
 *	@return The bypass vector of this Effect.
 */
Vector4 EffectNode::getBypass() const
{
	BW_GUARD;

	Vector4 ret( 1.f, 1.f, 1.f, 1.f );

	PyObjectPtr pyBypass( PyObject_GetAttrString( pyEffect_.get(), "bypass" ), PyObjectPtr::STEAL_REFERENCE );
	if (Vector4Provider::Check( pyBypass.get() ))
	{
		PyObjectPtr pyValue( PyObject_GetAttrString( pyBypass.get(), "value" ), PyObjectPtr::STEAL_REFERENCE );
		if (PyVector<Vector4>::Check( pyValue.get() ))
		{
			Vector4 bypass;
			if (Script::setData( pyValue.get(), bypass ) == 0)
			{
				ret = bypass;
			}
			else
			{
				PyErr_Print();
				PyErr_Clear();
			}
		}
	}

	return ret;
}


/**
 *	This method sets the bypass vector of this Effect.
 *
 *	@param bypass	The new bypass vector for this Effect.
 */
bool EffectNode::setBypass( const Vector4 & bypass )
{
	BW_GUARD;

	PyObjectPtr pyBypass( Script::getData( bypass ), PyObjectPtr::STEAL_REFERENCE );

	PyObject_SetAttrString( pyEffect_.get(), "bypass", pyBypass.get() );

	BasePostProcessingNode::active( bypass != Vector4( 0.f, 0.f, 0.f, 0.f ) );
	WorldManager::instance().userEditingPostProcessing( true );

	return true;
}

BW_END_NAMESPACE

