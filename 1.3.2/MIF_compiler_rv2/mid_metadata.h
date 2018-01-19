#ifndef _MID_META_DATA_H_
#define _MID_META_DATA_H_
#ifdef __cplusplus
extern "C" {
#endif
	// fundamental mid type
	typedef unsigned char mid_char1_val_t; // max = 9, => 1 byte
	typedef unsigned char mid_char2_val_t; // max = 99 => 1 byte
	typedef unsigned short mid_char3_val_t; // max = 999 => 2 bytes
	typedef unsigned long mid_char8_val_t; // max = 99,999,999 => 4 bytes
	typedef unsigned long long mid_char13_val_t; // max = 9,999,999,999,999 => 8 bytes
	
	// all data header
	typedef struct
	{
		size_t province_num;
	}MID_AllDataHeader_t;

	// province date header
	typedef struct
	{
		size_t CrossTransRecNum;
		size_t CrossRestriRecNum;
		
	}MID_ProviceHeader_t;
    
    typedef struct
    {
        MID_AllDataHeader_t header;
    }MID_AllData_t;
#ifdef __cplusplus
}
#endif

#endif