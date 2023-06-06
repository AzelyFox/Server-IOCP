#define _SIMPLE_HEADER

#define NETWORK_HEADER_SIZE sizeof(NetworkHeader)
#define NETWORK_SECURE_CODE 0x89

namespace azely
{
	#pragma pack(push, 1)
	struct NetworkHeader
	{
#ifdef _SIMPLE_HEADER
		BYTE secureCode;
		BYTE length;
#else
		unsigned char secureCode;
		unsigned short length;
		unsigned char checksum;
#endif
	};
	#pragma pack(pop)




}