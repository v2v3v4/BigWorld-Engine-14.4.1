#ifndef CURRENT_GENERAL_PROPERTIES_HPP
#define CURRENT_GENERAL_PROPERTIES_HPP

#include "general_editor.hpp"
#include "general_properties.hpp"

BW_BEGIN_NAMESPACE

template<class PROPERTY>
class PropertyCollator : public GeneralProperty::View
{
public:
	PropertyCollator( PROPERTY& prop ) : prop_( prop )
	{
	}

	virtual void elect()
	{
		BW_GUARD;

		properties_.push_back( &prop_ );
	}

	virtual void expel()
	{
		BW_GUARD;

		properties_.erase( std::find( properties_.begin(), properties_.end(), &prop_ ) );
	}

	virtual void select()
	{
	}

	static BW::vector<PROPERTY*> properties()
	{
		return properties_;
	}

private:
	PROPERTY&						prop_;
protected:
	static BW::vector<PROPERTY*>	properties_;
};


class CurrentPositionProperties : public PropertyCollator<GenPositionProperty>
{
public:
	CurrentPositionProperties( GenPositionProperty& prop ) : PropertyCollator<GenPositionProperty>( prop )
	{
	}

	static GeneralProperty::View * create( GenPositionProperty & prop )
	{
		BW_GUARD;

		return new CurrentPositionProperties( prop );
	}

	/** Get the average origin of all the selected properties */
	static Vector3 averageOrigin();


private:
	static struct ViewEnroller
	{
		ViewEnroller()
		{
			BW_GUARD;

			GenPositionProperty_registerViewFactory(
				GeneralProperty::nextViewKindID(), &create );
		}
	}	s_viewEnroller;
};


/**
 *	A view into the current rotation property of the current editor
 *	Allows WheelRotator and MatrixRotator to rotate the correct thing
 */
class CurrentRotationProperties : public PropertyCollator<GenRotationProperty> //: public GeneralProperty::View
{
public:
	CurrentRotationProperties( GenRotationProperty& prop ) : PropertyCollator<GenRotationProperty>( prop )
	{
	}

	/** Get the average origin of all the selected properties */
	static Vector3 averageOrigin();

	static GeneralProperty::View * create( GenRotationProperty & prop )
	{
		BW_GUARD;

		return new CurrentRotationProperties( prop );
	}

private:

	static struct ViewEnroller
	{
		ViewEnroller()
		{
			BW_GUARD;

			GenRotationProperty_registerViewFactory(
				GeneralProperty::nextViewKindID(), &create );
		}
	}	s_viewEnroller;
};



class CurrentScaleProperties : public PropertyCollator<GenScaleProperty>
{
public:
	CurrentScaleProperties( GenScaleProperty& prop ) : PropertyCollator<GenScaleProperty>( prop )
	{
	}

	/** Get the average origin of all the selected properties */
	static Vector3 averageOrigin();

	static GeneralProperty::View * create( GenScaleProperty& prop )
	{
		BW_GUARD;

		return new CurrentScaleProperties( prop );
	}

private:
	static struct ViewEnroller
	{
		ViewEnroller()
		{
			BW_GUARD;

			GenScaleProperty_registerViewFactory(
				GeneralProperty::nextViewKindID(), &create );
		}
	}	s_viewEnroller;
};


BW_END_NAMESPACE
#endif // CURRENT_GENERAL_PROPERTIES_HPP
