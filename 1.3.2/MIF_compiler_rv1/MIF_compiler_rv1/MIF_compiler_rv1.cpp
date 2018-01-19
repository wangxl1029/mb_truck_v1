// MIF_compiler_rv1.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "MetaFileIterator.h"


const static char* g_test_cities[] =
{
	"beijing",
	"chongqing", 
	"liaoning" 
};

const char* MIF_ROOT = "E:\\proj_nav_truck\\map_data\\MIF\\Transport";
const size_t TEST_CITY_NUM = sizeof(g_test_cities) / sizeof(g_test_cities[0]);

int _tmain(int argc, _TCHAR* argv[])
{
	CMetaFileIterator itr(MIF_ROOT);

	itr.ScanCities(g_test_cities, TEST_CITY_NUM);
	return 0;
}

