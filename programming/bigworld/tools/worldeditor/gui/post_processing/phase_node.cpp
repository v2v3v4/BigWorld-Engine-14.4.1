
#include "pch.hpp"
#include "phase_node.hpp"
#include "post_processing/phase.hpp"
#include "post_processing/phase_factory.hpp"

#include "resmgr/xml_section.hpp"

BW_BEGIN_NAMESPACE

// Special render target name reserved for the back buffer.
/*static*/ const char * PhaseNode::BACK_BUFFER_STR = "backBuffer";


/**
 *	Constructor.
 *
 *	@param pyPhase	Post-processing Phase python object.
 *	@param callback	Object wishing to receive notifications from the node.
 */
PhaseNode::PhaseNode( PostProcessing::Phase * pyPhase, NodeCallback * callback ) :
	BasePostProcessingNode( callback ),
	pyPhase_( pyPhase ),
	name_( "" )
{
	BW_GUARD;

	PyObjectPtr pyPhaseName( PyObject_GetAttrString( pyPhase, "name" ), PyObjectPtr::STEAL_REFERENCE );
	name_ = pyPhaseName ? PyString_AS_STRING( pyPhaseName.get() ) : "<unknown_phase>";
}


/**
 *	Destructor.
 */
PhaseNode::~PhaseNode()
{
	BW_GUARD;
}


/**
 *	This method returns the Phase's output render target name.
 *
 *	@return The Phase's output render target name.
 */
BW::string PhaseNode::output() const
{
	BW_GUARD;

	BW::string ret( BACK_BUFFER_STR );
	
	PyObjectPtr pyRT( PyObject_GetAttrString( pyPhase_.get(), "renderTarget" ), PyObjectPtr::STEAL_REFERENCE );
	if (pyRT && pyRT.get() != Py_None)
	{
		PyObjectPtr pyRTName( PyObject_GetAttrString( pyRT.get(), "name" ),	PyObjectPtr::STEAL_REFERENCE );
		if (pyRTName)
		{
			ret = PyString_AS_STRING( pyRTName.get() );
		}
	}
	
	return ret;
}


/**
 *	This method creates a new Phase from this Phase.
 *
 *	@return New Phase, of the same type and params as this Phase.
 */
PhaseNodePtr PhaseNode::clone() const
{
	BW_GUARD;

	PhaseNodePtr newPhase;
	DataSectionPtr tempSection = new XMLSection( "phaseData" );
	if (pyPhase_->save( tempSection ))
	{
		// Save should create a child section in tempSection with the appropriate
		// name for this Phase's factory to load.
		if (tempSection->countChildren() == 1)
		{
			PostProcessing::PhasePtr newPyPhase(
				PostProcessing::PhaseFactory::loadItem( tempSection->openChild( 0 ) ), true );
			if (newPyPhase)
			{
				newPhase = new PhaseNode( newPyPhase.get(), callback() );
				newPhase->active( active() );
				newPhase->name_ = name_;
				newPhase->output_ = output_;
			}
		}
	}
	return newPhase;
}


/**
 *	This method retrieves all the properties of the Phase node for displaying
 *	them in the Properties area of the post-processing panel.
 *
 *	@param editor	Editor object that will receive and handle the properties.
 */
void PhaseNode::edEdit( GeneralEditor * editor )
{
	BW_GUARD;

	STATIC_LOCALISE_NAME( s_propLabel, "WORLDEDITOR/GUI/PAGE_POST_PROCESSING/PHASE/PROP_LABEL" );
	STATIC_LOCALISE_NAME( s_propLabelDesc, "WORLDEDITOR/GUI/PAGE_POST_PROCESSING/PHASE/PROP_LABEL_DESC" );

	TextProperty * propName = new TextProperty( s_propLabel,
			new GetterSetterProxy< PhaseNode, StringProxy >(
				this, "name",
			&PhaseNode::getName, 
			&PhaseNode::setName ) );

	propName->UIDesc( s_propLabelDesc );

	editor->addProperty( propName );

	pyPhase_->edEdit( editor );
}


/**
 *	This method loads the properties of the node from a data section.
 *
 *	@param section	Data section containing the properties for the node.
 *	@return	True if successful, false if not.
 */
bool PhaseNode::edLoad( DataSectionPtr pDS )
{
	return true;
}


/**
 *	This method saves the properties of the node to a data section.
 *
 *	@param section	Data section that will contain the properties for the node.
 *	@return	True if successful, false if not.
 */
bool PhaseNode::edSave( DataSectionPtr pDS )
{
	return true;
}


/**
 *	This method returns the name of this node.
 *
 *	@return The name of this node.
 */
BW::string PhaseNode::getName() const
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
bool PhaseNode::setName( const BW::string & name )
{
	BW_GUARD;

	PyObjectPtr pyName( Script::getData( name ), PyObjectPtr::STEAL_REFERENCE );

	PyObject_SetAttrString( pyPhase_.get(), "name", pyName.get() );

	name_ = name;

	BasePostProcessingNode::changed( true );
	WorldManager::instance().userEditingPostProcessing( true );

	return true;
}

BW_END_NAMESPACE

