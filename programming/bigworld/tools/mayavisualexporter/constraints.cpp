#include "pch.hpp"

#include "constraints.hpp"
#include "exportiterator.h"
#include "utility.hpp"

#include <maya/MPlugArray.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnAttribute.h>
#include <maya/MFnIkHandle.h>
#include <maya/MFnIkJoint.h>
#include <maya/MFnIkEffector.h>

BW_BEGIN_NAMESPACE

/**
 *	This static method saves out the constraints found in the current scene
 */
void Constraints::saveConstraints( DataSectionPtr pVisualSection )
{
	DataSectionPtr pConstraintsSection = 
		pVisualSection->newSection( "constraints" );

	MStatus status;
	ExportIterator orientIt( MFn::kOrientConstraint, &status );
	for (; orientIt.isDone() == false; orientIt.next())
	{
		writeOrientConstraint( orientIt.item(),
			pConstraintsSection );
	}

	ExportIterator pointIt( MFn::kPointConstraint, &status );
	for (; pointIt.isDone() == false; pointIt.next())
	{
		writePointConstraint( pointIt.item(),
			pConstraintsSection );
	}

	ExportIterator parentIt( MFn::kParentConstraint, &status );
	for (; parentIt.isDone() == false; parentIt.next())
	{
		writeParentConstraint( parentIt.item(),
			pConstraintsSection );
	}

	ExportIterator aimIt( MFn::kAimConstraint, &status );
	for (; aimIt.isDone() == false; aimIt.next())
	{
		writeAimConstraint( aimIt.item(),
			pConstraintsSection );
	}

	ExportIterator ikHandleIt( MFn::kIkHandle, &status );
	for (; ikHandleIt.isDone() == false; ikHandleIt.next())
	{
		writeIKHandle( ikHandleIt.item(),
			pConstraintsSection );
	}
}

namespace
{
	/**
	 *	Helper function to get an object connected to a plug
	 *	@param plug the plug to get the connection from
	 *	@param wantSource true if we want the object the plug is
	 *		connected to as source, false otherwise
	 *	@param o_connected return value for the object connected to the plug
	 *	@return false if an object could not be found
	 */
	bool getConnectedObject( const MPlug& plug, 
		bool wantSource, MObject& o_connected )
	{
		// get a list of connections to the plug
		MPlugArray plugs;
		plug.connectedTo( plugs, !wantSource, wantSource );

		// if we have a connection
		if (plugs.length() > 0) 
		{
			o_connected = plugs[0].node();
			return true;
		}

		return false;
	}


	/**
	 *	Helper function to get a Vector3 from a plug
	 *	@param plug the plug to get the Vector3 from
	 *	@return the Vector3 value of the plug
	 */
	Vector3 vector3FromPlug( const MPlug& plug )
	{
		return Vector3( plug.child(0).asFloat(),
			plug.child(1).asFloat(),
			plug.child(2).asFloat() );
	}

}


/**
 * Write out an orient constraint to the provided datasection
 * @param object the orient constraint object
 * @param pConstraintsSection the parent datasection to write the orient 
 *	constraint to
 */
void Constraints::writeOrientConstraint( const MObject& object, 
										  DataSectionPtr pConstraintsSection )
{
	MStatus status;
	MFnDependencyNode orientConstraintFn( object, &status );

	DataSectionPtr pOrientConstraintSection = 
		pConstraintsSection->newSection( "orientConstraint" );
	
	pOrientConstraintSection->writeString( "identifier", 
		orientConstraintFn.name().asChar() );

	MObject targetNodeObj;

	if (getConnectedObject( 
		orientConstraintFn.findPlug( "constraintRotateX" ), true, targetNodeObj ))
	{
		MFnDependencyNode targetNode( targetNodeObj );
		pOrientConstraintSection->writeString( 
			"target", targetNode.name().asChar() );
	}

	const Vector3 offsetAngle = vector3FromPlug(
		orientConstraintFn.findPlug( "offset" ) );
	pOrientConstraintSection->writeVector3( "offset",
		convertAngleOffsetToBigWorldUnits( offsetAngle ) );

	MObject targetArrayObj = orientConstraintFn.attribute( "target" );
	MPlug arrayPlug( object, targetArrayObj );

	for (uint i = 0; i < arrayPlug.numElements(); ++i)
	{
		DataSectionPtr pSourceSection = 
			pOrientConstraintSection->newSection( "source" );
		MPlug sourcePlug = arrayPlug.elementByLogicalIndex( i );

		for (uint j = 0; j < sourcePlug.numChildren(); ++j)
		{
			MPlug propertyPlug = sourcePlug.child( j );
			MFnAttribute propertyAttribute( propertyPlug.attribute() );

			if (propertyAttribute.name() == "targetRotate")
			{
				MObject destNodeObj;
				if (getConnectedObject( 
					propertyPlug, false, destNodeObj ))
				{
					MFnDependencyNode destNode( destNodeObj );
					pSourceSection->writeString( 
						"identifier", destNode.name().asChar() );
				}
			}

			if (propertyAttribute.name() == "targetWeight")
			{
				pSourceSection->writeFloat( "weight", propertyPlug.asFloat() );
			}
		}
	}
}


