#pragma once
#ifndef _META_FILE_ITERATOR_H_
#define _META_FILE_ITERATOR_H_

#include <string>
class CMetaFileIterator
{
public:
	// life cycle
	CMetaFileIterator(const char* root);
	CMetaFileIterator() = delete;
	CMetaFileIterator& operator=(const CMetaFileIterator&) = delete;
	~CMetaFileIterator();
	// helper
	void ScanCities(const char* cities[], const size_t city_num);
private:
	const std::string m_root_dir;
};
#endif

