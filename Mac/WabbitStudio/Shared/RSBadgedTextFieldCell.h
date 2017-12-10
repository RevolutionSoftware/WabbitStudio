//
//  WCBadgedTextFieldCell.h
//  WabbitStudio
//
//  Created by William Towe on 4/23/11.
//  Copyright 2011 Revolution Software. All rights reserved.
//

#import "RSIconTextFieldCell.h"


@interface RSBadgedTextFieldCell : RSIconTextFieldCell <NSCopying> {
@private
    NSUInteger _badgeCount;
}
@property (assign,nonatomic) NSUInteger badgeCount;
@property (readonly,nonatomic) NSColor *badgeFillColor;
@property (readonly,nonatomic) NSColor *badgeTextColor;

- (NSRect)badgeRectForBounds:(NSRect)bounds remainingRect:(NSRectPointer)remainingRect;
@end
