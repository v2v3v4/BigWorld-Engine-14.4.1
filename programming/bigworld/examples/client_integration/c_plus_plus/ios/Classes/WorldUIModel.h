#import <Foundation/Foundation.h>


@interface WorldUIModel : NSObject
{
	float entityScale_;
    BOOL followPlayer_;
    CGAffineTransform transform_;
}

@property (nonatomic) float entityScale;
@property (nonatomic) BOOL followPlayer;
@property (nonatomic) CGAffineTransform transform;

@property (nonatomic, readonly) CGPoint position;
@property (nonatomic, readonly) float scale;
@property (nonatomic, readonly) float rotation;

@end
