//
//  RSLCDView.m
//  WabbitEmu Beta
//
//  Created by William Towe on 4/24/11.
//  Copyright 2011 Revolution Software. All rights reserved.
//

#import "RSLCDView.h"
#import "WETransferSheetController.h"
#import "RSCalculator.h"
#import "RSScreenCaptureIndicatorBar.h"
#import "RSCalculatorSkinView.h"
#include "gif.h"

#import <OpenGL/OpenGL.h>


NSString *const kLCDUseWirePatternKey = @"LCDUseWirePattern";

@interface RSLCDView ()
@property (copy,nonatomic) NSArray *currentFilePaths;
@property (readonly,nonatomic) NSBitmapImageRep *LCDBitmap;

- (void)_privateInit;
@end

@implementation RSLCDView

- (id)initWithCoder:(NSCoder *)coder {
	if (!(self = [super initWithCoder:coder]))
		return nil;
	
	[self commonInit];
	
	return self;
}

- (void)dealloc {
	[[NSNotificationCenter defaultCenter] removeObserver:self];
	[_calculator release];
	[_currentFilePaths release];
	glDeleteTextures(2, _textures);
    [super dealloc];
}

- (BOOL)isOpaque {
	return YES;
}

- (BOOL)acceptsFirstResponder {
	return YES;
}

- (void)mouseDragged:(NSEvent *)event {
	NSString *directoryPath = NSTemporaryDirectory();
	NSString *fileName = NSLocalizedString(@"screenshot", @"screenshot");
	//NSString *fileName = [[NSUserDefaults standardUserDefaults] stringForKey:kWEPrefsScreenShotsFileNameKey];
	NSBitmapImageFileType fileType = NSPNGFileType;
	//NSBitmapImageFileType fileType = [[NSUserDefaults standardUserDefaults] unsignedIntegerForKey:kWEPrefsScreenShotsFormatKey];
	NSString *fileExtension = @"png";
	//NSString *fileExtension = [[WEBasicPerformer sharedInstance] fileExtensionForBitmapImageFileType:fileType];
	NSString *filePath = [directoryPath stringByAppendingPathComponent:[fileName stringByAppendingPathExtension:fileExtension]];
	
	// grab our bitmap rep
	NSBitmapImageRep *bitmap = [self LCDBitmap];
	// get it's data
	NSData *bitmapData = [bitmap representationUsingType:fileType properties:nil];
	// get the dragging pboard
	NSPasteboard *pboard = [NSPasteboard pasteboardWithName:NSDragPboard];
	// get the point for this event, and the drag point which is centered on the image
	NSPoint dragPoint, location = [self convertPointFromBase:[event locationInWindow]];
	dragPoint.x = location.x - [bitmap size].width/2;
	dragPoint.y = location.y - [bitmap size].height/2;
	// the image that will be dragged
	NSImage *dragImage = [[[NSImage alloc] initWithSize:[bitmap size]] autorelease];
	// temp image used to draw into the drag image
	NSImage *tempImage = [[[NSImage alloc] initWithSize:[bitmap size]] autorelease];
	[tempImage addRepresentation:bitmap];
	[dragImage lockFocus];
	// draw with partial transparency
	[tempImage compositeToPoint:NSZeroPoint operation:NSCompositeCopy fraction:0.85];
	[dragImage unlockFocus];
	
	// write our temporary file to disk
	if (![bitmapData writeToFile:filePath options:NSAtomicWrite error:NULL])
		return;
	
	// declare the pboard types, adding our additional type
	[pboard declareTypes:[NSArray arrayWithObjects:NSFilenamesPboardType, nil] owner:nil];
	// pass the path to the temporary file we wrote to the pboard
	[pboard setPropertyList:[NSArray arrayWithObjects:filePath,nil] forType:NSFilenamesPboardType];
	
	// initiate the drag
	[self dragImage:dragImage at:dragPoint offset:NSZeroSize event:event pasteboard:pboard source:self slideBack:YES];
}

