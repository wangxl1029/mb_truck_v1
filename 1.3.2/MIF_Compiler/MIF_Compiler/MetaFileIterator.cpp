#include "stdafx.h"
#include <vector>
#include <map>
#include <fstream>
#include <iostream>
#include <future>
#include <algorithm>
#include "MetaFileIterator.h"
#include "mif_file.h"
const char PATH_DELIM =
#if WIN32
'\\';
#else
'/';
#endif

const static char* g_city_ary[] = 
{
#if 0
	"anhui",
	"aomen",
#endif
	"beijing",
	"chongqing",
#if 0
	"fujian",
	"gansu",
	"guangdong1",
	"guangdong2",
	"guangxi",
	"guangzhou",
	"guizhou",
	"hainan",
	"hebei",
	"heilongjiang",
	"henan",
	"hubei",
	"hunan",
	"jiangsu1",
	"jiangsu2",
	"jiangxi",
	"jilin",
#endif
	"liaoning",
#if 0
	"neimenggu",
	"ningxia",
	"qinghai",
	"shan3xi",
	"shandong1",
	"shandong2",
	"shanghai",
	"shanxi",
	"sichuan1",
	"sichuan2",
	"tianjin",
	"xianggang",
	"xinjiang",
	"xizang",
	"yunnan",
#endif
	"zhejiang1",
	"zhejiang2"
};

enum class ECrossTransFeild : size_t{
	MapID, 
	ID, 
	inLinkID, 
	outLinkID, 
	CondType, 
	CRID, 
	FIELD_MAX
};
enum class ECrossRestrictTransFeild : size_t
{
	CRID,
	VPeriod,
	VPDir,
	Res_Weigh,
	Res_AxLoad,
	Res_AxCnt,
	Res_Trail,
	Res_Out,
	FIELD_MAX
};

const size_t CITY_NUM = sizeof(g_city_ary) / sizeof(g_city_ary[0]);
const size_t CROSS_TRANS_TXT_FIELD_MAX = static_cast<size_t>(ECrossTransFeild::FIELD_MAX);
const size_t CROSS_RSTR_TRANS_TXT_FIELD_MAX = static_cast<size_t>(ECrossRestrictTransFeild::FIELD_MAX);

typedef unsigned long long mif_byte13key_t;
typedef unsigned long mif_byte8_t;
typedef unsigned long mif_byte3_t;
typedef unsigned long mif_byte2_t;
typedef unsigned long mif_byte1_t;
struct mif_CrossTransInfo
{
	mif_byte8_t MapID; 
	mif_byte13key_t ID;
	mif_byte2_t CondType;
	mif_byte13key_t CRID;
};
class CCrossTransTextRecord
{
public: // life cycle
	CCrossTransTextRecord() {};
	~CCrossTransTextRecord() {};
public: // helper
	void fill(int index, std::string& field)
	{
		if (index >= 0 && index < CROSS_TRANS_TXT_FIELD_MAX)
		{
			m_FieldText[index] = field;
		}
	}
	
	std::string dump_text()
	{
		std::string ret_str;
		for (auto& field : m_FieldText)
		{
			ret_str += field + ',';
		}
		return ret_str;
	}

