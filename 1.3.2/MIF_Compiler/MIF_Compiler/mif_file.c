
#ifdef __cplusplus

#endif
#include <stdio.h>
#include <string.h>
#include "mif_file.h"


int mif_scan_mid(const char* mid, mif_callback_t cb_fun)
{
	int mif_code = MIF_SUCCESS;

	if (cb_fun)
	{
		FILE* pf = NULL;
		errno_t err = fopen_s(&pf, mid, "r");
		if (!err)
		{
			char buf[BUFSIZ];
			int lineno = 0;
			memset(buf, 0x00, BUFSIZ);

			while (fgets(buf, BUFSIZ, pf))
			{
				char delim[] = " ,\t\r\n";
				char* token = NULL;
				char* context = NULL;
				int tkn_num = 0;

				lineno++;
				token = strtok_s(buf, delim, &context);
				while (token)
				{
					int cb_ret = MIF_SUCCESS;
					cb_ret = cb_fun(MID_TOKEN, lineno, tkn_num, token);
					if (MIF_SUCCESS != cb_ret)
					{
						break;
					}

					// next
					tkn_num++;
					token = strtok_s(NULL, delim, &context);
				}
				(void)cb_fun(MID_END_LINE, -1, -1, NULL);
			}

			(void)cb_fun(MID_EOF, -1, -1, NULL);

			fclose(pf);
		}
		else
		{
			mif_code = MIF_FAILURE;
		}

	}
	else
	{
		// print some warning logs
	}


	return mif_code;
}



