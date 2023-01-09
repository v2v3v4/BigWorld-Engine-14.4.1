#include "pch.hpp"

#include "current_general_properties.hpp"

BW_BEGIN_NAMESPACE

BW::vector<GenPositionProperty*> PropertyCollator<GenPositionProperty>::properties_;
CurrentPositionProperties::ViewEnroller CurrentPositionProperties::s_viewEnroller;

BW::vector<GenScaleProperty*> PropertyCollator<GenScaleProperty>::properties_;
CurrentScaleProperties::ViewEnroller CurrentScaleProperties::s_viewEnroller;

BW::vector<GenRotationProperty*> PropertyCollator<GenRotationProperty>::properties_;
CurrentRotationProperties::ViewEnroller CurrentRotationProperties::s_viewEnroller;


Vector3 CurrentPositionProperties::averageOrigin()
{
	BW_GUARD;

	Vector3 avg = Vector3::zero();

	if (properties_.empty())
		return Vector3::zero();

	BW::vector<GenPositionProperty*>::iterator i = properties_.begin();
	for (; i != properties_.end(); ++i)
	{
		Matrix m;
		(*i)->pMatrix()->getMatrix( m );
		avg += m.applyToOrigin();
	}

	return avg / (float) properties_.size();
}



Vector3 CurrentRotationProperties::averageOrigin()
{
	BW_GUARD;

	Vector3 avg = Vector3::zero();

	if (properties_.empty())
		return Vector3::zero();

	BW::vector<GenRotationProperty*>::iterator i = properties_.begin();
	for (; i != properties_.end(); ++i)
	{
		Matrix m;
		(*i)->pMatrix()->getMatrix( m );
		avg += m.applyToOrigin();
	}

	return avg / (float) properties_.size();
}

Vector3 CurrentScaleProperties::averageOrigin()
{
	BW_GUARD;

	if (!properties_.empty())
	{
		Vector3 avg = Vector3::zero();

		BW::vector<GenScaleProperty*>::iterator i = properties_.begin();
		for (; i != properties_.end(); ++i)
		{
			Matrix m;
			(*i)->pMatrix()->getMatrix( m );
			avg += m.applyToOrigin();
		}

		return avg / (float) properties_.size();
	}

	return Vector3::zero();
}
BW_END_NAMESPACE

