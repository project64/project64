#include <stdafx.h>
#include "N64Image.h"

struct ImgFormatInfo {
    int bitsPerPixel;
    int paletteColorCount;
};

static const std::map<int, ImgFormatInfo> FormatInfo = {
    { IMG_I4,         { 4,   0 } },
    { IMG_IA4,        { 4,   0 } },
    { IMG_I8,         { 8,   0 } },
    { IMG_IA8,        { 8,   0 } },
    { IMG_IA16,       { 16,  0 } },
    { IMG_RGBA16,     { 16,  0 } },
    { IMG_RGBA32,     { 32,  0 } },
    { IMG_CI4_RGBA16, { 4,  16 } },
    { IMG_CI4_IA16,   { 4,  16 } },
    { IMG_CI8_RGBA16, { 8,  256 } },
    { IMG_CI8_IA16,   { 8,  256 } },
};

CN64Image::CN64Image() :
    m_PixelSize(0),
    m_Format(IMG_RGBA32),
    m_Width(0),
    m_Height(0),
    m_NumPixels(0),
    m_bUsePalette(false)
{
}

int CN64Image::Init(int format, size_t width, size_t height,
    void* pixelData, size_t pixelDataSize,
    void* paletteData, size_t paletteDataSize)
{
    m_Format = format;
    m_PixelSize = BitsPerPixel(format);
    m_bUsePalette = UsesPalette(format);
    m_Width = width;
    m_Height = height;
    m_NumPixels = width * height;

    size_t requiredPixelDataSize = (m_NumPixels * m_PixelSize) / 8;

    if (pixelData != nullptr && pixelDataSize != requiredPixelDataSize)
    {
        return N64IMG_DATA_SIZE_INCORRECT;
    }
    else
    {
        pixelDataSize = requiredPixelDataSize;
    }

    m_PixelData.resize(pixelDataSize);

    if (pixelData != nullptr)
    {
        memcpy(m_PixelData.data(), pixelData, m_PixelData.size());
    }

    if (m_bUsePalette)
    {
        size_t maxPaletteSize = (1 << m_PixelSize) * 2;

        if (paletteData == nullptr)
        {
            m_PaletteData.resize(maxPaletteSize);
        }
        else
        {
            m_PaletteData.resize(min(paletteDataSize, maxPaletteSize));
            memcpy(m_PaletteData.data(), paletteData, m_PaletteData.size());
        }
    }

    int result = UpdateBitmap();
    if (result != N64IMG_OK)
    {
        return result;
    }

    return N64IMG_OK;
}

int CN64Image::Init(int format, uint8_t* pngData, size_t pngSize)
{
    m_Format = format;
    m_PixelSize = BitsPerPixel(format);
    m_bUsePalette = UsesPalette(format);

    int result = ReadPNG(pngData, pngSize, &m_Width, &m_Height, m_BitmapRgba32);
    if (result != N64IMG_OK)
    {
        return result;
    }

    m_NumPixels = m_Width * m_Height;

    size_t requiredPixelDataSize = (m_NumPixels * m_PixelSize) / 8;
    m_PixelData.resize(requiredPixelDataSize);

    result = UpdatePixelsAndPaletteFromBitmap();
    if (result != N64IMG_OK)
    {
        return result;
    }

    result = UpdateBitmap();
    if (result != N64IMG_OK)
    {
        return result;
    }

    return N64IMG_OK;
}

void CN64Image::ToPNG(std::vector<uint8_t>& outPngImage)
{
    WritePNG(m_BitmapRgba32.data(), m_Width, m_Height, outPngImage);
}

uint16_t* CN64Image::PalettePtr(size_t index)
{
    size_t offset = index * sizeof(uint16_t);
    if (offset + sizeof(uint16_t) > m_PaletteData.size())
    {
        return nullptr;
    }
    return (uint16_t*)&m_PaletteData[offset];
}

void* CN64Image::TexelPtr(size_t index)
{
    size_t offset = (index * m_PixelSize) / 8;
    if (offset + max(1, (m_PixelSize / 8)) > m_PixelData.size())
    {
        return nullptr;
    }
    return (void*)&m_PixelData[offset];
}

uint32_t* CN64Image::BitmapPtr(size_t index)
{
    size_t offset = index * sizeof(uint32_t);
    if (offset + sizeof(uint32_t) > m_BitmapRgba32.size())
    {
        return nullptr;
    }
    return (uint32_t*)&m_BitmapRgba32[offset];
}

unsigned int CN64Image::GetTexel(size_t index)
{
    void* pTexel = TexelPtr(index);

    if (pTexel == nullptr)
    {
        return 0;
    }

    switch (m_PixelSize)
    {
    case 4:
        if ((index % 2) == 0)
        {
            return (*(uint8_t*)pTexel & 0xF0) >> 4;
        }
        else
        {
            return (*(uint8_t*)pTexel & 0x0F);
        }
    case 8:
        return *(uint8_t*)pTexel;
    case 16:
        return _byteswap_ushort(*(uint16_t*)pTexel);
    case 32:
        return _byteswap_ulong(*(uint32_t*)pTexel);
    }

    return 0;
}

