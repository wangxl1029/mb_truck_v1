#pragma once
#ifndef _MID_COMPILER_H_
#define _MID_COMPILER_H_
#include <string>
class CMidCompiler
{
public:
	CMidCompiler() = delete;
	CMidCompiler(const char* root);
	~CMidCompiler();
	// helper
	bool scan(const char*[], const size_t);
    bool test();
private:
	const std::string m_root_dir;
};
#endif

