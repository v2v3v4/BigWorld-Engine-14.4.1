#ifndef PHASE_NODE_HPP
#define PHASE_NODE_HPP


#include "base_post_processing_node.hpp"

BW_BEGIN_NAMESPACE

// Forward declarations
namespace PostProcessing
{
	class Phase;
	typedef SmartPointer<Phase> PhasePtr;
}

class PhaseNode;
typedef SmartPointer< PhaseNode > PhaseNodePtr;


/**
 *	This class implements a Phase node, implementing the necessary additional
 *	editor info for a Post-processing Phase.
 */
class PhaseNode : public BasePostProcessingNode
{
public:
	static const char * BACK_BUFFER_STR;

	PhaseNode( PostProcessing::Phase * pyPhase, NodeCallback * callback );
	~PhaseNode();

	const BW::string & name() const { return name_; }

	BW::string output() const;

	virtual PhaseNode * phaseNode() { return this; }

	PostProcessing::PhasePtr pyPhase() const { return pyPhase_; }

	PyObject * pyObject() const { return (PyObject *)(pyPhase_.get()); }

	PhaseNodePtr clone() const;

	void edEdit( GeneralEditor * editor );
	bool edLoad( DataSectionPtr pDS );
	bool edSave( DataSectionPtr pDS );

	BW::string getName() const;
	bool setName( const BW::string & name );

private:
	PostProcessing::PhasePtr pyPhase_;
	BW::string name_;
	BW::string output_;
};

BW_END_NAMESPACE

#endif // PHASE_NODE_HPP
