#ifndef LABELS_HPP
#define LABELS_HPP

#include "romp/font.hpp"
#include "romp/font_metrics.hpp"
#include "romp/font_manager.hpp"
#include "moo/draw_context.hpp"


BW_BEGIN_NAMESPACE

/** This class allows labels to easily be drawn on the screen.
  * Just add the labels and their world positions to the list.
  * then call Moo::SortedChannel::addDrawItem( ChannelDrawItem labels )
  * and it will take care of the rest.
  */

class Labels : public Moo::DrawContext::UserDrawItem
{
public:
	struct Label
	{
		Label( const BW::string& t, const Vector3& p )
			: text_(t), position_(p)
		{}

		BW::string text_;
		Vector3 position_;
	};

public:
	Labels()
	{
	}

	virtual void draw()
	{
		Matrix viewProj = Moo::rc().view();
		viewProj.postMultiply( Moo::rc().projection() );

		FontPtr font = FontManager::instance().get( "system_small.font" );
		if ( font && FontManager::instance().begin( *font ) )
		{
			LabelVector::const_iterator it  = this->labels_.begin();
			LabelVector::const_iterator end = this->labels_.end();
			while (it != end)
			{
				Vector3 projectedPos = viewProj.applyPoint( it->position_ );
				projectedPos /= abs( projectedPos.z );
				if (projectedPos.z >= 0.0f)
				{
					font->drawStringInClip( it->text_, projectedPos );
				}

				++it;
			}
			FontManager::instance().end();
		}
	}

	virtual void fini()
	{
		delete this;
	}

	void add( const BW::string & id, const Vector3 & position )
	{
		this->labels_.push_back( Label(id, position) );
	}

	typedef BW::vector<Label> LabelVector;
	LabelVector labels_;
};

/** This class allows set of labels to easily be drawn on the screen.
  * This class differs to Labels in that it stacks all the labels on
  *	top of each other vertically rooted at a single position.
  */
class StackedLabels : public Moo::DrawContext::UserDrawItem
{
public:
	StackedLabels( const Vector3& position )
		:	position_(position)
	{
	}

	virtual void draw()
	{
		Matrix viewProj = Moo::rc().view();
		viewProj.postMultiply( Moo::rc().projection() );
		Vector3 projectedPos = viewProj.applyPoint( position_ );
		if ( projectedPos.z <= 1.f )
		{
			FontPtr font = FontManager::instance().get( "system_small.font" );
			if (FontManager::instance().begin( *font ))
			{
				float clipHeight = font->metrics().clipHeight();
				Vector3 pos = position_;

				StrVector::const_iterator it  = this->labels_.begin();
				StrVector::const_iterator end = this->labels_.end();
				while (it != end)
				{
					float widthInClip = font->metrics().stringWidth( *it ) / float(Moo::rc().halfScreenWidth());

					projectedPos.y += clipHeight;
					font->drawStringInClip( *it, projectedPos-Vector3(widthInClip/2.f,0,0) );
					++it;
				}
				FontManager::instance().end();
			}
		}
	}

	virtual void fini()
	{
		delete this;
	}

	void add( const BW::string & id )
	{
		this->labels_.push_back( id );
	}

	typedef BW::vector<BW::string> StrVector;
	StrVector labels_;
	Vector3 position_;
};

BW_END_NAMESPACE

#endif // LABELS_HPP
