//
//  WCMultiLineTextFieldCell.m
//  WabbitStudio
//
//  Created by William Towe on 4/23/11.
//  Copyright 2011 Revolution Software. All rights reserved.
//

#import "RSMultiLineTextFieldCell.h"
#import "WCDefines.h"
#import "NS(Attributed)String+Geometrics.h"
#import "WCInfoTextFieldCell.h"


@interface RSMultiLineTextFieldCell ()
- (NSAttributedString *)_attributedStringForSecondaryTitle;
@end

@implementation RSMultiLineTextFieldCell

- (void)dealloc {
	[_secondaryTextFieldCell release];
    [super dealloc];
}

- (id)copyWithZone:(NSZone *)zone {
	RSMultiLineTextFieldCell *copy = [super copyWithZone:zone];
	
	copy->_secondaryTextFieldCell = [_secondaryTextFieldCell copyWithZone:zone];
	
	return copy;
}

- (void)setControlView:(NSView *)view {
	[super setControlView:view];
	[_secondaryTextFieldCell setControlView:view];
}

- (void)setHighlighted:(BOOL)flag { 
	[super setHighlighted:flag];
	[_secondaryTextFieldCell setHighlighted:flag];
}

- (void)drawInteriorWithFrame:(NSRect)cellFrame inView:(NSView *)controlView {
	if ([[self secondaryTitle] length] > 0) {
		NSRect stRect = [self secondaryTitleRectForBounds:cellFrame remainingRect:&cellFrame];
		
		[_secondaryTextFieldCell drawWithFrame:stRect inView:controlView];
	}
	
	[super drawInteriorWithFrame:cellFrame inView:controlView];
}

- (void)editWithFrame:(NSRect)aRect inView:(NSView *)controlView editor:(NSText *)textObj delegate:(id)anObject event:(NSEvent *)theEvent {
	[self secondaryTitleRectForBounds:aRect remainingRect:&aRect];
	[super editWithFrame:aRect inView:controlView editor:textObj delegate:anObject event:theEvent];
}

- (void)selectWithFrame:(NSRect)aRect inView:(NSView *)controlView editor:(NSText *)textObj delegate:(id)anObject start:(NSInteger)selStart length:(NSInteger)selLength {
	[self secondaryTitleRectForBounds:aRect remainingRect:&aRect];
	[super selectWithFrame:aRect inView:controlView editor:textObj delegate:anObject start:selStart length:selLength];
}

- (NSRect)expansionFrameWithFrame:(NSRect)cellFrame inView:(NSView *)view {
	if ([[self secondaryTitle] length] == 0)
		return [super expansionFrameWithFrame:cellFrame inView:view];
	
	NSSize mSize = [[self attributedStringValue] sizeForWidth:FLT_MAX height:NSHeight(cellFrame)];
	NSRect mRect = NSMakeRect(NSMinX(cellFrame), NSMinY(cellFrame), mSize.width, mSize.height);
	NSSize sSize = [[_secondaryTextFieldCell attributedStringValue] sizeForWidth:FLT_MAX height:NSHeight(cellFrame)];
	NSRect sRect = NSMakeRect(NSMinX(cellFrame), NSMinY(cellFrame)+NSHeight(mRect), sSize.width, sSize.height);
	
	return NSUnionRect(mRect, sRect);
}

- (NSUInteger)hitTestForEvent:(NSEvent *)event inRect:(NSRect)cellFrame ofView:(NSView *)controlView {
	[self secondaryTitleRectForBounds:cellFrame remainingRect:&cellFrame];
	return [super hitTestForEvent:event inRect:cellFrame ofView:controlView];
}

- (NSRect)secondaryTitleRectForBounds:(NSRect)bounds remainingRect:(NSRectPointer)remainingRect; {
	if ([[self secondaryTitle] length] == 0)
		return NSZeroRect;
	
	NSRect top, bottom;
	NSDivideRect(bounds, &top, &bottom, floor(NSHeight(bounds)/2.0), NSMinYEdge);
	
	if (remainingRect != NULL)
		*remainingRect = top;
	
	return bottom;
}
		 
@dynamic secondaryTitle;
- (NSString *)secondaryTitle {
	return [_secondaryTextFieldCell stringValue];
}
- (void)setSecondaryTitle:(NSString *)secondaryTitle {
	if (_secondaryTextFieldCell == nil) {
		_secondaryTextFieldCell = [[WCInfoTextFieldCell alloc] init];
		[_secondaryTextFieldCell setControlView:[self controlView]];
		[_secondaryTextFieldCell setControlSize:[self controlSize]];
		[_secondaryTextFieldCell setFont:[self font]];
	}
	if (secondaryTitle == nil)
		[_secondaryTextFieldCell setStringValue:@""];
	else
		[_secondaryTextFieldCell setStringValue:secondaryTitle];
}

- (NSAttributedString *)_attributedStringForSecondaryTitle {
	NSMutableParagraphStyle *style = [[[NSParagraphStyle defaultParagraphStyle] mutableCopy] autorelease];
	[style setLineBreakMode:NSLineBreakByTruncatingTail];
	NSDictionary *attributes = [NSDictionary dictionaryWithObjectsAndKeys:[self font],NSFontAttributeName,([self isHighlighted])?[NSColor alternateSelectedControlTextColor]:[NSColor disabledControlTextColor],NSForegroundColorAttributeName,style,NSParagraphStyleAttributeName, nil];
	NSAttributedString *attributedString = [[[NSAttributedString alloc] initWithString:[self secondaryTitle] attributes:attributes] autorelease];
	
	return attributedString;
}

@end
