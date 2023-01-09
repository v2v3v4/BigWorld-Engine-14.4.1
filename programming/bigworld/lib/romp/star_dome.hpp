#ifndef StarDome_HPP
#define StarDome_HPP

#include <iostream>
#include "moo/Visual.hpp"
#include "moo/material.hpp"
#include "moo/managed_texture.hpp"


BW_BEGIN_NAMESPACE

class StarDome
{
public:
	StarDome();
	~StarDome();

	void		init( void );
	void		draw( float timeOfDay );
private:

	StarDome(const StarDome&);
	StarDome& operator=(const StarDome&);

	Moo::VisualPtr					visual_;
	Moo::Material					mat_;
	Moo::BaseTexturePtr				texture_;

	friend std::ostream& operator<<(std::ostream&, const StarDome&);
};

#ifdef CODE_INLINE
#include "star_dome.ipp"
#endif

BW_END_NAMESPACE


#endif
/*StarDome.hpp*/