- (void)keyDown:(NSEvent *)fEvent {
	if ([self calculator] == nil || ![[self calculator] isRunning])
		return;
	
	CPU_t *cpu = &[[self calculator] calc]->cpu;
	NSMutableIndexSet *keyCodesForKeysPressed = [NSMutableIndexSet indexSet];
	
	[keyCodesForKeysPressed addIndex:[fEvent keyCode]];
	
	keypad_key_press(cpu, [fEvent keyCode]);
	
	// create a pool to flush each time through the cycle
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	// track!
	NSEvent *event = nil;
	while([keyCodesForKeysPressed count] != 0) {
		[pool drain];
		pool = [[NSAutoreleasePool alloc] init];
	
		event = [[self window] nextEventMatchingMask:NSKeyDownMask|NSKeyUpMask];
		
		if ([event type] == NSKeyDown && ![keyCodesForKeysPressed containsIndex:[event keyCode]]) {
			[keyCodesForKeysPressed addIndex:[event keyCode]];
			keypad_key_press(cpu, [event keyCode]);
		}
		else if ([event type] == NSKeyUp && [keyCodesForKeysPressed containsIndex:[event keyCode]]) {
			[keyCodesForKeysPressed removeIndex:[event keyCode]];
			keypad_key_release(cpu, [event keyCode]);
		}
	}
	[pool drain];
}

- (IBAction)copy:(id)sender {
	if ([self calculator] == nil || ![[self calculator] isActive] || ![[self calculator] isRunning]) {
		NSBeep();
		return;
	}
	
	TCHAR *ans = GetRealAns(&[[self calculator] calc]->cpu);
	if (ans != NULL) {
		NSPasteboard *pboard = [NSPasteboard pasteboardWithName:NSGeneralPboard];
		
		[pboard declareTypes:[NSArray arrayWithObjects:NSStringPboardType, nil] owner:nil];
		[pboard setString:[NSString stringWithCString:ans encoding:NSASCIIStringEncoding] forType:NSStringPboardType];
		
		free(ans);
	}
}

- (void)drawRect:(NSRect)dirtyRect
{	
	if ([self calculator] == nil || ![[self calculator] isActive] || ![[self calculator] isRunning])
		return;
	
	uint8_t *lcd = LCD_image([[self calculator] calc]->cpu.pio.lcd);
	uint16_t row, col;
	
	glClear(GL_COLOR_BUFFER_BIT);
	
	if ([[NSUserDefaults standardUserDefaults] boolForKey:kLCDUseWirePatternKey])
		glEnable(GL_BLEND);
	 
	if ([self isWidescreen]) {
		for (row=0; row<kLCDHeight; row++) {
			for (col=0; col<kLCDWidescreenWidth; col++) {
				uint8_t val = 255-lcd[row*kLCDWidescreenWidth+col];
				
				_wbuffer[row][col][2] = (0x9E*val)/255;
				_wbuffer[row][col][1] = (0xAB*val)/255;
				_wbuffer[row][col][0] = (0x88*val)/255;
				
			}
		}
	}
	else {
		for (row=0; row<kLCDHeight; row++) {
			for (col=0; col<kLCDWidth; col++) {
				u_char val = 255-lcd[(row)*kLCDWidescreenWidth+(col)];
				
				_buffer[row][col][2] = (0x9E*val)/255;
				_buffer[row][col][1] = (0xAB*val)/255;
				_buffer[row][col][0] = (0x88*val)/255;
				
			}
		}
	}
	
	CGFloat width = ([self isWidescreen])?kLCDWidescreenWidth:kLCDWidth, height = kLCDHeight;
	
	glBindTexture(GL_TEXTURE_RECTANGLE_EXT, _textures[0]);
	glTexParameterf(GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_STORAGE_HINT_APPLE, GL_STORAGE_SHARED_APPLE);
	glTexParameterf(GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glPixelStoref(GL_UNPACK_CLIENT_STORAGE_APPLE, GL_TRUE);
	
	if ([self isWidescreen])
		glTexImage2D(GL_TEXTURE_RECTANGLE_EXT, 0, GL_RGBA, width, height, 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, _wbuffer);
	else
		glTexImage2D(GL_TEXTURE_RECTANGLE_EXT, 0, GL_RGBA, width, height, 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, _buffer);
		
	glBegin(GL_QUADS);
	glTexCoord2f(0.0f, 0.0f);
	glVertex2f(-1.0f, 1.0f);
	glTexCoord2f(0.0f, height );
	glVertex2f(-1.0f, -1.0f);
	glTexCoord2f(width, height );
	glVertex2f(1.0f, -1.0f);
	glTexCoord2f(width, 0.0f );
	glVertex2f(1.0f, 1.0f);
	glEnd();
	
	if ([[NSUserDefaults standardUserDefaults] boolForKey:kLCDUseWirePatternKey]) {		
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, _textures[1]);
		glTexParameterf(GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_STORAGE_HINT_APPLE, GL_STORAGE_CACHED_APPLE);
		glTexParameterf(GL_TEXTURE_RECTANGLE_EXT, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glPixelStoref(GL_UNPACK_CLIENT_STORAGE_APPLE, GL_TRUE);
		
		if ([self isWidescreen])
			glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA, kLCDDisplayWidescreenWidth, kLCDDisplayHeight, 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, _wlcd_buffer);
		else
			glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA, kLCDDisplayWidth, kLCDDisplayHeight, 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, _lcd_buffer);
		
		glBegin(GL_QUADS);
		glTexCoord2f(0.0f, 0.0f);
		glVertex2f(-1.0f, 1.0f);
		glTexCoord2f(0.0f, height );
		glVertex2f(-1.0f, -1.0f);
		glTexCoord2f(width, height );
		glVertex2f(1.0f, -1.0f);
		glTexCoord2f(width, 0.0f );
		glVertex2f(1.0f, 1.0f);
		glEnd();
		
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		
		glDisable(GL_BLEND);
	}
	
	glFinish();
}

