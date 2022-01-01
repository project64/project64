#include <stdafx.h>
#include "ScriptAPI.h"
#include <sys/stat.h>

#pragma warning(disable: 4702) // disable unreachable code warning

enum fsop { FS_READ, FS_WRITE };
static duk_ret_t ReadWriteImpl(duk_context *ctx, fsop op); // (fd, buffer, offset, length, position)
static duk_ret_t js__FileFinalizer(duk_context *ctx);

void ScriptAPI::Define_fs(duk_context *ctx)
{
    // todo DukPutProps DukClass

    const duk_function_list_entry funcs[] = {
        { "open",      js_fs_open,      DUK_VARARGS },
        { "close",     js_fs_close,     DUK_VARARGS },
        { "write",     js_fs_write,     DUK_VARARGS },
        { "writefile", js_fs_writefile, DUK_VARARGS },
        { "read",      js_fs_read,      DUK_VARARGS },
        { "readfile",  js_fs_readfile,  DUK_VARARGS },
        { "exists",    js_fs_exists,    DUK_VARARGS },
        { "fstat",     js_fs_fstat,     DUK_VARARGS },
        { "stat",      js_fs_stat,      DUK_VARARGS },
        { "unlink",    js_fs_unlink,    DUK_VARARGS },
        { "mkdir",     js_fs_mkdir,     DUK_VARARGS },
        { "rmdir",     js_fs_rmdir,     DUK_VARARGS },
        { "readdir",   js_fs_readdir,   DUK_VARARGS },
        { "Stats", js_fs_Stats__constructor, DUK_VARARGS },
        { nullptr, nullptr, 0 }
    };

    const duk_function_list_entry Stats_funcs[] = {
        { "isFile",      js_fs_Stats_isFile,      DUK_VARARGS },
        { "isDirectory", js_fs_Stats_isDirectory, DUK_VARARGS },
        { nullptr, nullptr, 0 }
    };

    duk_push_global_object(ctx);
    duk_push_string(ctx, "fs");
    duk_push_object(ctx);
    duk_put_function_list(ctx, -1, funcs);

    duk_get_prop_string(ctx, -1, "Stats");
    duk_push_object(ctx);
    duk_put_function_list(ctx, -1, Stats_funcs);
    duk_put_prop_string(ctx, -2, "prototype");
    duk_pop(ctx);

    duk_freeze(ctx, -1);
    duk_def_prop(ctx, -3, DUK_DEFPROP_HAVE_VALUE | DUK_DEFPROP_SET_ENUMERABLE);
    duk_pop(ctx);
}

duk_ret_t ScriptAPI::js_fs_open(duk_context *ctx)
{
    CheckArgs(ctx, { Arg_String, Arg_String });

    const char* path = duk_get_string(ctx, 0);
    const char* mode = duk_get_string(ctx, 1);

    bool bModeValid = false;

    const char* validModes[] = {
        "r", "rb",
        "w", "wb",
        "a", "ab",
        "r+", "rb+", "r+b",
        "w+", "wb+", "w+b",
        "a+", "ab+", "a+b",
        nullptr
    };

    for (int i = 0; validModes[i] != nullptr; i++)
    {
        if (strcmp(mode, validModes[i]) == 0)
        {
            bModeValid = true;
            break;
        }
    }

    if (!bModeValid)
    {
        duk_push_error_object(ctx, DUK_ERR_TYPE_ERROR, "mode '%s' is not valid", mode);
        return duk_throw(ctx);
    }

    FILE *fp = fopen(path, mode);

    if(fp == nullptr)
    {
        duk_push_error_object(ctx, DUK_ERR_ERROR, "could not open '%s' (mode: '%s')", path, mode);
        return duk_throw(ctx);
    }

    int fd = _fileno(fp);

    //  FILES[fd] = {fp: fp}
    duk_get_global_string(ctx, HS_gOpenFileDescriptors);

    duk_push_object(ctx);
    duk_push_pointer(ctx, fp);
    duk_put_prop_string(ctx, -2, "fp");
    duk_push_c_function(ctx, js__FileFinalizer, 1);
    duk_set_finalizer(ctx, -2);

    duk_put_prop_index(ctx, -2, fd);
    duk_pop_n(ctx, 2);

    duk_push_number(ctx, fd);

    return 1;
}

