/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

#include "BMPImporter.h"

#include "RGBAColor.h"
#include "win32def.h"

#include "Interface.h"
#include "Video.h"

#define BMP_HEADER_SIZE  54

static ieDword red_mask = 0x00ff0000;
static ieDword green_mask = 0x0000ff00;
static ieDword blue_mask = 0x000000ff;

BMPImporter::BMPImporter(void)
{
	Palette = NULL;
	pixels = NULL;
	if (DataStream::IsEndianSwitch()) {
		red_mask = 0x000000ff;
		green_mask = 0x0000ff00;
		blue_mask = 0x00ff0000;
	}
}

BMPImporter::~BMPImporter(void)
{
	free( Palette );
	free( pixels );
}

bool BMPImporter::Open(DataStream* stream)
{
	str = stream;
	//we release the previous pixel data
	free( pixels );
	pixels = NULL;
	free( Palette );
	Palette = NULL;

	//BITMAPFILEHEADER
	char Signature[2];
	ieDword FileSize, DataOffset;

	str->Read( Signature, 2 );
	if (strncmp( Signature, "BM", 2 ) != 0) {
		printMessage( "BMPImporter","Not a valid BMP File.\n",LIGHT_RED);
		return false;
	}
	str->ReadDword( &FileSize );
	str->Seek( 4, GEM_CURRENT_POS );
	str->ReadDword( &DataOffset );

	//BITMAPINFOHEADER

	str->ReadDword( &Size );
	//some IE palettes are of a different format (OS/2 BMP)!
	if (Size < 24) {
		printMessage( "BMPImporter","OS/2 Bitmap, not supported.\n", LIGHT_RED);
		return false;
	}
	str->ReadDword( &Width );
	str->ReadDword( &Height );
	str->ReadWord( &Planes );
	str->ReadWord( &BitCount );
	str->ReadDword( &Compression );
	str->ReadDword( &ImageSize );
	//24 = the already read bytes 3x4+2x2+2x4
	//this is normally 16
	str->Seek( Size-24, GEM_CURRENT_POS );
	//str->ReadDword(&Hres );
	//str->ReadDword(&Vres );
	//str->ReadDword(&ColorsUsed );
	//str->ReadDword(&ColorsImportant );
	if (Compression != 0) {
		printMessage( "BMPImporter"," ", LIGHT_RED);
		printf( "Compressed %d-bits Image, not supported.\n", BitCount );
		return false;
	}
	//COLORTABLE
	Palette = NULL;
	if (BitCount <= 8) {
		if (BitCount == 8)
			NumColors = 256;
		else
			NumColors = 16;
		Palette = ( Color * ) malloc( 4 * NumColors );
		for (unsigned int i = 0; i < NumColors; i++) {
			str->Read( &Palette[i].b, 1 );
			str->Read( &Palette[i].g, 1 );
			str->Read( &Palette[i].r, 1 );
			str->Read( &Palette[i].a, 1 );
		}
	}
	str->Seek( DataOffset, GEM_STREAM_START );
	//no idea if we have to swap this or not
	//RASTERDATA
	switch (BitCount) {
		case 32:
			PaddedRowLength = Width * 4;
			break;

		case 24:
			PaddedRowLength = Width * 3;
			break;

		case 16:
			PaddedRowLength = Width * 2;
			break;

		case 8:
			PaddedRowLength = Width;
			break;

		case 4:
			PaddedRowLength = ( Width >> 1 );
			break;
		default:
			printMessage( "BMPImporter"," ", LIGHT_RED);
			printf( "BitCount %d is not supported.\n", BitCount );
			return false;
	}
	//if(BitCount!=4)
	//{
	if (PaddedRowLength & 3) {
		PaddedRowLength += 4 - ( PaddedRowLength & 3 );
	}
	//}
	void* rpixels = malloc( PaddedRowLength* Height );
	str->Read( rpixels, PaddedRowLength * Height );
	if (BitCount == 32) {
		//convert to 24 bits on the fly
		int size = Width * Height * 3;
		pixels = malloc( size );
		unsigned char * dest = ( unsigned char * ) pixels;
		dest += size;
		unsigned char * src = ( unsigned char * ) rpixels;
		for (int i = Height; i; i--) {
			dest -= ( Width * 3 );
			for (unsigned int j=0;j<Width;j++) {
				dest[j*3]=src[j*4];
				dest[j*3+1]=src[j*4+1];
				dest[j*3+2]=src[j*4+2];
			}
			src += PaddedRowLength;
		}
		BitCount = 24;
	} else if (BitCount == 24) {
		int size = Width * Height * 3;
		pixels = malloc( size );
		unsigned char * dest = ( unsigned char * ) pixels;
		dest += size;
		unsigned char * src = ( unsigned char * ) rpixels;
		for (int i = Height; i; i--) {
			dest -= ( Width * 3 );
			memcpy( dest, src, Width * 3 );
			src += PaddedRowLength;
		}
	} else if (BitCount == 8) {
		pixels = malloc( Width * Height );
		unsigned char * dest = ( unsigned char * ) pixels;
		dest += Height * Width;
		unsigned char * src = ( unsigned char * ) rpixels;
		for (int i = Height; i; i--) {
			dest -= Width;
			memcpy( dest, src, Width );
			src += PaddedRowLength;
		}
	} else if (BitCount == 4) {
		Read4To8(rpixels);
	}
	free( rpixels );
	return true;
}