	typedef std::pair<mif_byte13key_t, mif_byte13key_t> InfoKey_t;
	bool CheckAndConvert(InfoKey_t& key, mif_CrossTransInfo& info) const
	{
		bool retval = false;
		if (std::all_of(m_FieldText, m_FieldText + CROSS_TRANS_TXT_FIELD_MAX,
			[](const std::string& s){
			return (s.size() > 2) && ('\"' == s[0] && '\"' == s[s.size() - 1]);
		}))
		{
			for (size_t i = 0; i < CROSS_TRANS_TXT_FIELD_MAX; i++)
			{
				std::string fieldStr(m_FieldText[i].begin() + 1, m_FieldText[i].end() - 1);
				switch (i)
				{
				case ECrossTransFeild::MapID:
					info.MapID = std::stoul(fieldStr);
					break;
				case ECrossTransFeild::ID:
					info.ID = std::stoull(fieldStr);
					break;
				case ECrossTransFeild::inLinkID:
					key.first = std::stoull(fieldStr);
					break;
				case ECrossTransFeild::outLinkID:
					key.second = std::stoull(fieldStr);
					break;
				case ECrossTransFeild::CondType:
					info.CondType = std::stoul(fieldStr);
					break;
				case ECrossTransFeild::CRID:
					info.CRID = std::stoul(fieldStr);
					break;
				case ECrossTransFeild::FIELD_MAX:
					break;
				default:
					break;
				}
			}

			retval = true;
		}

		return retval;
	}
private:
	std::string m_FieldText[CROSS_TRANS_TXT_FIELD_MAX];
};


struct mif_CrossRestrictInfo
{
	std::string VPeriod;
	mif_byte1_t VPDir;
	mif_byte8_t Res_Weigh;
	mif_byte8_t Res_AxLoad;
	mif_byte3_t Res_AxCnt;
	mif_byte1_t Res_Trail;
	mif_byte1_t Res_Out;
};

class CCrossRestrictTransTextRecord
{
public: // life cycle
	CCrossRestrictTransTextRecord() {};
	~CCrossRestrictTransTextRecord() {};
public: // helper
	void fill(int index, std::string& field)
	{
		if (index >= 0 && index < CROSS_RSTR_TRANS_TXT_FIELD_MAX)
		{
			m_FieldText[index] = field;
		}
	}

	std::string dump_text()
	{
		std::string ret_str;
		for (auto& field : m_FieldText)
		{
			ret_str += field + ',';
		}
		return ret_str;
	}
	
	bool CheckAndConvert(mif_byte13key_t& key, mif_CrossRestrictInfo& info) const
	{
		bool retval = false;
		if (std::all_of(m_FieldText, m_FieldText + CROSS_RSTR_TRANS_TXT_FIELD_MAX, 
			[](const std::string& s){ 
			return (s.size() > 2) && ('\"' == s[0] && '\"' == s[s.size() - 1]);
		}))
		{
			for (size_t i = 0; i < CROSS_RSTR_TRANS_TXT_FIELD_MAX; i++)
			{
				std::string fieldStr(m_FieldText[i].begin() +1,  m_FieldText[i].end() - 1);
				switch (i)
				{
				case ECrossRestrictTransFeild::CRID:
					key = std::stoll(fieldStr);
					break;
				case ECrossRestrictTransFeild::VPeriod:
					info.VPeriod = fieldStr;
					break;
				case ECrossRestrictTransFeild::VPDir:
					info.VPDir = std::stoul(fieldStr);
					break;
				case ECrossRestrictTransFeild::Res_Weigh:
					info.Res_Weigh = std::stoul(fieldStr);
					break;
				case ECrossRestrictTransFeild::Res_AxLoad:
					info.Res_AxLoad = std::stoul(fieldStr);
					break;
				case ECrossRestrictTransFeild::Res_AxCnt:
					info.Res_AxCnt = std::stoul(fieldStr);
					break;
				case ECrossRestrictTransFeild::Res_Trail:
					info.Res_Trail = std::stoul(fieldStr);
					break;
				case ECrossRestrictTransFeild::Res_Out:
					info.Res_Out = std::stoul(fieldStr);
					break;
				case ECrossRestrictTransFeild::FIELD_MAX:
					break;
				default:
					break;
				}
			}

			retval = true;
		}

		return retval;
	}
private:
	std::string m_FieldText[CROSS_RSTR_TRANS_TXT_FIELD_MAX];
};

class CCrossTransContext
{
public: // life cycle
	CCrossTransContext() {};
	CCrossTransContext(const CCrossTransContext&) = delete;
	~CCrossTransContext() {};
public: // helper 
	void FillCurRecord(int index, const char* token)
	{
		if (token)
		{
			m_CurRecord.fill(index, std::string(token));
		}
	}