- (void)prepareOpenGL {
	[super prepareOpenGL];
	
	glClearColor(128.0/255.0, 142.0/255.0, 107.0/255.0, 1.0);
	
	glEnable(GL_TEXTURE_RECTANGLE_EXT);
	
	glDisable(GL_DITHER);
	glDisable(GL_ALPHA_TEST);
	glDisable(GL_STENCIL_TEST);
	glDisable(GL_FOG);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_DEPTH_TEST);
	glGenTextures(2, _textures);
}

- (void)reshape {
	// comment this out to see something interesting when you have more than one calc open and resize
	[[self openGLContext] makeCurrentContext];
	
	NSRect bounds = [self bounds];
	glViewport( 0, 0, bounds.size.width, bounds.size.height );
}

- (NSDragOperation)draggingEntered:(id<NSDraggingInfo>)sender {
	if ([sender draggingSource] == self)
		return NSDragOperationNone;
	
	NSArray *filePaths = [[sender draggingPasteboard] propertyListForType:NSFilenamesPboardType];
	
	[self setCurrentFilePaths:[WETransferSheetController validateFilePaths:filePaths]];
	return ([[self currentFilePaths] count] > 0)?NSDragOperationCopy:NSDragOperationNone;
}

- (BOOL)performDragOperation:(id<NSDraggingInfo>)sender {
	[WETransferSheetController transferFiles:[self currentFilePaths] toCalculator:[self calculator]];
	return YES;
}

@dynamic calculator;
- (RSCalculator *)calculator {
	return _calculator;
}
- (void)setCalculator:(RSCalculator *)calculator {
	if (_calculator == calculator)
		return;
	
	if (_calculator != nil)
		[[NSNotificationCenter defaultCenter] removeObserver:self name:kRSCalculatorModelDidChangeNotification object:_calculator];
	
	[_calculator release];
	_calculator = [calculator retain];
	
	[self setIsWidescreen:([[self calculator] model] == RSCalculatorModelTI85 || [[self calculator] model] == RSCalculatorModelTI86)];
	
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(_calculatorModelDidChange:) name:kRSCalculatorModelDidChangeNotification object:_calculator];
}
@dynamic skinView;
- (RSCalculatorSkinView *)skinView {
	RSCalculatorSkinView *retval = nil;
	for (id view in [[self superview] subviews]) {
		if ([view isKindOfClass:[RSCalculatorSkinView class]]) {
			retval = view;
			break;
		}
	}
	return retval;
}
@synthesize isWidescreen=_isWidescreen;
@dynamic hasSkin;
- (BOOL)hasSkin {
	return ([self skinView] != nil);
}
@dynamic skinIsBorderless;
- (BOOL)skinIsBorderless {
	return ([[self window] styleMask] == NSBorderlessWindowMask);
}
@synthesize currentFilePaths=_currentFilePaths;

- (void)commonInit; {	
	[self registerForDraggedTypes:[NSArray arrayWithObjects:NSFilenamesPboardType, nil]];
	[self _privateInit];
}

@dynamic LCDBitmap;
- (NSBitmapImageRep *)LCDBitmap {
	NSUInteger width = ([self isWidescreen])?kLCDDisplayWidescreenWidth:kLCDDisplayWidth, height = kLCDDisplayHeight;
	NSBitmapImageRep *bitmap = [[[NSBitmapImageRep alloc] initWithBitmapDataPlanes:NULL pixelsWide:width pixelsHigh:height bitsPerSample:8 samplesPerPixel:3 hasAlpha:NO isPlanar:NO colorSpaceName:NSCalibratedRGBColorSpace bytesPerRow:0 bitsPerPixel:0] autorelease];
	
	uint8_t *lcd = LCD_image([[self calculator] calc]->cpu.pio.lcd);
	uint16_t row, col;
	for (row=0; row<height; row++) {
		for (col=0; col<width; col++) {
			uint8_t val = 255-lcd[(row/2)*kLCDWidescreenWidth+(col/2)];
			NSUInteger pixel[3];
			
			pixel[2] = val;
			pixel[1] = val;
			pixel[0] = val;
			
			[bitmap setPixel:pixel atX:col y:row];
		}
	}
	return bitmap;
}

