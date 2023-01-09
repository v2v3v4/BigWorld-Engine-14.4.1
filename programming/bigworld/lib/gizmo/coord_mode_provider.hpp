#ifndef COORD_MODE_PROVIDER_HPP
#define COORD_MODE_PROVIDER_HPP

BW_BEGIN_NAMESPACE

class CoordModeProvider
{
public:
	enum CoordMode
	{
		COORDMODE_WORLD = 0,
		COORDMODE_OBJECT,
		COORDMODE_VIEW
	};
	virtual CoordMode getCoordMode() const
	{
		return COORDMODE_WORLD;
	}

	static CoordModeProvider* ins( CoordModeProvider* instance = NULL )
	{
		if (instance == NULL)
		{
			if (s_instance_ == NULL)
			{
				s_internal_ = true;
				s_instance_ = new CoordModeProvider();
			}
		}
		else
		{
			if (s_instance_ && s_internal_)
				bw_safe_delete(s_instance_);
			s_instance_ = instance;
			s_internal_ = false;
		}
		return s_instance_;
	}

	static void fini()
	{
		if (s_internal_)
			bw_safe_delete(s_instance_);

		s_instance_ = NULL;
	}
private:
	static CoordModeProvider*	s_instance_;
	static bool					s_internal_;
};

BW_END_NAMESPACE
#endif	// COORD_MODE_PROVIDER_HPP