	void AppendTable()
	{
		m_TextTable.push_back(m_CurRecord);
	}

	void CompleteTable()
	{
		// optimize the memory
		m_TextTable.shrink_to_fit();
	}

	void dump(const std::string& path)
	{
		std::ofstream dump_out(path);
		for (auto & rec : m_TextTable)
		{
			dump_out << rec.dump_text() << std::endl;
		}
	}

	typedef std::multimap<CCrossTransTextRecord::InfoKey_t, mif_CrossTransInfo> KeyTable;
	void convert()
	{
		for (const auto& record : m_TextTable)
		{
			CCrossTransTextRecord::InfoKey_t linkInOutKey = {0ULL, 0ULL};
			mif_CrossTransInfo info;
			if (record.CheckAndConvert(linkInOutKey, info))
			{
				m_KeyTable.insert({ linkInOutKey, info });
			}
		}
	}
	
	void ClearTextTable()
	{
		return m_TextTable.clear();
	} 

	KeyTable& GetKeyTable()
	{
		return m_KeyTable;
	}
private:
	std::vector<CCrossTransTextRecord> m_TextTable;
	KeyTable m_KeyTable;
	CCrossTransTextRecord m_CurRecord;
};

class CCrossRestrictTransContext
{
public: // life cycle
	CCrossRestrictTransContext() {};
	CCrossRestrictTransContext(const CCrossRestrictTransContext&) = delete;
	~CCrossRestrictTransContext() {};
public: // helper 
	void FillCurRecord(int index, const char* token)
	{
		if (token)
		{
			m_CurRecord.fill(index, std::string(token));
		}
	}

	void AppendTable()
	{
		m_TextTable.push_back(m_CurRecord);
	}

	void CompleteTable()
	{
		// optimize the memory
		m_TextTable.shrink_to_fit();

	}

	void dump(const std::string& path)
	{
		std::ofstream dump_out(path);
		for (auto & rec : m_TextTable)
		{
			dump_out << rec.dump_text() << std::endl;
		}
	}

	typedef std::vector<CCrossRestrictTransTextRecord> TextTabble;
	typedef std::multimap<mif_byte13key_t, mif_CrossRestrictInfo> KeyTable;
	void convert()
	{
		for (const auto& record : m_TextTable)
		{
			mif_byte13key_t crid = 0ULL;
			mif_CrossRestrictInfo info;
			if (record.CheckAndConvert(crid, info))
			{
				m_KeyTable.insert({ crid, info });
			}
		}
		
	}

	void ClearTextTable()
	{
		return m_TextTable.clear();
	}

	KeyTable& GetKeyTable()
	{
		return m_KeyTable;
	}
private:
	TextTabble m_TextTable;
	KeyTable m_KeyTable;
	CCrossRestrictTransTextRecord m_CurRecord;
};

class CTiedCrossAndRestrictInfo
{
public:
	void move(CCrossRestrictTransContext::KeyTable&& RestrictInfo, CCrossTransContext::KeyTable&& CrossInfo)
	{
		m_restrict = std::move(RestrictInfo);
		m_cross = std::move(CrossInfo);
	}
	typedef CCrossTransContext::KeyTable::key_type LinkKey_t;
	bool find(LinkKey_t k)
	{
		bool ret = false;
		const auto cross_itr = m_cross.find(k);
		if (cross_itr != m_cross.end())
		{
			const auto restr_itr = m_restrict.find(cross_itr->second.CRID);
			if (restr_itr != m_restrict.end())
			{
				ret = true;
			}
		}
		return ret;
	}
private:
	CCrossRestrictTransContext::KeyTable m_restrict;
	CCrossTransContext::KeyTable m_cross;
};

