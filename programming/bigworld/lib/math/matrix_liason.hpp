#ifndef MATRIX_LIASON_HPP
#define MATRIX_LIASON_HPP

#include "matrix.hpp"

BW_BEGIN_NAMESPACE

/**
 *	Interface class for top level attachments
 */
class MatrixLiaison
{
public:
	virtual ~MatrixLiaison() {}
	virtual const Matrix & getMatrix() const = 0;
	virtual bool setMatrix( const Matrix & m ) = 0;
};

BW_END_NAMESPACE

#endif //MATRIX_LIASON_HPP
