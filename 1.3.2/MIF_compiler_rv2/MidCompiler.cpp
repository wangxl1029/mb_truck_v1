#include <vector>
#include <map>
#include <fstream>
#include <iterator>
#include "MidCompiler.h"
#include "mid_metadata.h"

// global constants
const char PATH_DELIM =
#if WIN32
'\\';
#else
'/';
#endif

// local helper class CMidFile
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
                token = strtok_r(buf, delim, &savepoint);
                while (token)
                {
                    int ret = (*visitor)(directive::mid_token, tkn_num, token);
                    if (ret)
                    {
                        break;
                    }
                    // next
                    tkn_num++;
                    token = strtok_r(NULL, delim, &savepoint);
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
            auto strip_range = std::make_pair(token.cbegin(), std::prev(token.cend()));
            if ('\"' == *strip_range.first && '\"' == *strip_range.second)
            {
                m_stripped = std::string(std::next(strip_range.first), strip_range.second);
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
class CMidCrossTransVisitor : public CMidFile::CVisitor
{
public:
    // enum
    enum class token_id : size_t
    {
        MapID, ID, inLinkID, outLinkID, CondType, CRID, MAX
    };
    // nesting class
    struct CCrossTransInfo
    {
        mid_char8_val_t  MapID;
        mid_char13_val_t ID;
        mid_char13_val_t inLinkID;
        mid_char13_val_t outLinkID;
        mid_char2_val_t  CondType;
        mid_char13_val_t CRID;
    };
    // life cycle
    CMidCrossTransVisitor() = default;
    ~CMidCrossTransVisitor() = default;
    CMidCrossTransVisitor(const CMidCrossTransVisitor&) = delete;
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
    // operator
    typedef std::vector<CCrossTransInfo>  data_vec;
    data_vec& get_data()
    {
        return m_vecCrossTransInfo;
    }
private:
    // helper
    token_id toTokenID(size_t sz)
    {
        token_id ret = token_id::MAX;
        
        if (std::is_unsigned<decltype(sz)>::value && sz < static_cast<size_t>(token_id::MAX))
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
            switch (toTokenID(id))
            {
                case token_id::MapID:
                    m_CurInfo.MapID = field.toChar8_val();
                case token_id::ID:
                    m_CurInfo.ID = field.toChar13_val();
                    break;
                case token_id::inLinkID:
                    m_CurInfo.inLinkID = field.toChar13_val();
                    break;
                case token_id::outLinkID:
                    m_CurInfo.outLinkID = field.toChar13_val();
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
        m_vecCrossTransInfo.push_back(m_CurInfo);
        CurClean();
    }
    
    void endfile()
    {
        CurClean();
    }
    
    void CurClean()
    {
        memset(&m_CurInfo, 0x00, sizeof(m_CurInfo));
    }
    // data
    data_vec::value_type m_CurInfo;
    data_vec m_vecCrossTransInfo;
};

class CMidCrossRestrictTransVisitor : public CMidFile::CVisitor
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
    struct CCrossRestriTransInfo
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
    CMidCrossRestrictTransVisitor() = default;
    ~CMidCrossRestrictTransVisitor() = default;
    CMidCrossRestrictTransVisitor(const CMidCrossRestrictTransVisitor&) = delete;
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
    // method
    typedef std::vector<CCrossRestriTransInfo> data_vec;
    data_vec& get_data()
    {
        return m_vecCrossRestriTransInfo;
    }
private:
    // helper
    token_id toTokenID(size_t sz) const
    {
        token_id ret = token_id::MAX;
        if (std::is_unsigned<decltype(sz)>::value && sz < static_cast<size_t>(token_id::MAX))
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
            switch (toTokenID(id))
            {
                case token_id::CRID:
                    m_CurInfo.CRID = field.toChar13_val();
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
        m_vecCrossRestriTransInfo.push_back(m_CurInfo);
        CurClean();
    }
    void endfile()
    {}
    void CurClean()
    {
        memset(&m_CurInfo, 0x00, sizeof(m_CurInfo));
    }
    // data
    data_vec::value_type m_CurInfo;
    data_vec m_vecCrossRestriTransInfo;
};

// helper class CCondTransMidVisitor
class CMidCondTransMidVisitor : public CMidFile::CVisitor
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
    CMidCondTransMidVisitor()
    {
        CurClean();
    }
    
    ~CMidCondTransMidVisitor()
    {}
    
    CMidCondTransMidVisitor(const CMidCondTransMidVisitor&) = delete;
    
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
        
        if (std::is_unsigned<decltype(sz)>::value && sz < static_cast<size_t>(token_id::MAX))
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
//        m_mmapCondTrans.insert({ m_CurKey, m_CurInfo });
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
//    std::multimap<mid_char13_val_t, MID_CondTransInfo> m_mmapCondTrans;
    mid_char13_val_t m_CurKey;
    MID_CondTransInfo m_CurInfo;
};

// helper class CCrossNodeLineTransVisitor
class CMidCrossNodeLineTransVisitor : public CMidFile::CVisitor
{
public:
    // override
    int operator()(CMidFile::directive direct, size_t id, const char* token) final
    {
        return 0;
    }
};

class CMidBinLoader
{
public:
    // life cycle
    CMidBinLoader() = delete;
    CMidBinLoader(const char* bin):m_mid_bin(bin)
    {}
    CMidBinLoader(const CMidBinLoader&) = delete;
    ~CMidBinLoader() = default;
    // nesting class
    struct CCrossRestriTransValue
    {
        //mid_char13_val_t CRID;
        //CVehiclePeriod   VPeriod;
        mid_char1_val_t  VPDir;
        mid_char8_val_t  Res_Weigh;
        mid_char8_val_t  Res_AxLoad;
        mid_char3_val_t  Res_AxCnt;
        mid_char1_val_t  Res_Trail;
        mid_char1_val_t  Res_Out;
    };
    
    // nesting class
    struct CCrossTransValue
    {
        mid_char13_val_t ID;
        mid_char2_val_t  CondType;
        mid_char13_val_t CRID;
    };
    
    // operate
    CMidBinLoader& operator=(const CMidBinLoader&) = delete;
    bool load()
    {
        bool ret_val = false;
        
        return ret_val;
    }
private:
    const std::string m_mid_bin;    // full path of mid.bin file
    std::multimap<std::pair<mid_char13_val_t, mid_char13_val_t>, CCrossTransValue> m_mmpCrossTrans;
    std::multimap<mid_char13_val_t, CCrossRestriTransValue> m_mmpCrossRestriTrans;
};

class CMidSerialize
{
public:
    CMidSerialize() = delete;
    CMidSerialize(const char* path, bool isOutStream)
    : m_isOutStream(isOutStream)
    , m_isLittleEndian(false)
    , m_fs(path, std::ios::binary | (isOutStream ? std::ios::out : std::ios::in))
    {
        unsigned short val = 0;
        char* buf = reinterpret_cast<char*>(&val);
        switch (sizeof(val)) {
            case 2:
                val = 0x1234u;
                m_isLittleEndian = (buf[0] == 0x34) ? true : false;
                break;
            case 4:
                val = 0x12345678u;
                m_isLittleEndian = (buf[0] == 0x78) ? true : false;
                break;
            default:
                break;
        }

    }
    bool is_open() const
    {
        return m_fs.is_open();
    }
    // store
    void little_endian_store(unsigned char val)
    {
        return little_endian_store(reinterpret_cast<char*>(&val), sizeof(val));
    }
    void little_endian_store(unsigned short val)
    {
        return little_endian_store(reinterpret_cast<char*>(&val), sizeof(val));
    }
    void little_endian_store(unsigned long val)
    {
        return little_endian_store(reinterpret_cast<char*>(&val), sizeof(val));
    }
    void little_endian_store(unsigned long long val)
    {
        return little_endian_store(reinterpret_cast<char*>(&val), sizeof(val));
    }
    // restore
    void little_endian_restore(unsigned char *val)
    {
        return little_endian_restore(reinterpret_cast<char*>(val), sizeof(*val));
    }
    void little_endian_restore(unsigned short *val)
    {
        return little_endian_restore(reinterpret_cast<char*>(val), sizeof(*val));
    }
    void little_endian_restore(unsigned long *val)
    {
        return little_endian_restore(reinterpret_cast<char*>(val), sizeof(*val));
    }
    void little_endian_restore(unsigned long long *val)
    {
        return little_endian_restore(reinterpret_cast<char*>(val), sizeof(*val));
    }
private:
    void little_endian_store(char* buf, size_t siz)
    {
        if (m_isOutStream)
        {
            if (! m_isLittleEndian)
            {
                std::reverse(buf, buf + siz);
            }
            m_fs.write(buf, siz);
        }
    }
    void little_endian_restore(char* buf, size_t siz)
    {
        if (! m_isOutStream)
        {
            m_fs.read(buf, siz);
            if (! m_isLittleEndian)
            {
                std::reverse(buf, buf + siz);
            }
        }
    }
    std::fstream m_fs;
    const bool m_isOutStream;
    bool m_isLittleEndian;
};

// the implements for CMidCompiler
CMidCompiler::CMidCompiler(const char* root)
: m_root_dir(root)
{
}
class CMidMetaAllData
{
public:
    // life cycle
    CMidMetaAllData() = default;
    CMidMetaAllData(const CMidMetaAllData&) = delete;
    ~CMidMetaAllData() = default;
    // helper
    CMidMetaAllData& operator=(const CMidMetaAllData&) = delete;
    CMidCrossTransVisitor* getCrossTransVisitor()
    {
        return &m_CrossTransVisitor;
    }
    
    CMidCrossRestrictTransVisitor* getCrossRestriVisitor()
    {
        return &m_CrossRestriTransVisitor;
    }
    
    CMidCondTransMidVisitor* getCondTransVisitor()
    {
        return &m_CondTransVisitor;
    }
    
    CMidCrossNodeLineTransVisitor* getCrossNodeLineTransVisitor()
    {
        return &m_CrossNodeLineTransVisitor;
    }

    bool add_province()
    {
        ProvinData_t toAdd {
            std::move(m_CrossTransVisitor.get_data()),
            std::move(m_CrossRestriTransVisitor.get_data())
        };
        
        m_vecProvinData.push_back(std::move(toAdd));
        
        return true;
    }
    
    bool serilize(const char* serifile)
    {
        CMidSerialize out_bin(serifile, true);
        if (out_bin.is_open()) {
            MID_AllDataHeader_t all_data_header {m_vecProvinData.size()};
            out_bin.little_endian_store(all_data_header.province_num);
            for (auto prov = m_vecProvinData.cbegin(); prov < m_vecProvinData.cend(); ++prov)
            {
                out_bin.little_endian_store(prov->vecCross.size());
                for (auto cross = prov->vecCross.cbegin(); cross < prov->vecCross.cend(); ++cross) {
                    out_bin.little_endian_store(cross->MapID);
                    out_bin.little_endian_store(cross->ID);
                    out_bin.little_endian_store(cross->inLinkID);
                    out_bin.little_endian_store(cross->outLinkID);
                    out_bin.little_endian_store(cross->CondType);
                    out_bin.little_endian_store(cross->CRID);
                }
                out_bin.little_endian_store(prov->vecCroRestr.size());
                for (auto restr = prov->vecCroRestr.cbegin(); restr < prov->vecCroRestr.cend(); ++restr) {
                    out_bin.little_endian_store(restr->CRID);
                    // VPeriod
                    out_bin.little_endian_store(restr->VPDir);
                    out_bin.little_endian_store(restr->Res_Weigh);
                    out_bin.little_endian_store(restr->Res_AxLoad);
                    out_bin.little_endian_store(restr->Res_AxCnt);
                    out_bin.little_endian_store(restr->Res_Trail);
                    out_bin.little_endian_store(restr->Res_Out);
                }
            }
        }
        return true;
    }

    void cleanProvinceData()
    {
        m_vecProvinData.clear();
    }
    
    bool load(const char* binfile)
    {
        CMidSerialize in_bin(binfile, false);
        if (in_bin.is_open()) {
            MID_AllDataHeader_t all_data_header {0};
            in_bin.little_endian_restore(&all_data_header.province_num);
            m_vecProvinData.resize(all_data_header.province_num);
            for (auto prov = m_vecProvinData.begin(); prov < m_vecProvinData.end(); ++prov)
            {
                size_t CrossVecSiz = 0;
                in_bin.little_endian_restore(&CrossVecSiz);
                prov->vecCross.resize(CrossVecSiz);
                for (auto cross = prov->vecCross.begin(); cross < prov->vecCross.end(); ++cross) {
                    in_bin.little_endian_restore(&cross->MapID);
                    in_bin.little_endian_restore(&cross->ID);
                    in_bin.little_endian_restore(&cross->inLinkID);
                    in_bin.little_endian_restore(&cross->outLinkID);
                    in_bin.little_endian_restore(&cross->CondType);
                    in_bin.little_endian_restore(&cross->CRID);
                }
                size_t RestiVecSiz = 0;
                in_bin.little_endian_restore(&RestiVecSiz);
                prov->vecCroRestr.resize(RestiVecSiz);
                for (auto restr = prov->vecCroRestr.begin(); restr < prov->vecCroRestr.end(); ++restr) {
                    in_bin.little_endian_restore(&restr->CRID);
                    // VPeriod
                    in_bin.little_endian_restore(&restr->VPDir);
                    in_bin.little_endian_restore(&restr->Res_Weigh);
                    in_bin.little_endian_restore(&restr->Res_AxLoad);
                    in_bin.little_endian_restore(&restr->Res_AxCnt);
                    in_bin.little_endian_restore(&restr->Res_Trail);
                    in_bin.little_endian_restore(&restr->Res_Out);
                }
            }
        }

        return true;
    }
private:
    CMidCrossTransVisitor m_CrossTransVisitor;
    CMidCrossRestrictTransVisitor m_CrossRestriTransVisitor;
    CMidCondTransMidVisitor m_CondTransVisitor;
    CMidCrossNodeLineTransVisitor m_CrossNodeLineTransVisitor;
    
    typedef struct ProvinData_tag{
        CMidCrossTransVisitor::data_vec vecCross;
        CMidCrossRestrictTransVisitor::data_vec vecCroRestr;
    } ProvinData_t;
    typedef std::vector<ProvinData_t> ProvinData_vec;
    ProvinData_vec m_vecProvinData;
};



CMidCompiler::~CMidCompiler()
{
}


bool CMidCompiler::scan(const char* province[], const size_t provinum)
{
    CMidMetaAllData all_data;
    for (size_t i = 0; i < provinum; i++)
	{
        // C_Transport[city].mid
        CMidFile CrossTrans_mid(m_root_dir, province[i], "C_Transport");
        CrossTrans_mid.scan(all_data.getCrossTransVisitor());
        
        // CR_Transport[city].mid
        CMidFile CrossRestrTrans_mid(m_root_dir, province[i], "CR_Transport");
        CrossRestrTrans_mid.scan(all_data.getCrossRestriVisitor());
        
        // Cond_Transport[city].mid
        CMidFile CondTrans_mid(m_root_dir, province[i], "Cond_Transport");
        CondTrans_mid.scan(all_data.getCondTransVisitor());
        
        // CNL_Transport[city].mid
        CMidFile CrossNodeLineTrans_mid(m_root_dir, province[i], "CNL_Transport");
        CrossNodeLineTrans_mid.scan(all_data.getCrossNodeLineTransVisitor());
        
        all_data.add_province();
	}

    return all_data.serilize("some.bin");
}

bool CMidCompiler::test()
{
    CMidMetaAllData all_data;
    bool retval = all_data.load("some.bin");
    if (retval) {
        retval = all_data.serilize("other.bin");
    }
    
    return retval;
}