CCrossTransContext CMetaFileIterator::m_CrossTransContext;
CCrossRestrictTransContext CMetaFileIterator::m_CrossRestrictTransContext;
CTiedCrossAndRestrictInfo CMetaFileIterator::m_TiedCrossAndRestrictInfo;

CMetaFileIterator::CMetaFileIterator()
{
}

CMetaFileIterator::CMetaFileIterator(const char* root_dir)
: m_root_dir(root_dir)
{
	
}


CMetaFileIterator::~CMetaFileIterator()
{
}

void CMetaFileIterator::ScanCities()
{
	const char** cities = g_city_ary;
	for (size_t city_idx = 0; city_idx < CITY_NUM; city_idx++)
	{
		for (int loop_once = 0; loop_once < 1; loop_once++)
		{
			std::string city_dir = m_root_dir + PATH_DELIM + cities[city_idx];
			std::string C_Transport_mid = city_dir + PATH_DELIM + "C_Transport" + cities[city_idx] + ".mid";
			std::string CR_Transport_mid = city_dir + PATH_DELIM + "CR_Transport" + cities[city_idx] + ".mid";

			auto CrossTransFuture = std::async(mif_scan_mid, C_Transport_mid.c_str(), &CMetaFileIterator::CrossTransTextVisitor);
			auto CrossRestrictTransFuture = std::async(mif_scan_mid, CR_Transport_mid.c_str(), &CMetaFileIterator::CrossRestrictTransTextVisitor);

			auto CR_scan_ret = CrossRestrictTransFuture.get();
			auto C_scan_ret = CrossTransFuture.get();

			if (MIF_SUCCESS == CR_scan_ret && MIF_SUCCESS == C_scan_ret)
			{
				// do the connection across the table
				TieCrossAndRestrictInfo();
			}
			else
			{
				// print some warning
			}

		}

	}
}

int CMetaFileIterator::CrossTransTextVisitor(const int mif_evt, int lineno, int tkn_idx, const char* token)
{

	switch (mif_evt)
	{
	case MID_TOKEN:
		m_CrossTransContext.FillCurRecord(tkn_idx, token);
		break;
	case MID_END_LINE:
		m_CrossTransContext.AppendTable();
		break;
	case MID_EOF:
		//m_CrossTransContext.dump(std::string("cross_trans_mid.txt"));
		m_CrossTransContext.CompleteTable();
		break;
	default:
		break;
	}

	return MIF_SUCCESS;
}

int CMetaFileIterator::CrossRestrictTransTextVisitor(const int mif_evt, int lineno, int tkn_idx, const char* token)
{

	switch (mif_evt)
	{
	case MID_TOKEN:
		m_CrossRestrictTransContext.FillCurRecord(tkn_idx, token);
		break;
	case MID_END_LINE:
		m_CrossRestrictTransContext.AppendTable();
		break;
	case MID_EOF:
		//m_CrossTransContext.dump(std::string("cross_trans_mid.txt"));
		m_CrossRestrictTransContext.CompleteTable();
		break;
	default:
		break;
	}

	return MIF_SUCCESS;
}


void CMetaFileIterator::TieCrossAndRestrictInfo()
{
	m_CrossRestrictTransContext.convert();
	m_CrossRestrictTransContext.ClearTextTable();
	m_CrossTransContext.convert();
	m_CrossTransContext.ClearTextTable();

	//bool isEmpty = m_CrossRestrictTransContext.GetKeyTable().empty() && m_CrossTransContext.GetKeyTable().empty();
	//std::cout << "Before tie : " << std::boolalpha << isEmpty << std::endl;

	m_TiedCrossAndRestrictInfo.move(std::move(m_CrossRestrictTransContext.GetKeyTable()), std::move(m_CrossTransContext.GetKeyTable()));

	//isEmpty = m_CrossRestrictTransContext.GetKeyTable().empty() && m_CrossTransContext.GetKeyTable().empty();
	//std::cout << "After tie : " << std::boolalpha << isEmpty << std::endl;
}