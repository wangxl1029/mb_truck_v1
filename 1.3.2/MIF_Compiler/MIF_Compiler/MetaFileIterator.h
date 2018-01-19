
#ifndef _META_FILE_ITERATOR_H_
#define _META_FILE_ITERATOR_H_

#include <string>
#pragma once

class CCrossTransContext;
class CCrossRestrictTransContext;
class CTiedCrossAndRestrictInfo;
class CMetaFileIterator
{
public: // life cycle
	CMetaFileIterator(const char*);
	~CMetaFileIterator();
	// helper
	void ScanCities();
private:
	static int CrossTransTextVisitor(const int, int, int, const char*);
	static int CrossRestrictTransTextVisitor(const int, int, int, const char*);
	void TieCrossAndRestrictInfo();
private:	// data
	const std::string m_root_dir;
	static CTiedCrossAndRestrictInfo m_TiedCrossAndRestrictInfo;
	static CCrossTransContext m_CrossTransContext;
	static CCrossRestrictTransContext m_CrossRestrictTransContext;
private: // hide
	CMetaFileIterator();
};

#endif
