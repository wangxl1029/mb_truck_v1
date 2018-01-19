// header section
#include <functional>
#include <iostream>
#include <fstream>
#include <map>
#include <limits>

#include "MetaFileIterator.h"

// global constants
const char PATH_DELIM =
#if WIN32
'\\';
#else
'/';
#endif

// local class


class CMidFile
{
public:
	// life cycle
	CMidFile() = delete;
	CMidFile(const std::string& root, const std::string& city, const std::string basename)
		: m_path(root + PATH_DELIM + city + PATH_DELIM + basename + city + ".mid")
		, m_ifs(m_path)
	{
	}
	~CMidFile() {};

	// enum
	enum class directive: size_t
	{
		mid_token,
		mid_endl,
		mid_eof
	};
	
	// embed class
	class CVisitor
	{
	public:
		virtual int operator()(directive, size_t, const char*) = 0;
	};

	// helper
	CMidFile& operator=(const CMidFile&) = delete;
	void scan(CVisitor* visitor)
	{
		if (m_ifs.is_open() && nullptr != visitor)
		{
			int lineno = 0;
			char buf[BUFSIZ];
			memset(buf, 0x00, BUFSIZ);

			while (m_ifs.getline(buf, BUFSIZ))
			{
				char delim[] = " ,\t\r\n";
				char* token = NULL;
				char* savepoint = NULL;
				size_t tkn_num = 0;

				lineno++;
				token = strtok_s(buf, delim, &savepoint);
				while (token)
				{
					int ret = (*visitor)(directive::mid_token, tkn_num, token);
					if (ret)
					{
						break;
					}
					// next
					tkn_num++;
					token = strtok_s(NULL, delim, &savepoint);
				}

				(*visitor)(directive::mid_endl, -1, nullptr);
			}
			(*visitor)(directive::mid_eof, -1, nullptr);
		}
	}

private:
	const std::string m_path;
	std::ifstream m_ifs;
};

typedef unsigned char mid_char1_val_t; // max = 9, => 1 byte
typedef unsigned char mid_char2_val_t; // max = 99 => 1 byte
typedef unsigned short mid_char3_val_t; // max = 999 => 2 bytes
typedef unsigned long mid_char8_val_t; // max = 99,999,999 => 4 bytes
typedef unsigned long long mid_char13_val_t; // max = 9,999,999,999,999 => 8 bytes

// helper class CMidToken
class CMidToken
{
public:
	CMidToken() = delete;
	CMidToken(const std::string& token)
	{
		m_ok = false;
		if (token.size() > 2)
		{
			auto strip_range = std::make_pair(token.cbegin(), token.cend() - 1);
			if ('\"' == *strip_range.first && '\"' == *strip_range.second)
			{
				m_stripped = std::string(strip_range.first + 1, strip_range.second);
				m_ok = true;
			}
		}
	}

	~CMidToken() {};
	// helper
	CMidToken& operator=(const CMidToken&) = delete;
	bool isOk()  const
	{
		return m_ok;
	}

	mid_char1_val_t toChar1_val() const
	{
		return m_stripped.empty() ? std::numeric_limits<mid_char1_val_t>::max() : static_cast<mid_char2_val_t>(std::stoul(m_stripped));
	}

	mid_char2_val_t toChar2_val() const
	{
		return m_stripped.empty() ? std::numeric_limits<mid_char2_val_t>::max() : static_cast<mid_char2_val_t>(std::stoul(m_stripped));
	}

	mid_char3_val_t toChar3_val() const
	{
		return m_stripped.empty() ? std::numeric_limits<mid_char3_val_t>::max() : static_cast<mid_char2_val_t>(std::stoul(m_stripped));
	}

	mid_char8_val_t toChar8_val() const
	{
		return m_stripped.empty() ? std::numeric_limits<mid_char8_val_t>::max() : std::stoul(m_stripped);
	}

	mid_char13_val_t toChar13_val() const
	{
		return m_stripped.empty() ? std::numeric_limits<mid_char13_val_t>::max() : std::stoull(m_stripped);
	}
private:
	std::string m_stripped;
	bool m_ok;
};

// helper class CCrossTransMidVisitor
class CCrossTransMidVisitor : public CMidFile::CVisitor
{
public:
	// enum
	enum class token_id : size_t
	{
		ID, inLinkID, outLinkID, CondType, CRID, MAX
	};
	// nesting class
	struct CCrossRestriTransInfo
	{
		mid_char13_val_t ID;
		mid_char2_val_t  CondType;
		mid_char13_val_t CRID;
	};
	// life cycle
	CCrossTransMidVisitor() = default;
	~CCrossTransMidVisitor() = default;
	CCrossTransMidVisitor(const CCrossTransMidVisitor&) = delete;
	// override
	int operator()(CMidFile::directive direct, size_t id, const char* token) final
	{
		switch (direct)
		{
		case CMidFile::directive::mid_token:
			fill(id, token);
			break;
		case CMidFile::directive::mid_endl:
			endline();
			break;
		case CMidFile::directive::mid_eof:
			endfile();
			break;
		default:
			break;
		}

		return 0;
	}
private:
	// helper
	token_id SizeToTokeID(size_t sz)
	{
		token_id ret = token_id::MAX;
		if (sz >= 0 && sz < static_cast<size_t>(token_id::MAX))
		{
			ret = static_cast<token_id>(sz);
		}
		return ret;
	}

