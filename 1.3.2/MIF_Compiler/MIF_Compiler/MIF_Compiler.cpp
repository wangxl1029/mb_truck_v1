// MIF_Compiler.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "MetaFileIterator.h"

const char* MIF_ROOT = "E:\\proj_nav_truck\\map_data\\MIF\\Transport";

int _tmain(int argc, _TCHAR* argv[])
{
	CMetaFileIterator iter(MIF_ROOT);

	iter.ScanCities();


	return 0;
}

