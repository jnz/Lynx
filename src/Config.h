#pragma once
#include <string>
#include <map>

struct cvar_t
{
    std::string name;
    std::string string;
    float       value;
};

class CConfig
{
public:
    CConfig(void);
    virtual ~CConfig(void);

    bool          AddFile(const std::string& file);
    void          AddLine(const std::string& line);
    void          Unload();

    cvar_t*       GetVar(const std::string& name, const std::string default_value, bool createnew=true);
    cvar_t*       GetVarFloat(const std::string& name, const float default_value, bool createnew=true);
    int           GetVarAsInt(const std::string& name, const int default_value, bool createnew=true);
    std::string   GetVarAsStr(const std::string& name, const std::string& default_value, bool createnew=true);

private:
    cvar_t*       AddVarFloat(const std::string& name, const float value);
    cvar_t*       AddVar(const std::string& name, const std::string& value);

    std::map<std::string, cvar_t*> m_var;
};

