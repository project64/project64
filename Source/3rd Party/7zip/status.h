#ifndef __7ZIP_STATUS_H
#define __7ZIP_STATUS_H

enum _7Z_STATUS {
	LZMADECODE_START,
	LZMADECODE_UPDATE,
	LZMADECODE_DONE,
};

typedef
void (__stdcall *LP7ZSTATUS_UPDATE)( 
    enum _7Z_STATUS Status,
    int Value1,
    int Value2,
	void * StatusInfo
    );


#endif