/**
 * Write out a point constraint to the provided datasection
 * @param object the point constraint object
 * @param pConstraintsSection the parent datasection to write the point 
 *	constraint to
 */
void Constraints::writePointConstraint( const MObject& object, 
										 DataSectionPtr pConstraintsSection )
{
	MStatus status;
	MFnDependencyNode pointConstraintFn( object, &status );

	// Check if the point constraint is actually a pole vector
	// constraint and export it as such if it is
	DataSectionPtr pPointConstraintSection;
	if (object.hasFn( MFn::kPoleVectorConstraint ))
	{
		pPointConstraintSection = 
			pConstraintsSection->newSection( "poleVectorConstraint" );
	}
	else
	{
		pPointConstraintSection = 
			pConstraintsSection->newSection( "pointConstraint" );
	}

	pPointConstraintSection->writeString( "identifier", 
		pointConstraintFn.name().asChar() );

	MObject targetNodeObj;

	if (getConnectedObject( 
		pointConstraintFn.findPlug( "constraintTranslateX" ), true, targetNodeObj ))
	{
		// We currently ignore unit conversion nodes as these seem to only be
		// used by pole vectors
		if (targetNodeObj.apiType() == MFn::kUnitConversion)
		{
			MFnDependencyNode unitConversionFn( targetNodeObj, &status );
			MObject unitConversionTargetObj;
			if (getConnectedObject( 
				unitConversionFn.findPlug( "output" ), true, 
				unitConversionTargetObj ))
			{
				targetNodeObj = unitConversionTargetObj;
			}
		}

		MFnDependencyNode targetNode( targetNodeObj );
		pPointConstraintSection->writeString( 
			"target", targetNode.name().asChar() );
	}

	Vector3 offset = vector3FromPlug( pointConstraintFn.findPlug( "offset" ) );
	pPointConstraintSection->writeVector3( "offset", 
		convertPointToBigWorldUnits( offset ) );

	MObject sourceArrayObj = pointConstraintFn.attribute( "target" );
	MPlug arrayPlug( object, sourceArrayObj );

	for (uint i = 0; i < arrayPlug.numElements(); ++i)
	{
		DataSectionPtr pSourceSection = 
			pPointConstraintSection->newSection( "source" );
		MPlug sourcePlug = arrayPlug.elementByLogicalIndex( i );

		for (uint j = 0; j < sourcePlug.numChildren(); ++j)
		{
			MPlug propertyPlug = sourcePlug.child( j );
			MFnAttribute propertyAttribute( propertyPlug.attribute() );

			if (propertyAttribute.name() == "targetTranslate")
			{
				MObject destNodeObj;

				if (getConnectedObject( 
					propertyPlug, false, destNodeObj ))
				{
					MFnDependencyNode destNode( destNodeObj );
					pSourceSection->writeString( 
						"identifier", destNode.name().asChar() );
				}
			}

			if (propertyAttribute.name() == "targetWeight")
			{
				pSourceSection->writeFloat( "weight", propertyPlug.asFloat() );
			}
		}
	}
}


/**
 * Write out a parent constraint to the provided datasection
 * @param object the parent constraint object
 * @param pConstraintsSection the parent datasection to write the parent 
 *	constraint to
 */
