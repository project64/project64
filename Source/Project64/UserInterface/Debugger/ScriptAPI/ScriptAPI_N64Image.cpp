#include "stdafx.h"

#include "N64Image.h"
#include "ScriptAPI.h"

#pragma warning(disable : 4702) // disable unreachable code warning

using namespace ScriptAPI;

static CN64Image * GetThisImage(duk_context * ctx)
{
    duk_push_this(ctx);
    duk_get_prop_string(ctx, -1, HS_n64ImagePtr);
    CN64Image * image = (CN64Image *)duk_get_pointer(ctx, -1);
    duk_pop_n(ctx, 2);

    if (image == nullptr)
    {
        duk_push_error_object(ctx, DUK_ERR_ERROR, "internal image object is null");
        return duk_throw(ctx);
    }

    return image;
}

void ScriptAPI::Define_N64Image(duk_context * ctx)
{
    const DukPropListEntry prototype[] = {
        {"toPNG", DukCFunction(js_N64Image_toPNG)},
        {"update", DukCFunction(js_N64Image_update)},
        {nullptr},
    };

    const DukPropListEntry staticProps[] = {
        {"fromPNG", DukCFunction(js_N64Image_static_fromPNG)},
        {"format", DukCFunction(js_N64Image_static_format)},
        {"bpp", DukCFunction(js_N64Image_static_bpp)},
        {nullptr},
    };

    DefineGlobalClass(ctx, "N64Image", js_N64Image__constructor, prototype, staticProps);
}

static void InitImageObjectProps(duk_context * ctx, duk_idx_t idx, CN64Image * image)
{
    idx = duk_normalize_index(ctx, idx);

    duk_push_external_buffer(ctx);
    duk_config_buffer(ctx, -1, image->PixelData().data(), image->PixelData().size());
    duk_push_buffer_object(ctx, -1, 0, image->PixelData().size(), DUK_BUFOBJ_NODEJS_BUFFER);
    duk_remove(ctx, -2);

    duk_idx_t pixels_idx = duk_normalize_index(ctx, -1);

    if (image->UsesPalette())
    {
        duk_push_external_buffer(ctx);
        duk_config_buffer(ctx, -1, image->PaletteData().data(), image->PaletteData().size());
        duk_push_buffer_object(ctx, -2, 0, image->PaletteData().size(), DUK_BUFOBJ_NODEJS_BUFFER);
        duk_remove(ctx, -2);
    }
    else
    {
        duk_push_null(ctx);
    }

    duk_idx_t palette_idx = duk_normalize_index(ctx, -1);

    const DukPropListEntry props[] = {
        {HS_n64ImagePtr, DukPointer(image)},
        {"pixels", DukDupIndex(pixels_idx)},
        {"palette", DukDupIndex(palette_idx)},
        {"width", DukUInt(image->Width())},
        {"height", DukUInt(image->Height())},
        {nullptr},
    };

    DukPutPropList(ctx, idx, props);

    duk_pop_n(ctx, 2);

    duk_push_c_function(ctx, ScriptAPI::js_N64Image__finalizer, 1);
    duk_set_finalizer(ctx, idx);
}

duk_ret_t ScriptAPI::js_N64Image__constructor(duk_context * ctx)
{
    CheckArgs(ctx, {Arg_Number, Arg_Number, Arg_OptNumber, Arg_OptBufferData, Arg_OptBufferData});

    if (!duk_is_constructor_call(ctx))
    {
        return DUK_RET_ERROR;
    }

    size_t pixelDataSize = 0, paletteDataSize = 0;

    size_t width = duk_get_uint(ctx, 0);
    size_t height = duk_get_uint(ctx, 1);
    int format = duk_get_int_default(ctx, 2, IMG_RGBA32);
    void * pixelData = duk_get_buffer_data_default(ctx, 3, &pixelDataSize, nullptr, 0);
    void * paletteData = duk_get_buffer_data_default(ctx, 4, &paletteDataSize, nullptr, 0);

    CN64Image * image = new CN64Image();
    int result = image->Init(format, width, height, pixelData, pixelDataSize, paletteData, paletteDataSize);

    if (result != N64IMG_OK)
    {
        duk_push_error_object(ctx, DUK_ERR_ERROR, "failed to initialize image (%s)",
                              CN64Image::ResultCodeName(result));
        return duk_throw(ctx);
    }

    duk_push_this(ctx);
    InitImageObjectProps(ctx, -1, image);
    return 0;
}

