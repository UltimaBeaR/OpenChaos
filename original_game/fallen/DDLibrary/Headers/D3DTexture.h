// D3DTexture.h
// Guy Simmons, 29th November 1997.

#ifndef	D3DTEXTURE_H
#define	D3DTEXTURE_H


// Call after doing lots of loading.
void NotGoingToLoadTexturesForAWhileNowSoYouCanCleanUpABit ( void );

// handle = file opened with FileOpen
// dwSize = number of bytes to load.
// return = where the file was loaded.
// You _must_ copy this or do something with it before loading another file.
void *FastLoadFileSomewhere ( MFFileHandle handle, DWORD dwSize );

#include	"tga.h"
#include	"FileClump.h"

struct	Char
{
	SLONG		X,
				Y,
				Height,
				Width;
};

struct	Font
{
	SLONG			StartLine;
	Char			CharSet[96];
	Font			*NextFont;
};

#define	D3D_TEXTURE_FONT		(1<<0)
#define D3D_TEXTURE_FONT2		(1<<1)	// texture is a font with red borders

#define D3DTEXTURE_TYPE_UNUSED	0
#define D3DTEXTURE_TYPE_TGA		1
#define D3DTEXTURE_TYPE_USER	2		// The user updates the contents- it doesn't have a file.


// First number is size of subtextures, second is number of subtextures in page.
// 3x3 means the page is 4x as big in each direction, with lots of pixel padding.
#define D3DPAGE_NONE			0
#define D3DPAGE_64_3X3			1
#define D3DPAGE_64_4X4			2
#define D3DPAGE_32_3X3			3
#define D3DPAGE_32_4X4			4

class	D3DTexture
{
	public:

		CBYTE					texture_name[256];
		ULONG					ID;		// texture ID for FileClump
		// Allow the texture to be shrunk or replaced with junk for faster loading.
		BOOL					bCanShrink;
		SLONG					TextureFlags;
		Font					*FontList;
		LPDIRECT3DTEXTURE2		lp_Texture;
		LPDIRECTDRAWSURFACE4	lp_Surface;
		HRESULT					Reload_TGA (void);
		HRESULT					Reload_user(void);
		BOOL					GreyScale;
		BOOL					UserWantsAlpha;	// The user page needs an alpha-channel.
#ifdef TEX_EMBED
		//char *name;			// Texture file name.
		UBYTE bPagePos;			// Position in page.
		UBYTE bPageType;		// One of D3DPAGE_xxx
		WORD wPageNum;			// The D3Dpage this is in.
#endif

		D3DTexture				*NextTexture;


		D3DTexture()							{
													TextureFlags	=	0;
													FontList		=	NULL;
													NextTexture=NULL;
													#ifndef TARGET_DC
													#endif
													lp_Surface=NULL;
													lp_Texture=NULL;
													Type = D3DTEXTURE_TYPE_UNUSED;
#ifdef TEX_EMBED
													wPageNum = -1;	// None.
													bPagePos = 0;
													bPageType = D3DPAGE_NONE;
#endif
													// IT WOULD FUCKING HELP IF SOME OF THESE ACTUALLY GOT SET UP WITH DEFAULTS
													GreyScale = FALSE;
													UserWantsAlpha = FALSE;
													ID = -1;
													bCanShrink = FALSE;
												}

		//
		// The format used.
		// 

		SLONG mask_red=0;
		SLONG mask_green=0;
		SLONG mask_blue = 0;
		SLONG mask_alpha = 0;

		SLONG shift_red = 0;
		SLONG shift_green = 0;
		SLONG shift_blue = 0;
		SLONG shift_alpha = 0;

		SLONG		Type;
		SLONG       size;			// The size in pixels of the texture page.
		SLONG       ContainsAlpha;

		HRESULT		LoadTextureTGA(CBYTE *tga_file,ULONG texid,BOOL bCanShrink=TRUE);

		HRESULT		ChangeTextureTGA(CBYTE *tga_file);

		HRESULT		CreateUserPage(SLONG size, BOOL i_want_an_alpha_channel);	// Power of two between 32 and 256 inclusive
		HRESULT     LockUser      (UWORD **bitmap, SLONG *pitch);				// Returns the texture page on success. The pitch is in bytes!
		void        UnlockUser	  (void);

		HRESULT		Reload(void);
		HRESULT		Destroy(void);

		HRESULT		CreateFonts(TGA_Info *tga_info,TGA_Pixel *tga_data);
		Font		*GetFont(SLONG id);

		// resets texture page for loading
		static void	BeginLoading();

		//
		// Makes the texture grey-scale. If it is already loaded, it
		// is re-loaded.
		//

		void set_greyscale(BOOL is_greyscale);

		LPDIRECT3DTEXTURE2		GetD3DTexture()			{ return lp_Texture; }
		LPDIRECTDRAWSURFACE4	GetSurface(void)		{ return lp_Surface; }
#ifdef TEX_EMBED
		void					GetTexOffsetAndScale ( float *pfUScale, float *pfUOffset, float *pfVScale, float *pfVOffset );
#endif

		HRESULT					SetColorKey(SLONG flags,LPDDCOLORKEY key)	{
																					if (lp_Surface)
																					{
																						return	lp_Surface->SetColorKey(flags,key);
																					}
																					else
																					{
																						return DDERR_GENERIC;
																					}
																				}

		inline	BOOL		IsFont(void)				{	return	TextureFlags&D3D_TEXTURE_FONT;	}
		inline	void		FontOn(void)				{	TextureFlags	|=	D3D_TEXTURE_FONT;	}
		inline	void		FontOff(void)				{	TextureFlags	&=	~D3D_TEXTURE_FONT;	}

		inline	BOOL		IsFont2(void)				{	return	TextureFlags&D3D_TEXTURE_FONT2;	}
		inline	void		Font2On(void)				{	TextureFlags	|=	D3D_TEXTURE_FONT2;	}
		inline	void		Font2Off(void)				{	TextureFlags	&=	~D3D_TEXTURE_FONT2;	}
};



class D3DPage
{
public:
	UBYTE		bPageType;			// One of D3DPAGE_xxx
	UBYTE		bNumTextures;		// Number of textures in page.
	char		*pcDirectory;		// The page's directory. THIS IS STATIC - don't free it.
	char		*pcFilename;		// The page's filename. THIS IS STATIC - don't free it.
	char		**ppcTextureList;	// A pointer to an array of pointers to strings of the texture names :-)
	D3DTexture	*pTex;				// The texture page texture itself.
	//D3DTexture	*pTextures[16];		// The textures in this page, in order.

	D3DPage ( void )
	{
		bPageType = 0;
		bNumTextures = 0;
		pcDirectory = NULL;
		pcFilename = NULL;
		pTex = NULL;
		ppcTextureList = NULL;
	}

	// Call this when linking a standard D3DTexture to the page - it will demand-load the page's texture.
	void D3DPage::EnsureLoaded ( void );
	// Call this when unloading everything.
	void D3DPage::Unload ( void );

};

#endif


