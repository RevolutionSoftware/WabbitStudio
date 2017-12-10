//
//  WCHexFormatter.m
//  WabbitStudio
//
//  Created by William Towe on 4/20/11.
//  Copyright 2011 Revolution Software. All rights reserved.
//

#import "WCHexFormatter.h"
#import "NSString+WCExtensions.h"


@implementation NSString (WCHexFormatter_Extensions)

- (NSString *)stringByRemovingInvalidHexDigits; {
	if (!self || [self length] == 0)
		return nil;
	
	static NSCharacterSet *allowedCharacters = nil;
	if (!allowedCharacters)
		allowedCharacters = [[NSCharacterSet characterSetWithCharactersInString:@"0123456789abcdefABCDEF"] retain];
	NSUInteger trimLength = 0, length = [self length];
	unichar buffer[length];
	
	for (NSUInteger index = 0; index < length; index++) {
		if ([allowedCharacters characterIsMember:[self characterAtIndex:index]])
			buffer[trimLength++] = [self characterAtIndex:index];
	}
	
	if (trimLength == 0)
		return nil;
	return [[[NSString alloc] initWithCharacters:buffer length:trimLength] autorelease];
}

- (NSString *)stringByRemovingInvalidBaseTenDigits; {
	if (!self || [self length] == 0)
		return nil;
	
	static NSCharacterSet *allowedCharacters = nil;
	if (!allowedCharacters)
		allowedCharacters = [[NSCharacterSet characterSetWithCharactersInString:@"0123456789"] retain];
	NSUInteger trimLength = 0, length = [self length];
	unichar buffer[length];
	
	for (NSUInteger index = 0; index < length; index++) {
		if ([allowedCharacters characterIsMember:[self characterAtIndex:index]])
			buffer[trimLength++] = [self characterAtIndex:index];
	}
	
	if (trimLength == 0)
		return nil;
	return [[[NSString alloc] initWithCharacters:buffer length:trimLength] autorelease];
}

@end

@implementation WCHexFormatter

- (NSString *)stringForObjectValue:(id)object {
	if ([object isKindOfClass:[NSNumber class]])
		return [NSString stringWithFormat:@"%04X",[object unsignedIntValue]];
	return [NSString stringWithFormat:@"%04X",[object integerValue]];
}

- (NSAttributedString *)attributedStringForObjectValue:(id)obj withDefaultAttributes:(NSDictionary *)attrs {
	NSString *string = [self stringForObjectValue:obj];
	NSMutableAttributedString *attributedString = [[[NSMutableAttributedString alloc] initWithString:string attributes:attrs] autorelease];
	
	if ([self shouldDrawWithProgramCounterAttributes]) {
		[attributedString addAttribute:NSForegroundColorAttributeName value:[NSColor redColor] range:NSMakeRange(0, [attributedString length])];
		
		if ([self cellIsHighlighted])
			[attributedString applyFontTraits:NSBoldFontMask range:NSMakeRange(0, [attributedString length])];
	}
	return attributedString;
}

- (BOOL)getObjectValue:(id *)object forString:(NSString *)string errorDescription:(NSString **)error {
	string = [string stringByRemovingInvalidHexDigits];
	
	if (!string || [string length] == 0) {
		*object = [NSNumber numberWithUnsignedInt:0];
		return YES;
	}
	
	NSInteger index = [string length];
	uint32_t total = 0, exponent = 0, base = 16;
	
	while (index > 0) {
		uint8_t value = HexValueForCharacter([string characterAtIndex:--index]);
		total += value * (uint32_t)powf(base, exponent++);
	}
	
	*object = [NSNumber numberWithUnsignedInt:total];
	return YES;
}

@synthesize shouldDrawWithProgramCounterAttributes=_shouldDrawWithProgramCounterAttributes;
@synthesize cellIsHighlighted=_cellIsHighlighted;
@end
