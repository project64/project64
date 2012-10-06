#ifndef _MEMPAK_H_
#define _MEMPAK_H_

class Mempak
{
public:
	static void Close		 ( void );
	static BYTE CalculateCrc ( BYTE * DataToCrc );
	static void ReadFrom	 ( int Control, int Address, BYTE * Buffer );
	static void WriteTo		 ( int Control, int Address, BYTE * Buffer );

};
#endif //_MEMPAK_H_