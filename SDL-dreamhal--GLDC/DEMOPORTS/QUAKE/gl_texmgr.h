/*
Copyright (C) 1996-2001 Id Software, Inc.
Copyright (C) 2002-2005 John Fitzgibbons and others
Copyright (C) 2007-2008 Kristian Duske

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

//gl_texmgr.h -- fitzquake's texture manager. manages opengl texture images

#define TEXPREF_NONE			0x0000
#define TEXPREF_MIPMAP			0x0001	// generate mipmaps
#define TEXPREF_LINEAR			0x0002	// force linear
#define TEXPREF_NEAREST			0x0004	// force nearest
#define TEXPREF_ALPHA			0x0008	// allow alpha
#define TEXPREF_PAD				0x0010	// allow padding
#define TEXPREF_PERSIST			0x0020	// never free
#define TEXPREF_OVERWRITE		0x0040	// overwrite existing same-name texture
#define TEXPREF_NOPICMIP		0x0080	// always load full-sized
#define TEXPREF_FULLBRIGHT		0x0100	// use fullbright mask palette
#define TEXPREF_CONCHARS		0x0200	// use conchars palette
#define TEXPREF_WARPIMAGE		0x0400	// resize this texture when warpimagesize changes

enum srcformat {SRC_INDEXED, SRC_LIGHTMAP, SRC_RGBA};

typedef struct gltexture_s {
//managed by texture manager
	unsigned int		texnum;
	struct gltexture_s	*next;
	model_t				*owner;
//managed by image loading
	char				name[64];
	unsigned int		width; //size of image as it exists in opengl
	unsigned int		height; //size of image as it exists in opengl
	unsigned int		flags;
	char				shirt; //0-13 shirt color, or -1 if never colormapped
	char				pants; //0-13 pants color, or -1 if never colormapped
	char				source_file[MAX_OSPATH]; //filepath to data source, or "" if source is in memory
	unsigned int		source_offset; //byte offset into file, or memory address
	enum srcformat		source_format; //format of pixel data (indexed, lightmap, or rgba)
	unsigned int		source_width; //size of image in source data
	unsigned int		source_height; //size of image in source data
	unsigned short		source_crc; //generated by source data before modifications
//used for rendering
	int					visframe; //matches r_framecount if texture was bound this frame
} gltexture_t;

gltexture_t *notexture;
gltexture_t *nulltexture;

unsigned int d_8to24table[256];
unsigned int d_8to24table_fbright[256];
unsigned int d_8to24table_conchars[256];
unsigned int d_8to24table_shirt[256];
unsigned int d_8to24table_pants[256];

// TEXTURE MANAGER

float TexMgr_FrameUsage (void);
gltexture_t *TexMgr_FindTexture (model_t *owner, char *name);
gltexture_t *TexMgr_NewTexture (void);
void TexMgr_FreeTexture (gltexture_t *kill);
void TexMgr_FreeTextures (int flags, int mask);
void TexMgr_FreeTexturesForOwner (model_t *owner);
void TexMgr_NewGame (void);
void TexMgr_Init (void);

// IMAGE LOADING
gltexture_t *TexMgr_LoadImage (model_t *owner, char *name, int width, int height, enum srcformat format,
							   byte *data, char *source_file, unsigned source_offset, unsigned flags);
void TexMgr_ReloadImage (gltexture_t *glt, int shirt, int pants);
void TexMgr_ReloadImages (void);

int TexMgr_Pad(int s);
int TexMgr_SafeTextureSize (int s);
int TexMgr_PadConditional (int s);

// TEXTURE BINDING & TEXTURE UNIT SWITCHING

extern qboolean gl_mtexable;

void GL_DisableMultitexture (void); //selects texture unit 0
void GL_EnableMultitexture (void); //selects texture unit 1
void GL_Bind (gltexture_t *texture);