void Constraints::writeParentConstraint( const MObject& object, 
										DataSectionPtr pConstraintsSection )
{
	MStatus status;
	MFnDependencyNode parentConstraintFn( object, &status );

	DataSectionPtr pParentConstraintSection = 
		pConstraintsSection->newSection( "parentConstraint" );

	pParentConstraintSection->writeString( "identifier", 
		parentConstraintFn.name().asChar() );

	MObject targetNodeObj;

	if (getConnectedObject( 
		parentConstraintFn.findPlug( "constraintRotateX" ), true, targetNodeObj ))
	{
		MFnDependencyNode targetNode( targetNodeObj );
		pParentConstraintSection->writeString( 
			"target", targetNode.name().asChar() );
	}

	MObject srcArrayObj = parentConstraintFn.attribute( "target" );
	MPlug arrayPlug( object, srcArrayObj );

	for (uint i = 0; i < arrayPlug.numElements(); ++i)
	{
		DataSectionPtr pTargetSection = 
			pParentConstraintSection->newSection( "source" );
		MPlug sourcePlug = arrayPlug.elementByLogicalIndex( i );

		for (uint j = 0; j < sourcePlug.numChildren(); ++j)
		{
			MPlug propertyPlug = sourcePlug.child( j );
			MFnAttribute propertyAttribute( propertyPlug.attribute() );

			MString propertyName = propertyAttribute.name();

			if (propertyName == "targetTranslate")
			{
				MObject destNodeObj;
				if (getConnectedObject( 
					propertyPlug, false, destNodeObj ))
				{
					MFnDependencyNode destNode( destNodeObj );
					pTargetSection->writeString( 
						"identifier", destNode.name().asChar() );
				}
			}

			if (propertyName == "targetWeight")
			{
				pTargetSection->writeFloat( "weight", propertyPlug.asFloat() );
			}

			if (propertyName == "targetOffsetTranslate")
			{
				Vector3 translateOffset = vector3FromPlug( propertyPlug );
				pTargetSection->writeVector3( "translateOffset", 
					convertPointToBigWorldUnits( translateOffset ) );
			}

			if (propertyName == "targetOffsetRotate")
			{
				const Vector3 rotateOffset = vector3FromPlug( propertyPlug );
				pTargetSection->writeVector3( "rotateOffset",
					convertAngleOffsetToBigWorldUnits( rotateOffset ) );
			}
		}
	}
}


/**
 * Write out a aim constraint to the provided datasection
 * @param object the aim constraint object
 * @param pConstraintsSection the parent datasection to write the aim 
 * constraint to
 */
void Constraints::writeAimConstraint( const MObject& object, 
									 DataSectionPtr pConstraintsSection )
{
	MStatus status;
	MFnDependencyNode aimConstraintFn( object, &status );

	DataSectionPtr pAimConstraintSection = 
		pConstraintsSection->newSection( "aimConstraint" );

	pAimConstraintSection->writeString( "identifier", 
		aimConstraintFn.name().asChar() );

	MObject targetNodeObj;

	if (getConnectedObject( 
		aimConstraintFn.findPlug( "constraintRotateX" ), true, targetNodeObj ))
	{
		MFnDependencyNode srcNode( targetNodeObj );
		pAimConstraintSection->writeString( 
			"target", srcNode.name().asChar() );
	}

	const Vector3 offset = vector3FromPlug(
		aimConstraintFn.findPlug( "offset" ) );
	pAimConstraintSection->writeVector3( "offset",
		convertAngleOffsetToBigWorldUnits( offset ) );

	Vector3 aimVector = vector3FromPlug( aimConstraintFn.findPlug( "aimVector" ) );
	pAimConstraintSection->writeVector3( "aimVector", 
		convertVectorToBigWorldUnits( aimVector) );

	Vector3 upVector = vector3FromPlug( aimConstraintFn.findPlug( "upVector" ) );
	pAimConstraintSection->writeVector3( "upVector", 
		convertVectorToBigWorldUnits( upVector ) );

	Vector3 worldUpVector = 
		vector3FromPlug( aimConstraintFn.findPlug( "worldUpVector" ) );
	pAimConstraintSection->writeVector3( "worldUpVector", 
		convertVectorToBigWorldUnits( worldUpVector ) );

	MObject sourceArrayObj = aimConstraintFn.attribute( "target" );
	MPlug arrayPlug( object, sourceArrayObj );

	for (uint i = 0; i < arrayPlug.numElements(); ++i)
	{
		DataSectionPtr pSourceSection = 
			pAimConstraintSection->newSection( "source" );
		MPlug sourcePlug = arrayPlug.elementByLogicalIndex( i );

		for (uint j = 0; j < sourcePlug.numChildren(); ++j)
		{
			MPlug propertyPlug = sourcePlug.child( j );
			MFnAttribute propertyAttribute( propertyPlug.attribute() );

			MString propertyName = propertyAttribute.name();

			if (propertyAttribute.name() == "targetTranslate")
			{
				MObject destNodeObj;
				if (getConnectedObject( 
					propertyPlug, false, destNodeObj ))
				{
					MFnDependencyNode destNode( destNodeObj );
					pSourceSection->writeString( 
						"identifier", destNode.name().asChar() );
				}
			}

			if (propertyAttribute.name() == "targetWeight")
			{
				pSourceSection->writeFloat( "weight", propertyPlug.asFloat() );
			}
		}
	}
}