void CN64Image::SetTexel(size_t index, unsigned int value)
{
    size_t offset = (index * m_PixelSize) / 8;

    if (offset + (m_PixelSize / 8) > m_PixelData.size())
    {
        return;
    }

    switch (m_PixelSize)
    {
    case 4:
        if ((index % 2) == 0)
        {
            m_PixelData[offset] = (uint8_t)((m_PixelData[offset] & 0x0F) | (value << 4));
        }
        else
        {
            m_PixelData[offset] = (uint8_t)((m_PixelData[offset] & 0xF0) | (value & 0x0F));
        }
        break;
    case 8:
        *(uint8_t*)&m_PixelData[offset] = (uint8_t)value;
        break;
    case 16:
        *(uint16_t*)&m_PixelData[offset] = _byteswap_ushort((uint16_t)value);
        break;
    case 32:
        *(uint32_t*)&m_PixelData[offset] = _byteswap_ulong(value);
        break;
    }
}

bool CN64Image::GetPaletteColor(size_t index, unsigned int* color)
{
    uint16_t* pColor = PalettePtr(index);

    if (pColor == nullptr)
    {
        *color = 0;
        return false;
    }

    *color = _byteswap_ushort(*pColor);
    return true;
}

bool CN64Image::SetPaletteColor(size_t index, unsigned int color)
{
    uint16_t* pColor = PalettePtr(index);

    if (pColor == nullptr)
    {
        return false;
    }

    *pColor = _byteswap_ushort(color & 0xFFFF);
    return true;
}

bool CN64Image::GetBitmapColor(size_t index, uint32_t* color)
{
    uint32_t* pColor = BitmapPtr(index);
    if (pColor == nullptr)
    {
        *color = 0;
        return false;
    }
    *color = _byteswap_ulong(*pColor);
    return true;
}

bool CN64Image::SetBitmapColor(size_t index, unsigned int color)
{
    uint32_t* pColor = BitmapPtr(index);
    if (pColor == nullptr)
    {
        return false;
    }
    *pColor = _byteswap_ulong(color);
    return true;
}

int CN64Image::UpdateBitmap()
{
    m_BitmapRgba32.resize(m_NumPixels * sizeof(uint32_t));

    for (size_t nPixel = 0; nPixel < m_NumPixels; nPixel++)
    {
        unsigned int color = 0;
        unsigned int texel = GetTexel(nPixel);

        if (m_bUsePalette)
        {
            if (!GetPaletteColor(texel, &color))
            {
                return N64IMG_INVALID_COLOR_INDEX;
            }
        }
        else
        {
            color = texel;
        }

        SetBitmapColor(nPixel, ColorToRgba32(m_Format, color));
    }

    return N64IMG_OK;
}

int CN64Image::UpdatePixelsAndPaletteFromBitmap()
{
    if (m_bUsePalette)
    {
        m_PaletteData.resize(1 << m_PixelSize);

        std::vector<uint16_t> newPalette;
        std::map<uint16_t, size_t> colorIndexMap;
        std::vector<size_t> indices;

        for (size_t i = 0; i < m_NumPixels; i++)
        {
            uint32_t colorRgba32;
            GetBitmapColor(i, &colorRgba32);

            uint16_t color16 = (uint16_t)ColorFromRgba32(m_Format, colorRgba32);

            if (colorIndexMap.count(color16) > 0)
            {
                indices.push_back(colorIndexMap[color16]);
            }
            else
            {
                if (newPalette.size() > (size_t)(1 << m_PixelSize))
                {
                    return N64IMG_TOO_MANY_COLORS;
                }

                colorIndexMap[color16] = newPalette.size();
                indices.push_back(newPalette.size());
                newPalette.push_back(color16);
            }
        }

        for (size_t nPixel = 0; nPixel < indices.size(); nPixel++)
        {
            SetTexel(nPixel, indices[nPixel]);
        }

        m_PaletteData.resize(newPalette.size());

        for (size_t nColor = 0; nColor < newPalette.size(); nColor++)
        {
            SetPaletteColor(nColor, newPalette[nColor]);
        }
    }
    else
    {
        for (size_t nPixel = 0; nPixel < m_NumPixels; nPixel++)
        {
            uint32_t colorRgba32;
            GetBitmapColor(nPixel, &colorRgba32);
            SetTexel(nPixel, ColorFromRgba32(m_Format, colorRgba32));
        }
    }

    return N64IMG_OK;
}

std::vector<uint8_t>& CN64Image::PaletteData()
{
    return m_PaletteData;
}

std::vector<uint8_t>& CN64Image::PixelData()
{
    return m_PixelData;
}

