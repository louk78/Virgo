NTSTATUS	XOR_Encode_Decode		(IN PVOID lpData, IN PVOID	opData, IN ULONG length, IN PVOID key, IN __int64 offset);

/************************************************************************************************************************************
		 
	XOR/AES-128	º”√‹À„∑®

************************************************************************************************************************************/
NTSTATUS
XOR_Encode_Decode(PVOID	lpData, PVOID	opData, ULONG	Length, PVOID	KEY, __int64	Offset)
{
	NTSTATUS	ntStatus;
	unsigned	char*		&lpData1	=	(unsigned char *&)lpData;		unsigned	char*		&opData1	=	(unsigned char *&)opData;
	unsigned	__int64*	&lpData2	=	(unsigned __int64 *&)lpData;	unsigned	__int64*	&opData2	=	(unsigned __int64 *&)opData;
	unsigned	char*		&pass1		=	(unsigned char *&)KEY;			unsigned	__int64*	&pass2		=	(unsigned __int64 *&)KEY;


	pass1	+=	Offset % 16;

	__try
	{
		for (unsigned int	i = 0; i < Length / sizeof(__int64); i++)
		{
			*opData2++ = *lpData2++ ^ (i&0x1 ? *pass2-- : *pass2++);
		}

		for (unsigned int	i = 0; i < Length % sizeof(__int64); i++)
		{
			*opData1++ = *lpData1++ ^ pass1 [i];
		}

		ntStatus	=	STATUS_SUCCESS;
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		ntStatus	=	STATUS_UNSUCCESSFUL;
	}

	return	ntStatus;
}