@protocol TouchHandler;

@protocol TouchHandlerOwner

- (void) setTouchHandler: (id< TouchHandler >) newTouchHandler;

@end

@protocol TouchHandler

- (BOOL) touchBegan: (UITouch *) touch 
		  withEvent: (UIEvent *) event 
		   andOwner: (id <TouchHandlerOwner>) owner;

- (void) touchMoved: (UITouch *) touch 
		  withEvent: (UIEvent *) event 
		   andOwner: (id <TouchHandlerOwner>) owner;

- (void) touchEnded: (UITouch *) touch 
		  withEvent: (UIEvent *) event 
		   andOwner: (id <TouchHandlerOwner>) owner;

@end