std::vector<uint8_t>& CN64Image::Bitmap()
{
    return m_BitmapRgba32;
}

size_t CN64Image::Width()
{
    return m_Width;
}

size_t CN64Image::Height()
{
    return m_Height;
}

int CN64Image::Format()
{
    return m_Format;
}

bool CN64Image::UsesPalette()
{
    return UsesPalette(m_Format);
}

unsigned int CN64Image::ColorFromRgba32(int dstFormat, uint32_t rgba32)
{
    if (dstFormat == IMG_RGBA32)
    {
        return rgba32;
    }

    uint8_t r = ((rgba32 >> 24) & 0xFF);
    uint8_t g = ((rgba32 >> 16) & 0xFF);
    uint8_t b = ((rgba32 >>  8) & 0xFF);
    uint8_t a = ((rgba32 >>  0) & 0xFF);

    uint8_t i;

    switch (dstFormat)
    {
    case IMG_RGBA16:
    case IMG_CI8_RGBA16:
    case IMG_CI4_RGBA16:
        return ((r / 8) << 11) | ((g / 8) <<  6) | ((b / 8) <<  1) | (a / 128);
    case IMG_IA16:
    case IMG_CI8_IA16:
    case IMG_CI4_IA16:
        i = (r + g + b) / 3;
        return (i << 8) | a;
    case IMG_I4:
        i = (r + g + b) / 3;
        return (i / 16);
    case IMG_IA4:
        i = (r + g + b) / 3;
        return ((i / 32) << 1) | (a / 128);
    case IMG_I8:
        i = (r + g + b) / 3;
        return i;
    case IMG_IA8:
        i = (r + g + b) / 3;
        return ((i / 16) << 4) | (a / 16);
    }

    return 0;
}

uint32_t CN64Image::ColorToRgba32(int srcFormat, unsigned int color)
{
    uint8_t r = 0, g = 0, b = 0, a = 0;

    switch (srcFormat)
    {
    case IMG_RGBA32:
        return color;
    case IMG_RGBA16:
    case IMG_CI8_RGBA16:
    case IMG_CI4_RGBA16:
        r = (((color >> 11) & 0x1F) * 255) / 31;
        g = (((color >>  6) & 0x1F) * 255) / 31;
        b = (((color >>  1) & 0x1F) * 255) / 31;
        a = (color & 1) * 255;
        break;
    case IMG_IA16:
    case IMG_CI8_IA16:
    case IMG_CI4_IA16:
        r = g = b = (uint8_t)(color >> 8);
        a = (color & 0xFF);
        break;
    case IMG_I4:
        r = g = b = (uint8_t)(color * 17);
        a = 255;
        break;
    case IMG_IA4:
        r = g = b = (uint8_t)(((color >> 1) * 255) / 7);
        a = (color & 1) * 255;
        break;
    case IMG_I8:
        r = g = b = (uint8_t)color;
        a = 255;
        break;
    case IMG_IA8:
        r = g = b = (uint8_t)((color >> 4) * 17);
        a = (color & 0x0F) * 17;
        break;
    }

    return (r << 24) | (g << 16) | (b <<  8) | a;
}

int CN64Image::BitsPerPixel(int format)
{
    if (FormatInfo.count(format))
    {
        return FormatInfo.at(format).bitsPerPixel;
    }
    return 0;
}

int CN64Image::PaletteColorCount(int format)
{
    if (FormatInfo.count(format))
    {
        return FormatInfo.at(format).paletteColorCount;
    }
    return 0;
}

bool CN64Image::UsesPalette(int format)
{
    if (FormatInfo.count(format))
    {
        return FormatInfo.at(format).paletteColorCount > 0;
    }
    return false;
}

const char* CN64Image::ResultCodeName(int resultCode)
{
    static const std::map<int, const char*> names = {
        { N64IMG_OK, "OK" },
        { N64IMG_DATA_SIZE_INCORRECT, "ERR_DATA_SIZE_INCORRECT" },
        { N64IMG_INVALID_COLOR_INDEX, "ERR_INVALID_COLOR_INDEX" },
        { N64IMG_INCOMPATIBLE_COLOR,  "ERR_INCOMPATIBLE_COLOR" },
        { N64IMG_TOO_MANY_COLORS,     "ERR_TOO_MANY_COLORS" },
        { N64IMG_PNG_HEADER_MISSING,  "ERR_PNG_HEADER_MISSING" },
        { N64IMG_PNG_OUT_OF_MEMORY,   "ERR_PNG_OUT_OF_MEMORY" },
        { N64IMG_PNG_EXCEPTION,       "ERR_PNG_EXCEPTION" },
        { N64IMG_PNG_PARSER_FAILED,   "ERR_PNG_PARSER_FAILED" }
    };

    if (names.count(resultCode) != 0)
    {
        return names.at(resultCode);
    }

    return "ERR_UNKNOWN";
}
