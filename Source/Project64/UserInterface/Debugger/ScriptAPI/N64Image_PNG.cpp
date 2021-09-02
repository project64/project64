#include <stdafx.h>
#include <3rdParty/png/png.h>
#include "N64Image.h"

#pragma warning (disable:4611) // disable setjmp/c++ deconstruction warning

struct PNGReadState {
    uint8_t*   pngData;
    size_t     pngSize;
    png_size_t offset;
};

static void PNGReadCallback(png_structp png_ptr, png_bytep data, png_size_t length);
static void PNGWriteCallback(png_structp png_ptr, png_bytep data, png_size_t length);
static void PNGFlushCallback(png_structp png_ptr);
static bool ParsePNGRow(png_byte* row, png_size_t rowSize, int bitDepth, int colorType, std::vector<uint8_t>& outRGBA32);

int CN64Image::ReadPNG(uint8_t* pngData, size_t pngSize, size_t* outWidth, size_t* outHeight, std::vector<uint8_t>& outRGBA32)
{
    if (!png_check_sig(pngData, 8))
    {
        return N64IMG_PNG_HEADER_MISSING;
    }

    png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);

    if (!png_ptr)
    {
        return N64IMG_PNG_OUT_OF_MEMORY;
    }

    png_infop info_ptr = png_create_info_struct(png_ptr);

    if (!info_ptr)
    {
        png_destroy_read_struct(&png_ptr, nullptr, nullptr);
        return N64IMG_PNG_OUT_OF_MEMORY;
    }

    if (setjmp(png_jmpbuf(png_ptr)))
    {
        g_Notify->BreakPoint(__FILE__, __LINE__);

        png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
        return N64IMG_PNG_EXCEPTION;
    }

    PNGReadState readState;
    readState.pngData = pngData;
    readState.pngSize = pngSize;
    readState.offset = 8;

    png_set_read_fn(png_ptr, &readState, PNGReadCallback);
    png_set_sig_bytes(png_ptr, 8);
    png_read_info(png_ptr, info_ptr);

    png_uint_32 width, height;
    int bitDepth, colorType;
    png_get_IHDR(png_ptr, info_ptr, &width, &height, &bitDepth,
        &colorType, nullptr, nullptr, nullptr);

    png_size_t rowSize = png_get_rowbytes(png_ptr, info_ptr);
    std::vector<png_bytep> rowPointers(height);
    std::vector<png_byte> imageData(height * rowSize);

    for (size_t nRow = 0; nRow < height; nRow++)
    {
        rowPointers[nRow] = &imageData[nRow * rowSize];
    }

    png_read_image(png_ptr, &rowPointers[0]);

    for (size_t nRow = 0; nRow < height; nRow++)
    {
        if (!ParsePNGRow(rowPointers[nRow], rowSize, bitDepth, colorType, outRGBA32))
        {
            png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
            return N64IMG_PNG_PARSER_FAILED;
        }
    }

    png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);

    *outWidth = width;
    *outHeight = height;
    return N64IMG_OK;
}

void CN64Image::WritePNG(uint8_t* rgba32, size_t width, size_t height, std::vector<uint8_t>& buffer)
{
    png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);

    if (!png_ptr)
    {
        png_destroy_write_struct(&png_ptr, nullptr);
        return;
    }

    png_infop info_ptr = png_create_info_struct(png_ptr);

    if (!info_ptr)
    {
        png_destroy_write_struct(&png_ptr, nullptr);
        return;
    }

    if (setjmp(png_jmpbuf(png_ptr)))
    {
        png_destroy_write_struct(&png_ptr, &info_ptr);
        return;
    }

    png_set_IHDR(png_ptr, info_ptr, width, height,
        8, PNG_COLOR_TYPE_RGBA, PNG_INTERLACE_NONE,
        PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

    png_set_write_fn(png_ptr, &buffer, PNGWriteCallback, PNGFlushCallback);
    png_write_info(png_ptr, info_ptr);

    size_t rowSize = width * 4;

    std::vector<png_bytep> rowPointers(height);
    for (size_t nRow = 0; nRow < height; nRow++)
    {
        rowPointers[nRow] = &rgba32[nRow * rowSize];
    }

    png_write_image(png_ptr, &rowPointers[0]);
    png_write_end(png_ptr, info_ptr);
    png_destroy_write_struct(&png_ptr, &info_ptr);
}

static void PNGReadCallback(png_structp png_ptr, png_bytep data, png_size_t length)
{
    PNGReadState* state = (PNGReadState*)png_get_io_ptr(png_ptr);
    if (state->offset + length > state->pngSize)
    {
        return;
    }
    memcpy(data, &state->pngData[state->offset], length);
    state->offset += length;
}

static void PNGWriteCallback(png_structp png_ptr, png_bytep data, png_size_t length)
{
    std::vector<uint8_t>* buffer = (std::vector<uint8_t>*)png_get_io_ptr(png_ptr);
    buffer->insert(buffer->end(), &data[0], &data[length]);
}

static void PNGFlushCallback(png_structp /*png_ptr*/)
{
}

static bool ParsePNGRow(png_byte* row, png_size_t rowSize, int bitDepth, int colorType, std::vector<uint8_t>& outRGBA32)
{
    if (colorType == PNG_COLOR_TYPE_RGBA)
    {
        if (bitDepth == 8)
        {
            outRGBA32.insert(outRGBA32.end(), &row[0], &row[rowSize]);
            return true;
        }
        if (bitDepth == 16)
        {
            for (png_size_t i = 0; i < rowSize; i += 8)
            {
                outRGBA32.push_back(png_get_uint_16(&row[i + 0]) >> 8);
                outRGBA32.push_back(png_get_uint_16(&row[i + 2]) >> 8);
                outRGBA32.push_back(png_get_uint_16(&row[i + 4]) >> 8);
                outRGBA32.push_back(png_get_uint_16(&row[i + 6]) >> 8);
            }
            return true;
        }
    }

    if (colorType == PNG_COLOR_TYPE_RGB)
    {
        if (bitDepth == 8)
        {
            for (png_size_t i = 0; i < rowSize; i += 3)
            {
                outRGBA32.insert(outRGBA32.end(), &row[i], &row[i + 3]);
                outRGBA32.push_back(255);
            }
            return true;
        }
        if (bitDepth == 16)
        {
            for (png_size_t i = 0; i < rowSize; i += 6)
            {
                outRGBA32.push_back(png_get_uint_16(&row[i + 0]) >> 8);
                outRGBA32.push_back(png_get_uint_16(&row[i + 2]) >> 8);
                outRGBA32.push_back(png_get_uint_16(&row[i + 4]) >> 8);
                outRGBA32.push_back(255);
            }
            return true;
        }
    }

    return false;
}