duk_ret_t ScriptAPI::js_fs_close(duk_context *ctx)
{
    CheckArgs(ctx, { Arg_Number });

    int fd = duk_get_int(ctx, 0);
    int rc = -1;

    duk_get_global_string(ctx, HS_gOpenFileDescriptors);

    if(duk_has_prop_index(ctx, -1, fd))
    {
        duk_get_prop_index(ctx, -1, fd);
        duk_get_prop_string(ctx, -1, "fp");

        FILE* fp = (FILE*)duk_get_pointer(ctx, -1);
        rc = fclose(fp);

        // unset finalizer before deleting
        duk_push_undefined(ctx);
        duk_set_finalizer(ctx, -3); 
        duk_del_prop_index(ctx, -3, fd);

        duk_pop_n(ctx, 2);
    }
    else
    {
        duk_push_error_object(ctx, DUK_ERR_ERROR, "invalid file descriptor");
        return duk_throw(ctx);
    }

    duk_pop(ctx);
    duk_push_number(ctx, rc);
    return 1;
}

duk_ret_t ScriptAPI::js_fs_write(duk_context *ctx)
{
    CheckArgs(ctx, { Arg_Number, Arg_Any, Arg_OptNumber, Arg_OptNumber, Arg_OptNumber });
    return ReadWriteImpl(ctx, FS_WRITE);
}

duk_ret_t ScriptAPI::js_fs_writefile(duk_context *ctx)
{
    CheckArgs(ctx, { Arg_String, Arg_Any });

    const char* path = duk_get_string(ctx, 0);

    void* buffer;
    duk_size_t bufferSize;

    if(duk_is_string(ctx, 1))
    {
        buffer = (void*)duk_get_lstring(ctx, 1, &bufferSize);
    }
    else if(duk_is_buffer_data(ctx, 1))
    {
        buffer = duk_get_buffer_data(ctx, 1, &bufferSize);
    }
    else
    {
        return ThrowInvalidArgsError(ctx);
    }

    FILE* fp = fopen(path, "wb");

    if(fp == nullptr)
    {
        duk_push_error_object(ctx, DUK_ERR_ERROR, "could not open '%s' (mode: 'wb')", path);
        return duk_throw(ctx);
    }

    if(fwrite(buffer, 1, bufferSize, fp) != bufferSize)
    {
        fclose(fp);
        return DUK_RET_ERROR;
    }

    fclose(fp);
    return 0;
}

duk_ret_t ScriptAPI::js_fs_read(duk_context *ctx)
{
    CheckArgs(ctx, { Arg_Number, Arg_Any, Arg_Number, Arg_Number, Arg_Number });
    return ReadWriteImpl(ctx, FS_READ);
}

duk_ret_t ScriptAPI::js_fs_readfile(duk_context *ctx)
{
    CheckArgs(ctx, { Arg_String });

    const char* path = duk_get_string(ctx, 0);

    FILE* fp = fopen(path, "rb");

    if(fp == nullptr)
    {
        duk_push_error_object(ctx, DUK_ERR_ERROR, "could not open '%s' (mode: 'rb')", path);
        return duk_throw(ctx);
    }

    struct stat stats;
    if(fstat(_fileno(fp), &stats) != 0)
    {
        fclose(fp);
        return DUK_RET_ERROR;
    }

    void *data = duk_push_fixed_buffer(ctx, stats.st_size);
    duk_push_buffer_object(ctx, -1, 0, stats.st_size, DUK_BUFOBJ_NODEJS_BUFFER);

    if(fread(data, 1, stats.st_size, fp) != (size_t)stats.st_size)
    {
        duk_pop(ctx);
        fclose(fp);
        return DUK_RET_ERROR;
    }

    fclose(fp);
    return 1;
}

duk_ret_t ScriptAPI::js_fs_exists(duk_context* ctx)
{
    CheckArgs(ctx, { Arg_String });
    const char* path = duk_get_string(ctx, 0);
    duk_push_boolean(ctx, PathFileExistsA(path) ? 1 : 0);
    return 1;
}

duk_ret_t ScriptAPI::js_fs_fstat(duk_context *ctx)
{
    CheckArgs(ctx, { Arg_Number });

    int fd = duk_get_int(ctx, 0);

    duk_push_global_object(ctx);
    duk_get_prop_string(ctx, -1, "fs");
    duk_get_prop_string(ctx, -1, "Stats");
    duk_push_number(ctx, fd);
    duk_new(ctx, 1);
    return 1;
}