/**
 * Write out an IK handle to the provided datasection
 * @param object the IK handle object
 * @param pConstraintsSection the parent datasection to write the IK handle to
 */
void Constraints::writeIKHandle( const MObject& object, 
								  DataSectionPtr pConstraintsSection )
{
	MStatus status;
	MFnIkHandle ikHandleFn( object, &status );

	DataSectionPtr pIKHandleSection = 
		pConstraintsSection->newSection( "ikHandle" );

	pIKHandleSection->writeString( "identifier", ikHandleFn.name().asChar() );

	MDagPath jointPath;
	MDagPath effectorPath;
	ikHandleFn.getStartJoint( jointPath );
	ikHandleFn.getEffector( effectorPath );

	MFnDependencyNode jointNode( jointPath.node() );
	MFnDependencyNode effectorNode( effectorPath.node() );

	pIKHandleSection->writeString( "startJoint", jointNode.name().asChar() );

	// Export the node the end effector uses as its source
	MObject effectorSource;
	if (getConnectedObject( 
		effectorNode.findPlug( "translateX" ), false, effectorSource ))
	{
		MFnDependencyNode effectorSrcNode( effectorSource );
		pIKHandleSection->writeString( "endJoint", 
			effectorSrcNode.name().asChar() );
	}

	MObject translateSource;
	if (getConnectedObject( 
		ikHandleFn.findPlug( "translateX" ), false, translateSource ))
	{
		MFnDependencyNode translateNode( translateSource );
		pIKHandleSection->writeString( 
			"source/node", translateNode.name().asChar() );
	}
	else
	{
		Vector3 translate = 
			vector3FromPlug( ikHandleFn.findPlug( "translate" ) );
		pIKHandleSection->writeVector3( "source/vector",
			convertPointToBigWorldUnits( translate ) );
	}

	MObject poleVectorSource;
	if (getConnectedObject( 
		ikHandleFn.findPlug( "poleVectorX" ), false, poleVectorSource ))
	{
		// We currently ignore unit conversion nodes as these seem to only be
		// used by pole vectors
		if (poleVectorSource.apiType() == MFn::kUnitConversion)
		{
			MFnDependencyNode unitConversionFn( poleVectorSource, &status );
			MObject unitConversionSrcObj;
			if (getConnectedObject( 
				unitConversionFn.findPlug( "input" ), false, 
				unitConversionSrcObj ))
			{
				poleVectorSource = unitConversionSrcObj;
			}
		}

		MFnDependencyNode poleVectorNode( poleVectorSource );
		pIKHandleSection->writeString( 
			"poleVector/node", poleVectorNode.name().asChar() );
	}
	else
	{
		Vector3 poleVector = 
			vector3FromPlug( ikHandleFn.findPlug( "poleVector" ) );
		pIKHandleSection->writeVector3( "poleVector/vector",
			 convertVectorToBigWorldUnits( poleVector ) );
	}
}

BW_END_NAMESPACE

