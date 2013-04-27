#define GR_BUFFER_TEXTUREBUFFER_EXT       0x6
#define GR_BUFFER_TEXTUREAUXBUFFER_EXT    0x7

typedef FxU32 GrPixelFormat_t;
#define GR_PIXFMT_RGB_565                 0x03
#define GR_PIXFMT_ARGB_1555               0x0004
#define GR_PIXFMT_ARGB_8888               0x0005

typedef FxU32 GrCCUColor_t;
typedef FxU32 GrACUColor_t;
typedef FxU32 GrTCCUColor_t;
typedef FxU32 GrTACUColor_t;
#define GR_CMBX_ZERO                      0x00
#define GR_CMBX_TEXTURE_ALPHA             0x01
#define GR_CMBX_ALOCAL                    0x02
#define GR_CMBX_AOTHER                    0x03
#define GR_CMBX_B                         0x04
#define GR_CMBX_CONSTANT_ALPHA            0x05
#define GR_CMBX_CONSTANT_COLOR            0x06
#define GR_CMBX_DETAIL_FACTOR             0x07
#define GR_CMBX_ITALPHA                   0x08
#define GR_CMBX_ITRGB                     0x09
#define GR_CMBX_LOCAL_TEXTURE_ALPHA       0x0a
#define GR_CMBX_LOCAL_TEXTURE_RGB         0x0b
#define GR_CMBX_LOD_FRAC                  0x0c
#define GR_CMBX_OTHER_TEXTURE_ALPHA       0x0d
#define GR_CMBX_OTHER_TEXTURE_RGB         0x0e
#define GR_CMBX_TEXTURE_RGB               0x0f
#define GR_CMBX_TMU_CALPHA                0x10
#define GR_CMBX_TMU_CCOLOR                0x11

typedef FxU32 GrCombineMode_t;
#define GR_FUNC_MODE_ZERO                 0x00
#define GR_FUNC_MODE_X                    0x01
#define GR_FUNC_MODE_ONE_MINUS_X          0x02
#define GR_FUNC_MODE_NEGATIVE_X           0x03
#define GR_FUNC_MODE_X_MINUS_HALF         0x04

#define GR_TEXFMT_ARGB_8888               0x12

#define GR_LOD_LOG2_2048                  0xb
#define GR_LOD_LOG2_1024                  0xa
#define GR_LOD_LOG2_512                   0x9

#define GR_TEXTURE_UMA_EXT                0x06
//wrapper specific
FX_ENTRY void FX_CALL grConfigWrapperExt(FxI32, FxI32, FxBool, FxBool);
FX_ENTRY GrScreenResolution_t FX_CALL grWrapperFullScreenResolutionExt(FxU32*, FxU32*);
FX_ENTRY char ** FX_CALL grQueryResolutionsExt(FxI32*);
FX_ENTRY FxBool FX_CALL grKeyPressedExt(FxU32 key);
FX_ENTRY void FX_CALL grGetGammaTableExt(FxU32, FxU32*, FxU32*, FxU32*);

FX_ENTRY GrContext_t FX_CALL
	grSstWinOpenExt(
	FxU32                hWnd,
	GrScreenResolution_t screen_resolution,
	GrScreenRefresh_t    refresh_rate,
	GrColorFormat_t      color_format,
	GrOriginLocation_t   origin_location,
	GrPixelFormat_t      pixelformat,
	int                  nColBuffers,
	int                  nAuxBuffers);

//color combiner
FX_ENTRY void FX_CALL
	grColorCombineExt(GrCCUColor_t a, GrCombineMode_t a_mode,
	GrCCUColor_t b, GrCombineMode_t b_mode,
	GrCCUColor_t c, FxBool c_invert,
	GrCCUColor_t d, FxBool d_invert,
	FxU32 shift, FxBool invert);

FX_ENTRY void FX_CALL
	grAlphaCombineExt(GrACUColor_t a, GrCombineMode_t a_mode,
	GrACUColor_t b, GrCombineMode_t b_mode,
	GrACUColor_t c, FxBool c_invert,
	GrACUColor_t d, FxBool d_invert,
	FxU32 shift, FxBool invert);

FX_ENTRY void FX_CALL
	grTexColorCombineExt(GrChipID_t       tmu,
	GrTCCUColor_t a, GrCombineMode_t a_mode,
	GrTCCUColor_t b, GrCombineMode_t b_mode,
	GrTCCUColor_t c, FxBool c_invert,
	GrTCCUColor_t d, FxBool d_invert,
	FxU32 shift, FxBool invert);

FX_ENTRY void FX_CALL
	grTexAlphaCombineExt(GrChipID_t       tmu,
	GrTACUColor_t a, GrCombineMode_t a_mode,
	GrTACUColor_t b, GrCombineMode_t b_mode,
	GrTACUColor_t c, FxBool c_invert,
	GrTACUColor_t d, FxBool d_invert,
	FxU32 shift, FxBool invert);

FX_ENTRY void FX_CALL
	grConstantColorValueExt(GrChipID_t    tmu,
	GrColor_t     value);

//texture buffer
FX_ENTRY void FX_CALL grTextureBufferExt( GrChipID_t  		tmu,
	FxU32 				startAddress,
	GrLOD_t 			lodmin,
	GrLOD_t 			lodmax,
	GrAspectRatio_t 	aspect,
	GrTextureFormat_t 	fmt,
	FxU32 				evenOdd);

FX_ENTRY void FX_CALL
	grTextureAuxBufferExt( GrChipID_t tmu,
	FxU32      startAddress,
	GrLOD_t    thisLOD,
	GrLOD_t    largeLOD,
	GrAspectRatio_t aspectRatio,
	GrTextureFormat_t format,
	FxU32      odd_even_mask );
FX_ENTRY void FX_CALL grAuxBufferExt( GrBuffer_t buffer );