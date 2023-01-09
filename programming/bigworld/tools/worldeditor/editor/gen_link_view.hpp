#ifndef GEN_LINK_VIEW_HPP
#define GEN_LINK_VIEW_HPP

#include "common/editor_views.hpp"
#include "worldeditor/editor/link_property.hpp"

BW_BEGIN_NAMESPACE

class StringPropertyItem;

class GenLinkView : public BaseView
{
public:
	GenLinkView( LinkProperty & property );
	StringPropertyItem* item();
	
	virtual void elect();

	virtual void onSelect();

	virtual void onChange(bool transient, bool addBarrier = true) {}
	virtual void cloneValue( BaseView* another ) {}

	virtual bool updateGUI();

	static LinkProperty::View * create(LinkProperty & property );

	virtual PropertyManagerPtr getPropertyManager() const;

private:
	LinkProperty & property_;
	CString oldValue_;
	class Enroller {
		Enroller() {
			BW_GUARD;

			LinkProperty_registerViewFactory(
				GeneralProperty::nextViewKindID(), &create );
		}
		static Enroller s_instance;
	};
};

BW_END_NAMESPACE

#endif // GEN_LINK_VIEW_HPP