duk_ret_t ScriptAPI::js_N64Image__finalizer(duk_context * ctx)
{
    duk_get_prop_string(ctx, 0, HS_n64ImagePtr);
    CN64Image * image = (CN64Image *)duk_get_pointer(ctx, -1);
    if (image == nullptr)
    {
        return 0;
    }
    delete image;
    return 0;
}

duk_ret_t ScriptAPI::js_N64Image_static_fromPNG(duk_context * ctx)
{
    CheckArgs(ctx, {Arg_BufferData, Arg_OptNumber});

    int format = duk_get_int_default(ctx, 1, IMG_RGBA32);

    if (CN64Image::BitsPerPixel(format) == 0)
    {
        duk_push_error_object(ctx, DUK_RET_TYPE_ERROR, "invalid format");
        return duk_throw(ctx);
    }

    size_t pngSize;
    uint8_t * pngData = (uint8_t *)duk_get_buffer_data(ctx, 0, &pngSize);

    CN64Image * image = new CN64Image();
    int result = image->Init(format, pngData, pngSize);
    if (result != N64IMG_OK)
    {
        delete image;
        duk_push_error_object(ctx, DUK_ERR_ERROR, "failed to initialize image (%s)",
                              CN64Image::ResultCodeName(result), result);
        return duk_throw(ctx);
    }

    duk_push_object(ctx);
    duk_get_global_string(ctx, "N64Image");
    duk_get_prop_string(ctx, -1, "prototype");
    duk_set_prototype(ctx, -3);
    duk_pop(ctx);

    InitImageObjectProps(ctx, -1, image);
    return 1;
}

duk_ret_t ScriptAPI::js_N64Image_static_format(duk_context * ctx)
{
    CheckArgs(ctx, {Arg_Number, Arg_Number, Arg_OptNumber});

    duk_uint_t gbiFmt = duk_get_uint(ctx, 0);
    duk_uint_t gbiSiz = duk_get_uint(ctx, 1);
    duk_uint_t gbiTlutFmt = duk_get_uint_default(ctx, 2, G_TT_NONE);

    int format = (gbiFmt << 3) | gbiSiz | gbiTlutFmt;

    switch (format)
    {
    case IMG_RGBA16:
    case IMG_RGBA32:
    case IMG_CI4_RGBA16:
    case IMG_CI4_IA16:
    case IMG_CI8_RGBA16:
    case IMG_CI8_IA16:
    case IMG_IA4:
    case IMG_IA8:
    case IMG_IA16:
    case IMG_I4:
    case IMG_I8:
        duk_push_number(ctx, format);
        break;
    default:
        duk_push_number(ctx, -1);
    }

    return 1;
}

duk_ret_t ScriptAPI::js_N64Image_static_bpp(duk_context * ctx)
{
    CheckArgs(ctx, {Arg_Number});

    duk_uint_t format = duk_get_uint(ctx, 0);

    int bpp = 0;

    switch (format)
    {
    case G_IM_SIZ_4b: bpp = 4; break;
    case G_IM_SIZ_8b: bpp = 8; break;
    case G_IM_SIZ_16b: bpp = 16; break;
    case G_IM_SIZ_32b: bpp = 32; break;
    default: bpp = CN64Image::BitsPerPixel(format); break;
    }

    duk_push_number(ctx, bpp);
    return 1;
}

duk_ret_t ScriptAPI::js_N64Image_toPNG(duk_context * ctx)
{
    CN64Image * image = GetThisImage(ctx);

    std::vector<uint8_t> png;
    image->ToPNG(png);

    void * pngCopy = duk_push_buffer(ctx, png.size(), false);
    duk_push_buffer_object(ctx, -1, 0, png.size(), DUK_BUFOBJ_NODEJS_BUFFER);
    memcpy(pngCopy, png.data(), png.size());

    return 1;
}

duk_ret_t ScriptAPI::js_N64Image_update(duk_context * ctx)
{
    CN64Image * image = GetThisImage(ctx);
    int result = image->UpdateBitmap();
    if (result != N64IMG_OK)
    {
        duk_push_error_object(ctx, DUK_ERR_ERROR, "bitmap update failed (%s)",
                              CN64Image::ResultCodeName(result));
    }
    return 0;
}
