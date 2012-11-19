typedef const char *     LPCSTR;

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <memory.h>
#include "7zip.h"

C7zip::C7zip (LPCSTR FileName) :
	m_FileSize(0),
	m_CurrentFile(-1),
	m_blockIndex(0xFFFFFFFF),
	m_outBuffer(0),
	m_outBufferSize(0),
	m_NotfyCallback(NotfyCallbackDefault),
	m_NotfyCallbackInfo(NULL)
{
	memset(&m_FileName,0,sizeof(m_FileName));
	memset(&m_db,0,sizeof(m_db));

	m_archiveStream.InStream.Read = SzFileReadImp;
	m_archiveStream.InStream.Seek = SzFileSeekImp;

	m_allocImp.Alloc = SzAlloc;
	m_allocImp.Free = SzFree;

	m_allocTempImp.Alloc = SzAllocTemp;
	m_allocTempImp.Free = SzFreeTemp;

	m_archiveStream.File = fopen(FileName, "rb");
	if (m_archiveStream.File == 0)
	{
	//PrintError("can not open input file");
		return;
	}
	fseek(m_archiveStream.File, 0, SEEK_END);
	m_FileSize = ftell(m_archiveStream.File); 
	fseek(m_archiveStream.File, 0, SEEK_SET);

	char drive[_MAX_DRIVE] ,dir[_MAX_DIR], ext[_MAX_EXT];
	_splitpath( FileName, drive, dir, m_FileName, ext );

	InitCrcTable();
	SzArDbExInit(&m_db);
	SZ_RESULT res = SzArchiveOpen(&m_archiveStream.InStream, &m_db, &m_allocImp, &m_allocTempImp);
	res = res;
}

C7zip::~C7zip (void)
{
	SetNotificationCallback(NULL,NULL);
	SzArDbExFree(&m_db, m_allocImp.Free);

	if (m_archiveStream.File)
	{
		fclose(m_archiveStream.File);
	}
	if (m_outBuffer)
	{
		m_allocImp.Free(m_outBuffer);
	}
}

void C7zip::SetNotificationCallback (LP7ZNOTIFICATION NotfyFnc, void * CBInfo)
{
	m_NotfyCallback     = NotfyFnc ? NotfyFnc : NotfyCallbackDefault;
	m_NotfyCallbackInfo = CBInfo;
}

void C7zip::StatusUpdate(_7Z_STATUS status, int Value1, int Value2, C7zip * _this )
{
	CFileItem * File = _this->m_CurrentFile >= 0 ? _this->FileItem(_this->m_CurrentFile) : NULL;
	
	switch (status)
	{
	case LZMADECODE_START: 	_this->m_NotfyCallback("Start decoding",_this->m_NotfyCallbackInfo); break;
	case LZMADECODE_UPDATE: 
		{
			char Msg[200];
			sprintf(Msg,"decoding %s: %0.2f%%",File->Name, (Value1/(float)Value2) * 100);
			_this->m_NotfyCallback(Msg,_this->m_NotfyCallbackInfo); 
		}
		break;
	case LZMADECODE_DONE:  _this->m_NotfyCallback("Finished decoding",_this->m_NotfyCallbackInfo); break;
	}
}

bool C7zip::GetFile(int index, Byte * Data, size_t DataLen )
{
	m_CurrentFile = -1;
	if (Data == NULL || DataLen == 0)
	{
		return false;
	}
	if (m_archiveStream.File == 0)
	{
		return false;
	}
	m_CurrentFile = index;

	SZ_RESULT res;
    size_t offset;
    size_t outSizeProcessed;
	
	char Msg[200];
	CFileItem * f = FileItem(index);
	sprintf(Msg,"Getting %s",f->Name);
	m_NotfyCallback(Msg,m_NotfyCallbackInfo);

	res = SzExtract(&m_archiveStream.InStream, &m_db, index, 
            &m_blockIndex, &m_outBuffer, &m_outBufferSize, 
            &offset, &outSizeProcessed, 
            &m_allocImp, &m_allocTempImp,(LP7ZSTATUS_UPDATE)StatusUpdate, this);
    if (res != SZ_OK)
	{
		m_CurrentFile = -1;
		return false;
	}

	if (DataLen < outSizeProcessed)
	{
		outSizeProcessed = DataLen;
	}
	memcpy(Data,m_outBuffer + offset,outSizeProcessed);
	m_NotfyCallback("",m_NotfyCallbackInfo);
	m_CurrentFile = -1;
	return true;
}

SZ_RESULT C7zip::SzFileReadImp(void *object, void *buffer, size_t size, size_t *processedSize)
{
  CFileInStream *s = (CFileInStream *)object;
  size_t processedSizeLoc = fread(buffer, 1, size, s->File);
  if (processedSize != 0)
    *processedSize = processedSizeLoc;
  return SZ_OK;
}

SZ_RESULT C7zip::SzFileSeekImp(void *object, CFileSize pos)
{
  CFileInStream *s = (CFileInStream *)object;
  int res = fseek(s->File, (long)pos, SEEK_SET);
  if (res == 0)
    return SZ_OK;
  return SZE_FAIL;
}

const char * C7zip::FileName ( char * FileName, int SizeOfFileName ) const
{
	if (FileName == NULL) 
	{
		return NULL;
	}
	int Len = strlen(m_FileName);
	if (Len > (SizeOfFileName - 1))
	{
		Len = (SizeOfFileName - 1);
	}
	strncpy(FileName,m_FileName,Len);
	FileName[Len] = 0;
	return FileName;
}