	void fill(size_t id, const std::string token)
	{
		CMidToken field(token);
		if (field.isOk())
		{
			switch (SizeToTokeID(id))
			{
			case token_id::ID:
				m_CurInfo.ID = field.toChar8_val();
				break;
			case token_id::inLinkID:
				m_CurKey.first = field.toChar13_val();
				break;
			case token_id::outLinkID:
				m_CurKey.second = field.toChar13_val();
				break;
			case token_id::CondType:
				m_CurInfo.CondType = field.toChar2_val();
				break;
			case token_id::CRID:
				m_CurInfo.CRID = field.toChar13_val();
				break;
			default:
				break;
			}
		}
	}

	void endline()
	{
		m_mmapCondTrans.insert({ m_CurKey, m_CurInfo });
		CurClean();
	}

	void endfile()
	{
		CurClean();
	}

	void CurClean()
	{
		m_CurKey = { 0, 0 };
		memset(&m_CurInfo, 0x00, sizeof(m_CurInfo));
	}
	// data
	std::pair<mid_char13_val_t,mid_char13_val_t> m_CurKey;
	CCrossRestriTransInfo m_CurInfo;
	std::multimap<decltype(m_CurKey), decltype(m_CurInfo)> m_mmapCondTrans;
};

class CCrossRestrictTransMidVisitor : public CMidFile::CVisitor
{
public:
	// enum
	enum class token_id : size_t
	{
		CRID, VPeriod, VPDir, Res_Weigh, Res_AxLoad, Res_AxCnt, Res_Trail, Res_Out, MAX
	};
	struct CVehiclePeriod
	{

	};
	// nesting class
	struct CCrossTransInfo
	{
		mid_char13_val_t CRID;
		CVehiclePeriod   VPeriod;
		mid_char1_val_t  VPDir;
		mid_char8_val_t  Res_Weigh;
		mid_char8_val_t  Res_AxLoad;
		mid_char3_val_t  Res_AxCnt;
		mid_char1_val_t  Res_Trail;
		mid_char1_val_t  Res_Out;
	};
	// life cycle
	CCrossRestrictTransMidVisitor() = default;
	~CCrossRestrictTransMidVisitor() = default;
	CCrossRestrictTransMidVisitor(const CCrossRestrictTransMidVisitor&) = delete;
	// override
	int operator()(CMidFile::directive direct, size_t id, const char* token) final
	{
		switch (direct)
		{
		case CMidFile::directive::mid_token:
			fill(id, token);
			break;
		case CMidFile::directive::mid_endl:
			endline();
			break;
		case CMidFile::directive::mid_eof:
			endfile();
			break;
		default:
			break;
		}

		return 0;
	}
private:
	// helper
	token_id SizeToTokeID(size_t sz) const
	{
		token_id ret = token_id::MAX;
		if (sz >= 0 && sz < static_cast<size_t>(token_id::MAX))
		{
			ret = static_cast<token_id>(sz);
		}
		return ret;
	}

	void fill(size_t id, const std::string token)
	{
		CMidToken field(token);
		if (field.isOk())
		{
			switch (SizeToTokeID(id))
			{
			case token_id::CRID:
				m_CurKey = field.toChar13_val();
				break;
			case token_id::VPDir:
				m_CurInfo.VPDir = field.toChar1_val();
				break;
			case token_id::Res_Weigh:
				m_CurInfo.Res_Weigh = field.toChar8_val();
				break;
			case token_id::Res_AxLoad:
				m_CurInfo.Res_AxLoad = field.toChar8_val();
				break;
			case token_id::Res_AxCnt:
				m_CurInfo.Res_AxCnt = field.toChar3_val();
				break;
			case token_id::Res_Trail:
				m_CurInfo.Res_Trail = field.toChar1_val();
				break;
			case token_id::Res_Out:
				m_CurInfo.Res_Out = field.toChar1_val();
				break;
			default:
				break;
			}
		}
	}

	void endline()
	{
		m_mmapCrossTrans.insert({ m_CurKey, m_CurInfo });
		CurClean();
	}
	void endfile()
	{}
	void CurClean()
	{
		m_CurKey = 0;
		memset(&m_CurInfo, 0x00, sizeof(m_CurInfo));
	}
	// data
	mid_char13_val_t m_CurKey;
	CCrossTransInfo m_CurInfo;
	std::multimap<decltype(m_CurKey), decltype(m_CurInfo)> m_mmapCrossTrans;
};

