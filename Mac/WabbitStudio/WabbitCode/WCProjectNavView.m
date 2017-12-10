//
//  WCProjectNavView.m
//  WabbitStudio
//
//  Created by William Towe on 4/9/11.
//  Copyright 2011 Revolution Software. All rights reserved.
//

#import "WCProjectNavView.h"
#import "WCDefines.h"
#import "WCProject.h"
#import "NSGradient+WCExtensions.h"
#import "WCBreakpoint.h"
#import "WCGeneralPerformer.h"


@implementation WCProjectNavView

+ (Class)cellClass {
	return [NSActionCell class];
}

- (id)initWithFrame:(NSRect)frameRect {
	if (!(self = [super initWithFrame:frameRect]))
		return nil;
	
	_images = [[NSArray alloc] initWithObjects:[NSImage imageNamed:@"Group16x16"],[NSImage imageNamed:@"Building16x16"],[NSImage imageNamed:@"Breakpoints16x16"],[NSImage imageNamed:@"Search16x16"],[NSImage imageNamed:@"Symbols16x16"], nil];
	_selectors = [[NSArray alloc] initWithObjects:NSStringFromSelector(@selector(viewProject:)),NSStringFromSelector(@selector(viewBuildMessages:)),NSStringFromSelector(@selector(viewBreakpoints:)),NSStringFromSelector(@selector(viewSearch:)),NSStringFromSelector(@selector(viewSymbols:)), nil];
	_tooltips = [[NSArray alloc] initWithObjects:NSLocalizedString(@"Show the Files view", @"Show the Files view"),NSLocalizedString(@"Show the Build Messages view", @"Show the Build Messages view"),NSLocalizedString(@"Show the Breakpoints view", @"Show the Breakpoints view"),NSLocalizedString(@"Show the Search view", @"Show the Search view"),NSLocalizedString(@"Show the Symbols view", @"Show the Symbols view"), nil];
	
	return self;
}

- (void)dealloc {
	[_images release];
	[_selectors release];
	[_tooltips release];
    [super dealloc];
}

- (void)resetCursorRects {
	[super resetCursorRects];
	[self removeAllToolTips];
	
	NSRect bounds = [self bounds];
	CGFloat startX = floor(NSWidth(bounds)/2.0) - floor(([[[self images] lastObject] size].width * 2 * [[self images] count])/2.0);
	
	for (uint8_t index = 0; index < [[self images] count]; index++) {
		NSImage *image = [[self images] objectAtIndex:index];
		//[image setSize:WCSmallSize];
		NSSize size = [image size];
		NSRect frame = NSMakeRect(startX + (size.width * 2 * index), bounds.origin.y, size.width * 2, NSHeight(bounds));
		
		[self addToolTipRect:frame owner:self userData:[_tooltips objectAtIndex:index]];
	}
}

- (NSString *)view:(NSView *)view stringForToolTip:(NSToolTipTag)tag point:(NSPoint)point userData:(void *)data {
	return (NSString *)data;
}

- (void)mouseDown:(NSEvent *)theEvent {
	
	NSRect bounds = [self bounds];
	NSRectArray rects = calloc([[self images] count], sizeof(NSRect));
	CGFloat startX = floor(NSWidth(bounds)/2.0) - floor(([[[self images] lastObject] size].width * 2 * [[self images] count])/2.0);
	NSUInteger index;
	
	for (index = 0; index < [[self images] count]; index++) {
		NSRect frame = NSMakeRect(startX + (32.0 * index), bounds.origin.y, 32.0, NSHeight(bounds));
		
		rects[index] = frame;
	}
	
	NSPoint point = [self convertPoint:[theEvent locationInWindow] fromView:nil];
	for (index = 0; index < [[self images] count]; index++) {
		if (NSPointInRect(point, rects[index])) {
			[self setSelectedIndex:index];
			break;
		}
	}
	
	// create a pool to flush each time through the cycle
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	// track!
	NSEvent *event = nil;
	while([event type] != NSLeftMouseUp) {
		[pool drain];
		pool = [[NSAutoreleasePool alloc] init];
		
		event = [[self window] nextEventMatchingMask: NSLeftMouseDraggedMask | NSLeftMouseUpMask];

		NSPoint p = [self convertPoint:[event locationInWindow] fromView:nil];

		for (index = 0; index < [[self images] count]; index++) {
			if (NSPointInRect(p, rects[index])) {
				[self setSelectedIndex:index];
				break;
			}
		}
	}
	
	[self sendAction:[self action] to:[self target]];

	[pool drain];
	free(rects);
}

- (void)drawRect:(NSRect)dirtyRect {
	NSRect bounds = [self bounds];
	
	[[NSGradient unifiedNormalGradient] drawInRect:bounds angle:90.0];
	CGFloat startX = floor(NSWidth(bounds)/2.0) - floor(([[[self images] objectAtIndex:0] size].width * 2 * [[self images] count])/2.0);
	NSUInteger index;
	
	for (index = 0; index < [[self images] count]; index++) {
		NSImage *image = [[self images] objectAtIndex:index];
		//[image setSize:WCSmallSize];
		NSSize size = [image size];
		NSRect frame = NSMakeRect(startX + (32.0 * index), bounds.origin.y, 32.0, NSHeight(bounds));
		
		if (index == [self selectedIndex]) {
			[[NSGradient unifiedSelectedGradient] drawInRect:frame angle:90.0];
			[[NSColor colorWithCalibratedWhite:0.66 alpha:1.0] setFill];
			NSRectFill(NSMakeRect(frame.origin.x, frame.origin.y, 1.0, NSHeight(frame)));
			NSRectFill(NSMakeRect(frame.origin.x+NSWidth(frame), frame.origin.y, 1.0, NSHeight(frame)));
		}
		
		[image drawInRect:WCCenteredRect(NSMakeRect(0.0, 0.0, size.width, size.height), frame) fromRect:NSZeroRect operation:NSCompositeSourceOver fraction:1.0 respectFlipped:YES hints:nil];
	}
	
	[[NSColor colorWithCalibratedWhite:0.6 alpha:1.0] setFill];
	NSRectFill(NSMakeRect(0.0, 0.0, NSWidth(bounds), 1.0));
}

@synthesize images=_images;
@synthesize selectors=_selectors;
@dynamic selectedIndex;
- (NSUInteger)selectedIndex {
	return _selectedIndex;
}
- (void)setSelectedIndex:(NSUInteger)selectedIndex {
	if (_selectedIndex == selectedIndex)
		return;
	
	_selectedIndex = selectedIndex;
	
	[self setNeedsDisplay:YES];
}
@end