duk_ret_t ScriptAPI::js_fs_stat(duk_context *ctx)
{
    CheckArgs(ctx, { Arg_String });

    const char *path = duk_get_string(ctx, 0);

    duk_push_global_object(ctx);
    duk_get_prop_string(ctx, -1, "fs");
    duk_get_prop_string(ctx, -1, "Stats");
    duk_push_string(ctx, path);
    duk_new(ctx, 1);
    return 1;
}

duk_ret_t ScriptAPI::js_fs_unlink(duk_context *ctx)
{
    CheckArgs(ctx, { Arg_String });

    const char *path = duk_get_string(ctx, 0);
    duk_push_boolean(ctx, DeleteFileA(path) != 0);
    return 1;
}

duk_ret_t ScriptAPI::js_fs_mkdir(duk_context *ctx)
{
    CheckArgs(ctx, { Arg_String });

    const char *path = duk_get_string(ctx, 0);
    duk_push_boolean(ctx, CreateDirectoryA(path, nullptr) != 0);
    return 1;
}

duk_ret_t ScriptAPI::js_fs_rmdir(duk_context *ctx)
{
    CheckArgs(ctx, { Arg_String });

    const char *path = duk_get_string(ctx, 0);
    duk_push_boolean(ctx, RemoveDirectoryA(path) != 0);
    return 1;
}

// todo make sure failure behavior is similar to nodejs's fs.readdirSync
duk_ret_t ScriptAPI::js_fs_readdir(duk_context *ctx)
{
    CheckArgs(ctx, { Arg_String });

    const char* path = duk_get_string(ctx, 0);

    char findFileName[MAX_PATH];
    snprintf(findFileName, sizeof(findFileName), "%s%s", path, "\\*");

    WIN32_FIND_DATAA ffd;
    HANDLE hFind = FindFirstFileA(findFileName, &ffd);

    if(hFind == INVALID_HANDLE_VALUE)
    {
        return DUK_RET_ERROR;
    }

    duk_uarridx_t idx = 0;
    duk_push_array(ctx);

    do
    {
        if(strcmp(ffd.cFileName, ".") == 0 ||
           strcmp(ffd.cFileName, "..") == 0)
        {
            continue;
        }
        duk_push_string(ctx, ffd.cFileName);
        duk_put_prop_index(ctx, -2, idx++);
    } while(FindNextFileA(hFind, &ffd));
    
    FindClose(hFind);
    return 1;
}

duk_ret_t ScriptAPI::js_fs_Stats__constructor(duk_context *ctx)
{
    if(!duk_is_constructor_call(ctx))
    {
        return DUK_RET_TYPE_ERROR;
    }

    if(duk_get_top(ctx) != 1)
    {
        return ThrowInvalidArgsError(ctx);
    }

    struct stat stats;
    
    if(duk_is_number(ctx, 0))
    {
        int fd = duk_get_int(ctx, 0);
        if(fstat(fd, &stats) != 0)
        {
            return DUK_RET_ERROR;
        }
    }
    else if(duk_is_string(ctx, 0))
    {
        const char *path = duk_get_string(ctx, 0);
        if(stat(path, &stats) != 0)
        {
            return DUK_RET_ERROR;
        }
    }
    else
    {
        return ThrowInvalidArgsError(ctx);
    }

    const duk_number_list_entry numbers[] = {
        { "dev",     (double)stats.st_dev },
        { "ino",     (double)stats.st_ino },
        { "mode",    (double)stats.st_mode },
        { "nlink",   (double)stats.st_nlink },
        { "uid",     (double)stats.st_uid },
        { "gid",     (double)stats.st_gid },
        { "rdev",    (double)stats.st_rdev },
        { "size",    (double)stats.st_size },
        { "atimeMs", (double)stats.st_atime * 1000 },
        { "mtimeMs", (double)stats.st_mtime * 1000 },
        { "ctimeMs", (double)stats.st_ctime * 1000 },
        { nullptr, 0 }
    };

    struct { const char *key; time_t time; } dates[3] = {
        { "atime", stats.st_atime * 1000 },
        { "mtime", stats.st_mtime * 1000 },
        { "ctime", stats.st_ctime * 1000 },
    };

    duk_push_global_object(ctx);
    duk_get_prop_string(ctx, -1, "Date");
    duk_remove(ctx, -2);

    duk_push_this(ctx);
    duk_put_number_list(ctx, -1, numbers);

    for(int i = 0; i < 3; i++)
    {
        duk_dup(ctx, -2);
        duk_push_number(ctx, (duk_double_t)dates[i].time);
        duk_new(ctx, 1);
        duk_put_prop_string(ctx, -2, dates[i].key);
    }

    duk_remove(ctx, -2);
    duk_freeze(ctx, -1);
    return 0;
}

