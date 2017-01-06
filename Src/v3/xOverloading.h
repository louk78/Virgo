#include "tchar.h"


class UNICODE_STRING_2
{
public:
	UNICODE_STRING_2(WCHAR  *lpBuffer);
	operator	UNICODE_STRING*();
	operator	WCHAR*();

	UNICODE_STRING		Operator;
};

#define wsize(s)					(sizeof(s) - 2)
#define csize(s)					(sizeof(s) - 1)

BOOLEAN		RtlEqualUnicodeString		(IN  PCUNICODE_STRING String1,  IN  UNICODE_STRING_2	String2,	IN  USHORT nSize, BOOLEAN	CaseInSensitive	=	TRUE);
BOOLEAN		RtlPrefixUnicodeString		(IN  PCUNICODE_STRING String1,  IN  UNICODE_STRING_2	String2,	IN  USHORT nSize, BOOLEAN	CaseInSensitive	=	TRUE);
VOID		RstlAppendUnicodeToString	(IN  PUNICODE_STRING  String1,	IN  PCWSTR				String2,	IN  PCWSTR String3	=	NULL, PCWSTR String4	=	NULL);

USHORT		ho_memcmp(const void *s1, const void *s2, USHORT n1, USHORT n2);
USHORT		hi_memcmp(const void *s1, const void *s2, USHORT n1);

#pragma once

/************************************************************************************************************************************
		 
	基于面向对象编写!!!

************************************************************************************************************************************/
UNICODE_STRING_2::UNICODE_STRING_2(WCHAR  *lpBuffer)
{
	Operator.Buffer		 = lpBuffer;
/*
	while (*lpBuffer++ != L'\0'){}

	Operator.MaximumLength	= (USHORT)((char*)lpBuffer - (char*)Operator.Buffer);
	Operator.Length			= Operator.MaximumLength - 2;
*/
}

UNICODE_STRING_2::operator	UNICODE_STRING*()
{
	return	&this->Operator;
}

UNICODE_STRING_2::operator	WCHAR*()
{
	return	this->Operator.Buffer;
}


BOOLEAN 
RtlEqualUnicodeString ( PCUNICODE_STRING String1, UNICODE_STRING_2 String2, USHORT nSize, BOOLEAN	CaseInSensitive )
{
	String2.Operator.Length	=	nSize;
	String2.Operator.MaximumLength	=	nSize	+	2;

	return	RtlEqualUnicodeString(String1, (PUNICODE_STRING)String2, CaseInSensitive);
}

BOOLEAN 
RtlPrefixUnicodeString ( PCUNICODE_STRING String1, UNICODE_STRING_2 String2, USHORT	nSize, BOOLEAN	CaseInSensitive )
{
	String2.Operator.Length	=	nSize;
	String2.Operator.MaximumLength	=	nSize	+	2;

	return	RtlPrefixUnicodeString((PUNICODE_STRING)String2, String1, CaseInSensitive);
}

VOID
RtlAppendUnicodeToString ( PUNICODE_STRING String1, PCWSTR String2, PCWSTR String3, PCWSTR String4 )
{
	RtlAppendUnicodeToString(String1, String2);

	if (String3)

		RtlAppendUnicodeToString(String1, String3, String4, NULL);
}



/*	逆向比较	(无视大小写)	*/
USHORT ho_memcmp(const void *s1, const void *s2, unsigned	short n1, unsigned	short n2)
{
	unsigned	char	*p1, *p2;
	unsigned	short	m,	n = min(n1, n2);
	
	
	p1	=	(unsigned char *)s1 + n1;
    p2	=	(unsigned char *)s2 + n2;
	m	=	n;

	do 
	{
		if ( !n )	break;
		n--;
		
		register	char	al, bl, cl;

		al	=	*--p1;
		bl	=	*--p2;
		cl	=	al	^ bl;

		if ( !cl )	continue;

		if (al >= 'a' && al <= 'z' || bl >= 'a' && bl <= 'z')
		
			if ( !(cl ^ 0x20) )	continue;
		
		n++;	
	} while(0);
 

    return	m - n;
}

/*	正向比较	(无视大小写)	*/
USHORT hi_memcmp(const void *s1, const void *s2, unsigned	short n1)
{
	unsigned	char	*p1, *p2;
	unsigned	short	m,	n = n1;


	p1	=	(unsigned char *)s1;
    p2	=	(unsigned char *)s2;
	m	=	n;

	do 
	{
		if ( !n )	break;
		n--;
		
		register	char	al, bl, cl;

		al	=	*p1++;
		bl	=	*p2++;
		cl	=	al	^ bl;

		if ( !cl )	continue;

		if (al >= 'a' && al <= 'z' || bl >= 'a' && bl <= 'z')
		
			if ( !(cl ^ 0x20) )	continue;

		n++;
	} while(0);
 

    return	m - n;
}
