#pragma once
#include "ScriptAPI.h"

enum
{
    G_IM_FMT_RGBA,
    G_IM_FMT_YUV,
    G_IM_FMT_CI,
    G_IM_FMT_IA,
    G_IM_FMT_I
};

enum
{
    G_IM_SIZ_4b,
    G_IM_SIZ_8b,
    G_IM_SIZ_16b,
    G_IM_SIZ_32b
};

enum
{
    G_TT_NONE = 0x0000,
    G_TT_RGBA16 = 0x8000,
    G_TT_IA16 = 0xC000,
};

enum
{
    IMG_RGBA16 = (G_IM_FMT_RGBA << 3 | G_IM_SIZ_16b),
    IMG_RGBA32 = (G_IM_FMT_RGBA << 3 | G_IM_SIZ_32b),
    IMG_CI4_RGBA16 = (G_IM_FMT_CI << 3 | G_IM_SIZ_4b) | G_TT_RGBA16,
    IMG_CI4_IA16 = (G_IM_FMT_CI << 3 | G_IM_SIZ_4b) | G_TT_IA16,
    IMG_CI8_RGBA16 = (G_IM_FMT_CI << 3 | G_IM_SIZ_8b) | G_TT_RGBA16,
    IMG_CI8_IA16 = (G_IM_FMT_CI << 3 | G_IM_SIZ_8b) | G_TT_IA16,
    IMG_IA4 = (G_IM_FMT_IA << 3 | G_IM_SIZ_4b),
    IMG_IA8 = (G_IM_FMT_IA << 3 | G_IM_SIZ_8b),
    IMG_IA16 = (G_IM_FMT_IA << 3 | G_IM_SIZ_16b),
    IMG_I4 = (G_IM_FMT_I << 3 | G_IM_SIZ_4b),
    IMG_I8 = (G_IM_FMT_I << 3 | G_IM_SIZ_8b),
};

enum N64ImageResult
{
    N64IMG_OK,
    N64IMG_DATA_SIZE_INCORRECT,
    N64IMG_INVALID_COLOR_INDEX,
    N64IMG_INCOMPATIBLE_COLOR,
    N64IMG_TOO_MANY_COLORS,
    N64IMG_PNG_HEADER_MISSING,
    N64IMG_PNG_OUT_OF_MEMORY,
    N64IMG_PNG_EXCEPTION,
    N64IMG_PNG_PARSER_FAILED
};

class CN64Image
{
private:
    int m_PixelSize;
    int m_Format;
    bool m_bUsePalette;

    size_t m_Width;
    size_t m_Height;
    size_t m_NumPixels;

    std::vector<uint8_t> m_PixelData;
    std::vector<uint8_t> m_PaletteData;
    std::vector<uint8_t> m_BitmapRgba32;

public:
    CN64Image();

    int Init(int format, size_t width, size_t height,
             void * pixelData = nullptr, size_t pixelDataSize = 0,
             void * paletteData = nullptr, size_t paletteDataSize = 0);

    int Init(int format, uint8_t * pngData, size_t pngSize);
    void ToPNG(std::vector<uint8_t> & outPngImage);

    int UpdateBitmap();
    std::vector<uint8_t> & PaletteData();
    std::vector<uint8_t> & PixelData();
    std::vector<uint8_t> & Bitmap();
    size_t Width();
    size_t Height();
    int Format();
    bool UsesPalette();

    static int ReadPNG(uint8_t * pngData, size_t pngSize, size_t * width, size_t * height, std::vector<uint8_t> & outRGBA32);
    static void WritePNG(uint8_t * rgba32, size_t width, size_t height, std::vector<uint8_t> & buffer);

    static unsigned int ColorFromRgba32(int dstFormat, uint32_t rgba32);
    static uint32_t ColorToRgba32(int srcFormat, unsigned int color);
    static int BitsPerPixel(int format);
    static int PaletteColorCount(int format);
    static bool UsesPalette(int format);
    static const char * ResultCodeName(int resultCode);

private:
    uint16_t * PalettePtr(size_t index);
    void * TexelPtr(size_t index);
    uint32_t * BitmapPtr(size_t index);
    unsigned int GetTexel(size_t index);
    void SetTexel(size_t index, unsigned int value);
    bool GetPaletteColor(size_t index, unsigned int * color);
    bool SetPaletteColor(size_t index, unsigned int color);
    bool GetBitmapColor(size_t index, uint32_t * color);
    bool SetBitmapColor(size_t index, unsigned int color);
    int UpdatePixelsAndPaletteFromBitmap();
};
