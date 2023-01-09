#ifndef VISUAL_MANIPULATOR_NODE_HPP
#define VISUAL_MANIPULATOR_NODE_HPP

#include "math/matrix.hpp"
#include "resmgr/datasection.hpp"
#include "cstdmf/smartpointer.hpp"


BW_BEGIN_NAMESPACE

namespace VisualManipulator
{

class Node;
typedef SmartPointer<Node> NodePtr;

class Node : public ReferenceCount
{
public:
	Node();

	void save( DataSectionPtr pSection, bool recursive = true );
	Matrix worldTransform() const;

	bool isHardPoint() const;

	Matrix transform;
	BW::string identifier;

	bool included;

	Node* parent;
	BW::vector<NodePtr> children;
};

}

BW_END_NAMESPACE

#endif // VISUAL_MANIPULATOR_NODE_HPP
