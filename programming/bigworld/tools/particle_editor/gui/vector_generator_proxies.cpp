#include "pch.hpp"
#include "vector_generator_proxies.hpp"
#include "gizmo/combination_gizmos.hpp"
#include "gizmo/position_gizmo.hpp"
#include "moo/geometrics.hpp"

BW_BEGIN_NAMESPACE

VectorGeneratorGizmoProperties::VectorGeneratorGizmoProperties()
:
gizmo_(NULL),
referenceMatrix_(NULL),
referenceMatrix2_(NULL)
{
}

CreateGizmoInfo::CreateGizmoInfo()
:
vectorGenerator_(NULL),
wireColor_(0xff0000ff),
keepCurrentEditors_(false),
drawVectors_(false),
visualOffsetMatrixProxy_(NULL),
initialPosition_(NULL),
setDefaultRadius_(false),
radiusGizmoRadius_(4.f)
{
}


VectorGeneratorGizmoProperties AddVectorGeneratorGizmo
(
    CreateGizmoInfo const &info, 
    bool            drawIt
)
{
	BW_GUARD;

	VectorGenerator *vectorGenerator    = info.vectorGenerator_;
	Moo::Colour     wireColor           = info.wireColor_;
	bool            keepCurrentEditors  = info.keepCurrentEditors_;
	bool            drawVectors         = info.drawVectors_;
	MatrixProxyPtr  originMatrixProxy   = info.visualOffsetMatrixProxy_;
	Vector3         *initialPosition    = info.initialPosition_;
	bool            setDefaultRadius    = info.setDefaultRadius_;
    float           scale               = info.scale_;

	GeneralEditorPtr generalsDaughter   = new GeneralEditor();
	MatrixProxyPtr  returnMatrix        = NULL;
	MatrixProxyPtr  returnMatrix2       = NULL;
	GizmoPtr        newGizmo            = NULL;

	if (vectorGenerator->nameID() == PointVectorGenerator::nameID_)
	{
		PointVectorGenerator * pointVectorGenerator = 
            static_cast<PointVectorGenerator *>(vectorGenerator);

		// point
		VectorGeneratorMatrixProxy<PointVectorGenerator> *matrixProxy = 
            new VectorGeneratorMatrixProxy<PointVectorGenerator>
            (
                pointVectorGenerator, 
                &PointVectorGenerator::position, 
                &PointVectorGenerator::position,
                scale
            );
		generalsDaughter->addProperty
        (
            new GenPositionProperty("origin", matrixProxy)
        );
		if (drawVectors)
        {
			newGizmo = 
                new VectorGizmo
                (
                    MODIFIER_SHIFT | MODIFIER_ALT, 
                    matrixProxy, 
                    wireColor, 
                    0.015f, 
                    originMatrixProxy, 
                    0.1f
                );
        }
		else
        {
        	newGizmo = 
                new PositionGizmo
                (
                    MODIFIER_SHIFT | MODIFIER_ALT, 
                    matrixProxy, 
                    NULL, 
                    0.1f
                );
        }

		if (initialPosition)
		{
			Matrix mat;
			matrixProxy->getMatrix(mat);
			mat[3] = *initialPosition;
			matrixProxy->setMatrix(mat);
		}

		matrixProxy->recordState();	// record initial state (otherwise will forget it on first edit)
		matrixProxy->commitState(true);
		returnMatrix = matrixProxy;
	}
	else if (vectorGenerator->nameID() == LineVectorGenerator::nameID_)
	{
		LineVectorGenerator *lineVectorGenerator = 
            static_cast<LineVectorGenerator *>(vectorGenerator);

		// start
		VectorGeneratorMatrixProxy<LineVectorGenerator> *matrixProxy1 = 
            new VectorGeneratorMatrixProxy<LineVectorGenerator>
            (
			    lineVectorGenerator, 
			    &LineVectorGenerator::start, 
			    &LineVectorGenerator::start,
                scale
            );
        generalsDaughter->addProperty
        (
            new GenPositionProperty("start", matrixProxy1)
        );

		// end
		VectorGeneratorMatrixProxy<LineVectorGenerator> *matrixProxy2 = 
            new VectorGeneratorMatrixProxy<LineVectorGenerator>
            (
			    lineVectorGenerator, 
			    &LineVectorGenerator::end, 
			    &LineVectorGenerator::end
            );

		generalsDaughter->addProperty
        (
            new GenPositionProperty("end", matrixProxy2)
        );

		newGizmo = 
            new LineGizmo
            (
                matrixProxy1, 
                matrixProxy2, 
                wireColor, 
                drawVectors, 
                originMatrixProxy
            );

		if (initialPosition)
		{
			Matrix startM;
			matrixProxy1->getMatrix(startM);
			startM[3] = *initialPosition;
			matrixProxy1->setMatrix(startM);

			Matrix endM;
			matrixProxy2->getMatrix(endM);
			endM[3] = *initialPosition + Vector3(0.1f, 0.f, 0.f);
			matrixProxy2->setMatrix(endM);
		}

		matrixProxy1->recordState();
		matrixProxy1->commitState(true);
		matrixProxy2->recordState();
		matrixProxy2->commitState(true);
		returnMatrix  = matrixProxy1;
		returnMatrix2 = matrixProxy2;
	}
	else if (vectorGenerator->nameID() == CylinderVectorGenerator::nameID_)
	{
		CylinderVectorGenerator *cylinderVectorGenerator = 
            static_cast<CylinderVectorGenerator *>(vectorGenerator);

		VectorGeneratorMatrixProxy<CylinderVectorGenerator> *matrixProxy1 = 
            new VectorGeneratorMatrixProxy<CylinderVectorGenerator>
            (
			    cylinderVectorGenerator, 
			    &CylinderVectorGenerator::origin, 
			    &CylinderVectorGenerator::updateOrigin,
                scale
            );
		generalsDaughter->addProperty
        (
            new GenPositionProperty("origin", matrixProxy1)
        );

		VectorGeneratorMatrixProxy<CylinderVectorGenerator> *matrixProxy2 = 
            new VectorGeneratorMatrixProxy<CylinderVectorGenerator>
            (
			    cylinderVectorGenerator, 
			    &CylinderVectorGenerator::destination, 
			    &CylinderVectorGenerator::destination,
                scale
            );
		generalsDaughter->addProperty
        (
            new GenPositionProperty("destination", matrixProxy2)
        );

		FloatProxyPtr radiusProxy = 
            new AccessorDataProxy<CylinderVectorGenerator, FloatProxy>
            (
				cylinderVectorGenerator, 
				&CylinderVectorGenerator::maxRadius, 
				&CylinderVectorGenerator::maxRadius
            );
		generalsDaughter->addProperty
        (
            new GenRadiusProperty("radius", radiusProxy, matrixProxy1)
        );

		newGizmo = 
            new CylinderGizmo
            (
                matrixProxy1, 
                matrixProxy2, 
                radiusProxy, 
                wireColor, 
                drawVectors, 
                originMatrixProxy, 
                info.radiusGizmoRadius_
            );

		if (initialPosition)
		{
			Matrix startM;
			matrixProxy1->getMatrix(startM);
			startM[3] = *initialPosition;
			matrixProxy1->setMatrix(startM);

			Matrix endM;
			matrixProxy2->getMatrix(endM);
			endM[3] = *initialPosition + Vector3(0.f, 0.1f, 0.f);
			matrixProxy2->setMatrix(endM);
		}

		// Bug 5075 fix: added a sensable maximum radius check of 365 km 
        // (maximum possible distance in an space).
		if (setDefaultRadius || (radiusProxy->get() > 365000.f)) 
		{
			radiusProxy->set(0.1f, false);
		}

		matrixProxy1->recordState();
		matrixProxy1->commitState(true);
		matrixProxy2->recordState();
		matrixProxy2->commitState(true);
		returnMatrix = matrixProxy1;
		returnMatrix2 = matrixProxy2;
	}
	else if (vectorGenerator->nameID() == SphereVectorGenerator::nameID_)
	{
		SphereVectorGenerator *sphereVectorGenerator = 
            static_cast<SphereVectorGenerator *>(vectorGenerator);

		// position
		VectorGeneratorMatrixProxy<SphereVectorGenerator> * matrixProxy = 
            new VectorGeneratorMatrixProxy<SphereVectorGenerator>
            (
			    sphereVectorGenerator, 
			    &SphereVectorGenerator::centre, 
			    &SphereVectorGenerator::centre,
                scale
            );
		generalsDaughter->addProperty
        (
            new GenPositionProperty("position", matrixProxy)
        );

		FloatProxyPtr radiusProxy1 = 
            new AccessorDataProxy<SphereVectorGenerator, FloatProxy>
            (
			    sphereVectorGenerator, 
			    &SphereVectorGenerator::minRadius, 
			    &SphereVectorGenerator::minRadius
            );
		generalsDaughter->addProperty
        (
            new GenRadiusProperty("minRadius", radiusProxy1, matrixProxy)
        );

		FloatProxyPtr radiusProxy2 = 
            new AccessorDataProxy<SphereVectorGenerator, FloatProxy>
            (
			    sphereVectorGenerator, 
			    &SphereVectorGenerator::maxRadius, 
			    &SphereVectorGenerator::maxRadius
            );
		generalsDaughter->addProperty
        (
            new GenRadiusProperty("maxRadius", radiusProxy2, matrixProxy)
        );

		newGizmo = 
            new SphereGizmo
            (
                matrixProxy, 
                radiusProxy1, 
                radiusProxy2, 
                wireColor, 
                drawVectors, 
                originMatrixProxy
            );

		if (initialPosition)
		{
			Matrix mat;
			matrixProxy->getMatrix(mat);
			mat[3] = *initialPosition;
			matrixProxy->setMatrix(mat);
		}

		// Bug 5075 fix: added a sensable maximum radius check of 365 km
        // (maximum possible distance in an space).
		if 
        (
            setDefaultRadius 
            || 
            (radiusProxy1->get() > 365000.f)
            || 
            (radiusProxy2->get() > 365000.f)
        ) 
		{
			radiusProxy1->set(0.0f, false);
			radiusProxy2->set(0.1f, false);
		}

		matrixProxy->recordState();
		matrixProxy->commitState(true);
		returnMatrix = matrixProxy;
	}
	else if (vectorGenerator->nameID() == BoxVectorGenerator::nameID_)
	{
		BoxVectorGenerator * boxVectorGenerator = 
            static_cast<BoxVectorGenerator *>(vectorGenerator);

		// corner
		VectorGeneratorMatrixProxy<BoxVectorGenerator> *matrixProxy1 = 
            new VectorGeneratorMatrixProxy<BoxVectorGenerator>
            (
			    boxVectorGenerator, 
			    &BoxVectorGenerator::corner, 
			    &BoxVectorGenerator::corner,
                scale
            );
		generalsDaughter->addProperty
        (
            new GenPositionProperty("corner", matrixProxy1)
        );

		// opposite corner
		VectorGeneratorMatrixProxy<BoxVectorGenerator> *matrixProxy2 =
            new VectorGeneratorMatrixProxy<BoxVectorGenerator>
            (
			    boxVectorGenerator, 
			    &BoxVectorGenerator::oppositeCorner, 
			    &BoxVectorGenerator::oppositeCorner,
                scale
            );
		generalsDaughter->addProperty
        (
            new GenPositionProperty("opposite corner", matrixProxy2)
        );

		newGizmo = 
            new BoxGizmo
            (
                matrixProxy1, 
                matrixProxy2, 
                wireColor, 
                drawVectors, 
                originMatrixProxy
            );

		if (initialPosition)
		{
			Matrix startM;
			matrixProxy1->getMatrix(startM);
			startM[3] = *initialPosition;
			matrixProxy1->setMatrix(startM);

			Matrix endM;
			matrixProxy2->getMatrix(endM);
			endM[3] = *initialPosition + Vector3(0.2f, 0.2f, 0.2f);
			matrixProxy2->setMatrix(endM);
		}

		matrixProxy1->recordState();
		matrixProxy1->commitState(true);
		matrixProxy2->recordState();
		matrixProxy2->commitState(true);
		returnMatrix = matrixProxy1;
		returnMatrix2 = matrixProxy2;
	}

	GeneralEditor::Editors newEditors;

	if (keepCurrentEditors)
	{
		GeneralEditor::Editors const &oldEds = GeneralEditor::currentEditors();
		for 
        (
            GeneralEditor::Editors::const_iterator it = oldEds.begin(); 
            it != oldEds.end(); 
            it++
        )
		{
			newEditors.push_back(*it);
		}
	}

	if (drawIt) // Bug 5105 fix: only draw gizmo if required.
	{
		GizmoManager::instance().addGizmo(newGizmo);
	}

	newEditors.push_back(generalsDaughter);
	GeneralEditor::currentEditors(newEditors);

	VectorGeneratorGizmoProperties properties;
	properties.gizmo_ = newGizmo;
	properties.referenceMatrix_ = returnMatrix;
	properties.referenceMatrix2_ = returnMatrix2;
	return properties;
}
BW_END_NAMESPACE