// helper class CCondTransMidVisitor
class CCondTransMidVisitor : public CMidFile::CVisitor
{
public:
	// enum
	enum class token_id : size_t
	{ MapID, CondID, CondType, CRID, MAX};
	// nesting class
	struct MID_CondTransInfo
	{
		mid_char8_val_t  MapID;
		mid_char2_val_t  CondType;
		mid_char13_val_t CRID;
	};
	// life cycle
	CCondTransMidVisitor()
	{
		CurClean();
	}

	~CCondTransMidVisitor()
	{}

	CCondTransMidVisitor(const CCondTransMidVisitor&) = delete;

	// override
	int operator()(CMidFile::directive direct, size_t id, const char* token) final
	{
		switch (direct)
		{
		case CMidFile::directive::mid_token:
			fill(id, token);
			break;
		case CMidFile::directive::mid_endl:
			endline();
			break;
		case CMidFile::directive::mid_eof:
			endfile();
			break;
		default:
			break;
		}

		return 0;
	}

private:
	// helper
	token_id SizeToTokeID(size_t sz)
	{
		token_id ret = token_id::MAX;
		if (sz >= 0 && sz < static_cast<size_t>(token_id::MAX))
		{
			ret = static_cast<token_id>(sz);
		}
		return ret;
	}

	void fill(size_t id, const std::string token)
	{
		CMidToken field(token);
		if (field.isOk())
		{
			switch (SizeToTokeID(id))
			{
			case token_id::MapID:
				m_CurInfo.MapID = field.toChar8_val();
				break;
			case token_id::CondID:
				m_CurKey = field.toChar13_val();
				break;
			case token_id::CondType:
				m_CurInfo.CondType = field.toChar2_val();
				break;
			case token_id::CRID:
				m_CurInfo.CRID = field.toChar13_val();
				break;
			default:
				break;
			}
		}
	}

	void endline()
	{
		m_mmapCondTrans.insert({ m_CurKey, m_CurInfo });
		CurClean();
	}

	void endfile()
	{
		CurClean();
	}

	void CurClean()
	{
		m_CurKey = 0;
		memset(&m_CurInfo, 0x00, sizeof(m_CurInfo));
	}
	// data
	std::multimap<mid_char13_val_t, MID_CondTransInfo> m_mmapCondTrans;
	mid_char13_val_t m_CurKey;
	MID_CondTransInfo m_CurInfo;
};

// helper class CCrossNodeLineTransVisitor
class CCrossNodeLineTransVisitor : public CMidFile::CVisitor
{
public:
	// override
	int operator()(CMidFile::directive direct, size_t id, const char* token) final
	{
		return 0;
	}
};

// helper class CTruckMidContext
class CTruckMidContext
{
public:
	// life cycle
	CTruckMidContext() = default;
	CTruckMidContext(const CTruckMidContext&) = delete;
	~CTruckMidContext() = default;
	// helper
	CTruckMidContext& operator=(const CTruckMidContext&) = delete;
	void CheckAndConvert()
	{
	}

	CCrossTransMidVisitor* getCrossTransVisitor()
	{
		return &m_CrossTransVisitor;
	}

	CCrossRestrictTransMidVisitor* getCrossRestriVisitor()
	{
		return &m_CrossRestriTransVisitor;
	}

	CCondTransMidVisitor* getCondTransVisitor()
	{
		return &m_CondTransVisitor;
	}

	CCrossNodeLineTransVisitor* getCrossNodeLineTransVisitor()
	{
		return &m_CrossNodeLineTransVisitor;
	}
private:
	CCrossTransMidVisitor m_CrossTransVisitor;
	CCrossRestrictTransMidVisitor m_CrossRestriTransVisitor;
	CCondTransMidVisitor m_CondTransVisitor;
	CCrossNodeLineTransVisitor m_CrossNodeLineTransVisitor;
};

CMetaFileIterator::CMetaFileIterator(const char* root)
: m_root_dir(root)
{
}

CMetaFileIterator::~CMetaFileIterator()
{
}

void CMetaFileIterator::ScanCities(const char* cities[], const size_t city_num)
{
	CTruckMidContext MidContext;
	for (size_t i = 0; i < city_num; i++)
	{
		// C_Transport[city].mid
		CMidFile CrossTrans_mid(m_root_dir, cities[i], "C_Transport");
		CrossTrans_mid.scan(MidContext.getCrossTransVisitor());

		// CR_Transport[city].mid
		CMidFile CrossRestrTrans_mid(m_root_dir, cities[i], "CR_Transport");
		CrossRestrTrans_mid.scan(MidContext.getCrossRestriVisitor());

		// Cond_Transport[city].mid
		CMidFile CondTrans_mid(m_root_dir, cities[i], "Cond_Transport");
		CondTrans_mid.scan(MidContext.getCondTransVisitor());
		std::cout << "after binder" << std::endl;
		MidContext.CheckAndConvert();

		// CNL_Transport[city].mid
		CMidFile CrossNodeLineTrans_mid(m_root_dir, cities[i], "CNL_Transport");
		CrossNodeLineTrans_mid.scan(MidContext.getCrossNodeLineTransVisitor());
		MidContext.CheckAndConvert();
	}
}