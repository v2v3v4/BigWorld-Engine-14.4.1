#include "pch.hpp"

// Module Interface
#include "speedtree_config.hpp"

#if SPEEDTREE_SUPPORT // -------------------------------------------------------

#include "speedtree_vertex_types.hpp"



BW_BEGIN_NAMESPACE

namespace speedtree {

bool BranchVertex::getVertexFormat( Moo::VertexFormat & format )
{
	if (format.streamCount() != 0) 
	{ 
		WARNING_MSG("BranchVertex::getVertexFormat: got non-empty format\n");
		return false; 
	}

	format.addStream();

	// position_
	format.addElement( 0, 
		Moo::VertexElement::SemanticType::POSITION, 
		Moo::VertexElement::StorageType::FLOAT3 );

	// normal_
	format.addElement( 0, 
		Moo::VertexElement::SemanticType::NORMAL,
		Moo::VertexElement::StorageType::FLOAT3 );

	// texCoords_
	format.addElement( 0, 
		Moo::VertexElement::SemanticType::TEXCOORD, 
		Moo::VertexElement::StorageType::FLOAT2 );

	// windIndex_ (float1) + windWeight_ (float1)
	format.addElement( 0, 
		Moo::VertexElement::SemanticType::TEXCOORD, 
		Moo::VertexElement::StorageType::FLOAT2 );

#if SPT_ENABLE_NORMAL_MAPS
	// binormal_
	format.addElement( 0, 
		Moo::VertexElement::SemanticType::TEXCOORD, 
		Moo::VertexElement::StorageType::FLOAT3 );

	// tangent_
	format.addElement( 0, 
		Moo::VertexElement::SemanticType::TEXCOORD,
		Moo::VertexElement::StorageType::FLOAT3 );
#endif // SPT_ENABLE_NORMAL_MAPS

	return true;
}

bool LeafVertex::getVertexFormat( Moo::VertexFormat & format )
{
	if (format.streamCount() != 0) 
	{ 
		WARNING_MSG("LeafVertex::getVertexFormat: got non-empty format\n");
		return false; 
	}

	format.addStream();

	// position_
	format.addElement( 0, 
		Moo::VertexElement::SemanticType::POSITION, 
		Moo::VertexElement::StorageType::FLOAT3 );

	// normal_
	format.addElement( 0, 
		Moo::VertexElement::SemanticType::NORMAL,
		Moo::VertexElement::StorageType::FLOAT3 );

	// texCoords_
	format.addElement( 0,
		Moo::VertexElement::SemanticType::TEXCOORD, 
		Moo::VertexElement::StorageType::FLOAT2 );

	// windInfo_
	format.addElement( 0, 
		Moo::VertexElement::SemanticType::TEXCOORD, 
		Moo::VertexElement::StorageType::FLOAT2 );

	// rotInfo_
	format.addElement( 0, 
		Moo::VertexElement::SemanticType::TEXCOORD, 
		Moo::VertexElement::StorageType::FLOAT2 );

	// extraInfo_
	format.addElement( 0, 
		Moo::VertexElement::SemanticType::TEXCOORD, 
		Moo::VertexElement::StorageType::FLOAT3 );

	// geomInfo_
	format.addElement( 0, 
		Moo::VertexElement::SemanticType::TEXCOORD, 
		Moo::VertexElement::StorageType::FLOAT2 );

	// pivotInfo_
	format.addElement( 0, 
		Moo::VertexElement::SemanticType::TEXCOORD, 
		Moo::VertexElement::StorageType::FLOAT2 );

#if SPT_ENABLE_NORMAL_MAPS
	// binormal_
	format.addElement(0, 
		Moo::VertexElement::SemanticType::TEXCOORD, 
		Moo::VertexElement::StorageType::FLOAT3);

	// tangent_
	format.addElement(0, 
		Moo::VertexElement::SemanticType::TEXCOORD,
		Moo::VertexElement::StorageType::FLOAT3);
#endif // SPT_ENABLE_NORMAL_MAPS

	return true;
}


bool BillboardVertex::getVertexFormat( Moo::VertexFormat& format )
{
	if (format.streamCount() != 0) 
	{ 
		WARNING_MSG("BillboardVertex::getVertexFormat: got non-empty format\n");
		return false; 
	}

	format.addStream();

	// position_
	format.addElement( 0, 
		Moo::VertexElement::SemanticType::POSITION, 
		Moo::VertexElement::StorageType::FLOAT3 );

	// lightNormal_
	format.addElement( 0, 
		Moo::VertexElement::SemanticType::NORMAL,
		Moo::VertexElement::StorageType::FLOAT3 );
	
	// alphaNormal_
	format.addElement( 0, 
		Moo::VertexElement::SemanticType::TEXCOORD,
		Moo::VertexElement::StorageType::FLOAT3 );

	// texCoords_
	format.addElement( 0, 
		Moo::VertexElement::SemanticType::TEXCOORD, 
		Moo::VertexElement::StorageType::FLOAT2 );

	// binormal_
	format.addElement( 0, 
		Moo::VertexElement::SemanticType::TEXCOORD, 
		Moo::VertexElement::StorageType::FLOAT3 );

	// tangent_
	format.addElement( 0, 
		Moo::VertexElement::SemanticType::TEXCOORD,
		Moo::VertexElement::StorageType::FLOAT3 );

	return true;
}



}  // namespace speedtree

BW_END_NAMESPACE

#endif // SPEEDTREE_SUPPORT

// speedtree_tree_type.cpp