duk_ret_t ScriptAPI::js_fs_Stats_isDirectory(duk_context *ctx)
{
    CheckArgs(ctx, {});

    duk_push_this(ctx);
    duk_get_prop_string(ctx, -1, "mode");
    duk_uint_t mode = duk_get_uint(ctx, -1);
    duk_push_boolean(ctx, (mode & S_IFDIR) != 0);
    return 1;
}

duk_ret_t ScriptAPI::js_fs_Stats_isFile(duk_context *ctx)
{
    CheckArgs(ctx, {});

    duk_push_this(ctx);
    duk_get_prop_string(ctx, -1, "mode");
    duk_uint_t mode = duk_get_uint(ctx, -1);
    duk_push_boolean(ctx, (mode & S_IFREG) != 0);
    return 1;
}

static duk_ret_t ReadWriteImpl(duk_context *ctx, fsop op)
{
    int fd;
    const char* buffer;
    size_t offset = 0;
    size_t length = 0;
    size_t position = 0;

    bool bHavePos = false;
    duk_size_t bufferSize;
    FILE* fp;
    size_t rc;

    duk_idx_t nargs = duk_get_top(ctx);

    if(nargs < 2 || !duk_is_number(ctx, 0))
    {
        goto err_invalid_args;
    }

    fd = duk_get_int(ctx, 0);

    if(duk_is_buffer_data(ctx, 1))
    {
        buffer = (const char*)duk_get_buffer_data(ctx, 1, &bufferSize);
    }
    else if(duk_is_string(ctx, 1) && op == FS_WRITE)
    {
        buffer = duk_get_lstring(ctx, 1, &bufferSize);
    }
    else
    {
        goto err_invalid_args;
    }

    if(nargs >= 3)
    {
        if(!duk_is_number(ctx, 2))
        {
            goto err_invalid_args;
        }

        offset = duk_get_uint(ctx, 2);

        if(offset >= bufferSize)
        {
            goto err_invalid_range;
        }
    }

    length = bufferSize - offset;

    if(nargs >= 4)
    {
        if(!duk_is_number(ctx, 3))
        {
            goto err_invalid_args;
        }
        length = duk_get_uint(ctx, 3);
    }

    if(nargs >= 5)
    {
        if(!duk_is_number(ctx, 4))
        {
            goto err_invalid_args;
        }
        position = duk_get_uint(ctx, 4);
        bHavePos = true;
    }

    if(offset + length > bufferSize)
    {
        goto err_invalid_range;
    }

    duk_get_global_string(ctx, HS_gOpenFileDescriptors);

    if(!duk_has_prop_index(ctx, -1, fd))
    {
        goto err_invalid_fd;
    }

    duk_get_prop_index(ctx, -1, fd);
    duk_get_prop_string(ctx, -1, "fp");
    fp = (FILE*)duk_get_pointer(ctx, -1);
    
    duk_pop_n(ctx, 3);

    if(bHavePos)
    {
        fseek(fp, position, SEEK_SET);
    }

    switch(op)
    {
    case FS_READ:
        rc = fread((void*)&buffer[offset], 1, length, fp);
        break;
    case FS_WRITE:
        rc = fwrite((void*)&buffer[offset], 1, length, fp);
        break;
    default:
        return DUK_RET_ERROR;
    }

    duk_push_number(ctx, rc);
    return 1;

err_invalid_args:
    return ScriptAPI::ThrowInvalidArgsError(ctx);

err_invalid_range:
    duk_push_error_object(ctx, DUK_ERR_RANGE_ERROR, "invalid range");
    return duk_throw(ctx);

err_invalid_fd:
    duk_push_error_object(ctx, DUK_ERR_ERROR, "invalid file descriptor");
    return duk_throw(ctx);
}

static duk_ret_t js__FileFinalizer(duk_context *ctx)
{
    CScriptInstance* inst = ScriptAPI::GetInstance(ctx);

    duk_get_prop_string(ctx, 0, "fp");
    FILE *fp = (FILE *)duk_get_pointer(ctx, -1);
    duk_pop(ctx);
    fclose(fp);

    inst->System()->ConsoleLog("[SCRIPTSYS]: warning ('%s'): gc closed leftover file descriptor",
        inst->Name().c_str());

    return 0;
}