- (void)adjustSkinViewRespectingUserDefaults:(BOOL)respectUserDefaults; {	
	if ((respectUserDefaults && ![[NSUserDefaults standardUserDefaults] boolForKey:kRSCalculatorSkinViewUseSkinsKey]) ||
		(!respectUserDefaults && ![self hasSkin]))
		return;
	
	BOOL useBorderlessSkin = ((respectUserDefaults && [[NSUserDefaults standardUserDefaults] boolForKey:kRSCalculatorSkinViewUseBorderlessSkinsKey]) || (!respectUserDefaults && [self skinIsBorderless]));
	RSCalculatorSkinView *skinView = [self skinView];
	
	if (!useBorderlessSkin && respectUserDefaults)
		[[self window] setStyleMask:(NSTitledWindowMask|NSClosableWindowMask|NSMiniaturizableWindowMask)];
	
	NSImage *skin = [[self calculator] skinImage];
	
	[skinView removeFromSuperviewWithoutNeedingDisplay];
	
	skinView = [[[RSCalculatorSkinView alloc] initWithCalculator:[self calculator] frame:NSMakeRect(0, 0, [skin size].width, [skin size].height)] autorelease];
	
	[skinView setImage:skin];
	
	[self setAutoresizingMask:NSViewNotSizable];
	
	CGFloat newWidth = [skin size].width;
	CGFloat newHeight = (useBorderlessSkin)?[skin size].height:[skin size].height + [[self window] contentBorderThicknessForEdge:NSMinYEdge];
	NSRect currentRect = [[self window] frame];
	NSRect newRect = [[self window] frameRectForContentRect:NSMakeRect(NSMinX(currentRect), NSMinY(currentRect), newWidth, newHeight)];
	
	if (NSHeight(currentRect) < NSHeight(newRect))
		newRect.origin.y -= NSHeight(newRect) - NSHeight(currentRect);
	else if (NSHeight(currentRect) > NSHeight(newRect))
		newRect.origin.y += NSHeight(currentRect) - NSHeight(newRect);
	
	[[self window] setFrame:newRect display:YES];
	
	[[[self window] contentView] addSubview:skinView positioned:NSWindowBelow relativeTo:self];
	
	[skinView setFrameOrigin:NSMakePoint([skinView frame].origin.x, (useBorderlessSkin)?[skinView frame].origin.y:[skinView frame].origin.y + [[self window] contentBorderThicknessForEdge:NSMinYEdge])];
	
	NSImage *keymap = [[self calculator] keymapImage];
	if (keymap) {
		NSBitmapImageRep *bitmap = (NSBitmapImageRep *)[keymap bestRepresentationForRect:NSZeroRect context:nil hints:nil];
		if (bitmap) {
			NSSize size = [bitmap size];
			NSUInteger width = size.width, height = size.height;
			NSUInteger i, j;
			NSUInteger pixels[4];
			NSPoint point = NSZeroPoint;
			NSPoint endPoint = NSZeroPoint;
			
			for (i = 0; i < width; i++) {
				for (j = 0; j < height; j++) {
					[bitmap getPixel:pixels atX:i y:j];
					
					// red marks the start of the area for the lcd
					if (pixels[0] == 255 && pixels[1] == 0 && pixels[2] == 0) {
						point.x = i;
						point.y = j;
						
						while (pixels[0] == 255 && pixels[1] == 0 && pixels[2] == 0)
							[bitmap getPixel:pixels atX:i++ y:j];
						
						endPoint.x = i;
						i = point.x;
						
						[bitmap getPixel:pixels atX:i y:j];
						
						while (pixels[0] == 255 && pixels[1] == 0 && pixels[2] == 0)
							[bitmap getPixel:pixels atX:i y:j++];
						
						endPoint.y = j;
						break;
					}
				}
				
				if (!NSEqualPoints(point, NSZeroPoint))
					break;
			}
			
			if (!NSEqualPoints(point, NSZeroPoint)) {					
				[self setFrameOrigin:NSMakePoint(point.x, ((useBorderlessSkin)?[skin size].height:[skin size].height+[[self window] contentBorderThicknessForEdge:NSMinYEdge])-(point.y + (endPoint.y-point.y)))];
				[self setFrameSize:NSMakeSize(endPoint.x-point.x, endPoint.y-point.y)];
			}
		}
	}
}