void BMPImporter::Read8To8(void *rpixels)
{
	pixels = malloc( Width * Height );
	unsigned char * dest = ( unsigned char * ) pixels;
	dest += Height * Width;
	unsigned char * src = ( unsigned char * ) rpixels;
	for (int i = Height; i; i--) {
		dest -= Width;
		memcpy( dest, src, Width );
		src += PaddedRowLength;
	}
}

void BMPImporter::Read4To8(void *rpixels)
{
	BitCount = 8;
	pixels = malloc( Width * Height );
	unsigned char * dest = ( unsigned char * ) pixels;
	dest += Height * Width;
	unsigned char * src = ( unsigned char * ) rpixels;
	for (int i = Height; i; i--) {
		dest -= Width;
		for (unsigned int j=0;j<Width;j++) {
			if (!(j&1)) {
				dest[j] = ((unsigned) src[j/2])>>4;
			} else {
				dest[j] = src[j/2]&15;
			}
		}
		src += PaddedRowLength;
	}
}

Sprite2D* BMPImporter::GetSprite2D()
{
	Sprite2D* spr = NULL;
	if (BitCount == 24) {
		void* p = malloc( Width * Height * 3 );
		memcpy( p, pixels, Width * Height * 3 );
		spr = core->GetVideoDriver()->CreateSprite( Width, Height, 24,
			red_mask, green_mask, blue_mask, 0x00000000, p,
			true, green_mask );
	} else if (BitCount == 8) {
		void* p = malloc( Width* Height );
		memcpy( p, pixels, Width * Height );
		spr = core->GetVideoDriver()->CreateSprite8( Width, Height, NumColors==16?4:8,
			p, Palette, true, 0 );
	}
	return spr;
}

void BMPImporter::GetPalette(int colors, Color* pal)
{
	if (BitCount > 8) {
		ImageMgr::GetPalette(colors, pal);
		return;
	}

	for (int i = 0; i < colors; i++) {
		pal[i].r = Palette[i%NumColors].r;
		pal[i].g = Palette[i%NumColors].g;
		pal[i].b = Palette[i%NumColors].b;
		pal[i].a = 0xff;
	}
}

Bitmap* BMPImporter::GetBitmap()
{
	Bitmap *data = new Bitmap(Width,Height);

	unsigned char *p = ( unsigned char * ) pixels;
	switch (BitCount) {
	case 8:
		unsigned int y;
		for (y = 0; y < Height; y++) {
			for (unsigned int x = 0; x < Width; x++) {
				data->SetAt(x,y,p[y*Width + x]);
			}
		}
		break;
	case 24:
		printMessage("BMPImporter", "Don't know how to handle 24bit bitmap from ", WHITE);
		printf( "%s...", str->filename );
		printStatus( "ERROR", LIGHT_RED );
		for (y = 0; y < Height; y++) {
			for (unsigned int x = 0; x < Width; x++) {
				data->SetAt(x,y,p[3*(y*Width + x)]);
			}
		}
		break;
	}

	return data;
}

#include "plugindef.h"

GEMRB_PLUGIN(0xD768B1, "BMP File Reader")
PLUGIN_IE_RESOURCE(BMPImporter, ".bmp", (ieWord)IE_BMP_CLASS_ID)
END_PLUGIN()
