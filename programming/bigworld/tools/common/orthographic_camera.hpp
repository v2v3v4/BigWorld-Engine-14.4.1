#ifndef ORTHOGRAPHIC_CAMERA_HPP
#define ORTHOGRAPHIC_CAMERA_HPP

#include <iostream>
#include "base_camera.hpp"

BW_BEGIN_NAMESPACE

/**
 * This class implements an orthographic camera that
 * can be driven around using the keyboard and mouse
 */
class OrthographicCamera : public BaseCamera
{
public:
	OrthographicCamera();
	~OrthographicCamera();

	void update( float dTime, bool activeInputHandler = true );
	bool handleKeyEvent( const KeyEvent & );
	bool handleMouseEvent( const MouseEvent & );

    void view( const Matrix & );
	void view( const OrthographicCamera& other );

private:
	void	handleInput( float dTime );
    void 	viewToPolar();
    void 	polarToView();

	OrthographicCamera(const OrthographicCamera&);
	OrthographicCamera& operator=(const OrthographicCamera&);

	float		up_;
	float		right_;
	typedef BW::map< KeyCode::Key, bool>	KeyDownMap;
	KeyDownMap	keyDown_;

	//For hiding and showing the mouse cursor
	bool isCursorHidden_;

	// Where the cursor was when we started looking around
	POINT lastCursorPosition_;

	friend std::ostream& operator<<(std::ostream&, const OrthographicCamera&);
};

BW_END_NAMESPACE

#endif  // ORTHOGRAPHIC_CAMERA_HPP