- (IBAction)toggleScreenCapture:(id)sender; {
	if ([[self calculator] isDebugging] || ![[self calculator] isRunning])
		return;
	
	if ([[self calculator] calc]->gif_disp_state == GIF_IDLE) {
		const char *gifPath = [[[NSHomeDirectory() stringByAppendingPathComponent:@"Desktop"] stringByAppendingPathComponent:@"screencapture.gif"] fileSystemRepresentation];
		
		calc_start_screenshot([[self calculator] calc], gifPath);
	}
	else if ([[self calculator] calc]->gif_disp_state == GIF_START) {
		calc_stop_screenshot([[self calculator] calc]);
	}
}

- (IBAction)saveScreenCaptureAs:(id)sender; {
	if ([[self calculator] isDebugging] ||
		![[self calculator] isRunning] ||
		[[self calculator] calc]->gif_disp_state != GIF_IDLE)
		return;
	
	NSSavePanel *panel = [NSSavePanel savePanel];

	[panel setRequiredFileType:(NSString *)kUTTypeGIF];
	[panel setMessage:NSLocalizedString(@"Choose a name and location for your screen capture.", @"save screen capture as save panel message")];
	[panel setCanCreateDirectories:YES];
	[panel setNameFieldStringValue:NSLocalizedString(@"screen_capture", @"save screen capture as save panel name field string value")];
	
	[panel beginSheetModalForWindow:[self window] completionHandler:^(NSInteger result) {
		if (result != NSFileHandlingPanelOKButton)
			return;
		
		[self toggleScreenCapture:nil];
	}];
}

- (void)_privateInit {
	uint16_t row, col;
	for (row=0; row<kLCDHeight; row++) {
		for (col=0; col<kLCDWidth; col++) {
			// alpha channel is always the same, set it once and forget it, while the lcd doesn't have an alpha channel, opengl requires it for textures
			_buffer[row][col][3] = 255;
		}
	}
	
	for (row=0; row<kLCDDisplayHeight; row++) {
		for (col=0; col<kLCDDisplayWidth; col++) {
			if (col%2 == 0 && row%2 == 0) {
				_lcd_buffer[row][col][2] = 158;
				_lcd_buffer[row][col][1] = 171;
				_lcd_buffer[row][col][0] = 136;
			}
			else if (col%2 == 1 && row%2 == 1) {
				_lcd_buffer[row][col][2] = 126;
				_lcd_buffer[row][col][1] = 137;
				_lcd_buffer[row][col][0] = 109;
			}
			else {
				_lcd_buffer[row][col][2] = 142;
				_lcd_buffer[row][col][1] = 154;
				_lcd_buffer[row][col][0] = 122;
			}
			// alpha channel is always the same, set it once and forget it, while the lcd doesn't have an alpha channel, opengl requires it for textures
			_lcd_buffer[row][col][3] = 108;
		}
	}
	
	for (row=0; row<kLCDHeight; row++) {
		for (col=0; col<kLCDWidescreenWidth; col++) {
			// alpha channel is always the same, set it once and forget it, while the lcd doesn't have an alpha channel, opengl requires it for textures
			_wbuffer[row][col][3] = 255;
		}
	}
	
	for (row=0; row<kLCDDisplayHeight; row++) {
		for (col=0; col<kLCDDisplayWidescreenWidth; col++) {
			if (col%2 == 0 && row%2 == 0) {
				_wlcd_buffer[row][col][2] = 158;
				_wlcd_buffer[row][col][1] = 171;
				_wlcd_buffer[row][col][0] = 136;
			}
			else if (col%2 == 1 && row%2 == 1) {
				_wlcd_buffer[row][col][2] = 126;
				_wlcd_buffer[row][col][1] = 137;
				_wlcd_buffer[row][col][0] = 109;
			}
			else {
				_wlcd_buffer[row][col][2] = 142;
				_wlcd_buffer[row][col][1] = 154;
				_wlcd_buffer[row][col][0] = 122;
			}
			// alpha channel is always the same, set it once and forget it, while the lcd doesn't have an alpha channel, opengl requires it for textures
			_wlcd_buffer[row][col][3] = 108;
		}
	}
}

- (void)_calculatorModelDidChange:(NSNotification *)note {
	[self setIsWidescreen:([[self calculator] model] == RSCalculatorModelTI85 || [[self calculator] model] == RSCalculatorModelTI86)];
	[self adjustSkinViewRespectingUserDefaults:NO];
}
@end
