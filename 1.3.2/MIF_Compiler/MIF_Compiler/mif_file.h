#ifndef _MIF_FILE_H_
#define _MIF_FILE_H_

#define MIF_SUCCESS		(0)
#define MIF_FAILURE		(-1)

#ifdef __cplusplus
extern "C" {
#endif
	enum MIF_SCAN_EVENT{
		MID_TOKEN,
		MID_END_LINE,
		MID_EOF
	};
	// callback
	typedef int(*mif_callback_t)(int, int, int, const char*);

	int mif_scan_mid(const char*, mif_callback_t);

#ifdef __cplusplus
}
#endif

#endif
