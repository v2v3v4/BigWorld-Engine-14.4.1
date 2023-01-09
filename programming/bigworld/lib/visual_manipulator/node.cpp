#include "pch.hpp"

#include "node.hpp"


BW_BEGIN_NAMESPACE

using namespace VisualManipulator;

Node::Node() :
included(true),
parent(NULL)
{
	transform.setIdentity();
}

Matrix Node::worldTransform() const
{
	Matrix m = transform;

	NodePtr pParent = parent;
	while (pParent != NULL)
	{
		m.postMultiply( pParent->transform );
		pParent = pParent->parent;
	}

	return m;
}


void Node::save( DataSectionPtr pSection, bool recursive )
{
	DataSectionPtr pNodeSection = pSection;
	if (included)
	{
		pNodeSection = pSection->newSection("node");
		if (pNodeSection)
		{
			pNodeSection->writeString( "identifier", identifier );
			pNodeSection->writeVector3( "transform/row0", transform[0] );
			pNodeSection->writeVector3( "transform/row1", transform[1] );
			pNodeSection->writeVector3( "transform/row2", transform[2] );
			pNodeSection->writeVector3( "transform/row3", transform[3] );
		}
	}

	if (pNodeSection)
	{
		BW::vector<NodePtr>::iterator nodeIt = children.begin();

		while (nodeIt != children.end() && recursive)
		{
			(*nodeIt)->save( pNodeSection, recursive );
			++nodeIt;
		}
	}
}


bool Node::isHardPoint() const
{
	return identifier.length() >= 3 && identifier.substr(0,3) == BW::string("HP_");
}

BW_END_NAMESPACE
