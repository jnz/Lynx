#include "lynx.h"
#include <stdio.h>
#include <iostream>
#include <sstream>
#include <string>
#include <stdexcept>
#include <string.h>

#ifdef _DEBUG
#include <crtdbg.h>
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#endif

CConfig::CConfig(void)
{

}

CConfig::~CConfig(void)
{
    Unload();
}

void CConfig::Unload()
{
    std::map<std::string, cvar_t*>::iterator iter;

    for(iter = m_var.begin();iter!=m_var.end();++iter)
        delete (*iter).second;
    m_var.clear();
}

bool CConfig::AddFile(const std::string& file)
{
    FILE* f;
    char buff[1024];

    f = fopen(file.c_str(), "rb");
    assert(f);
    if(!f)
        return false;

    while(!feof(f))
    {
        fgets(buff, sizeof(buff), f);

        // remove comments
        for(unsigned int i=0;i<strlen(buff);i++) // gcc will optimize the strlen
        {
            if(buff[i] == '#') // # is the comment character
            {
                buff[i] = 0;
                break;
            }
        }

        // Process the line
        AddLine(buff);
    }
    fclose(f);

    return true;
}

bool CConfig::AddLine(const std::string& line)
{
    char var[1024]; // variable
    char val[1024]; // value

    // scan for two tokens, the second one in quotes "%s"
    if(sscanf(line.c_str(), "%s \"%[^\"]\"", var, val) == 2)
    {
        AddVar(std::string(var), std::string(val));
        return true;
    }

    // scan for two tokens without quotes
    if(sscanf(line.c_str(), "%s %s", var, val) == 2)
    {
        AddVar(std::string(var), std::string(val));
        return true;
    }

    // There is something wrong with this line.
    // Print a warning if we find a printable character.
    for(unsigned int i=0;i<line.length();i++)
    {
        if(line[i] == ' ' || line[i] == '\t' || line[i] == '\n' || line[i] == '\r')
            continue;
        fprintf(stderr, "Config: Failed to parse config line: %s\n", line.c_str());
        break;
    }

    return false;
}

cvar_t* CConfig::AddVarFloat(const std::string& name, const float value)
{
    const int precision = (value == (int)value) ? 1 : 7;
    std::ostringstream o;
    cvar_t* cvar = GetVar(name, "", true); // create new if not in system
    cvar->value = value;

    o.precision(precision);
    if(!(o << value))
    {
        cvar->string = "NaN";
        assert(0); // Unable to convert float to string?
    }
    else
    {
        cvar->string = o.str();
    }

    return cvar;
}

cvar_t* CConfig::AddVar(const std::string& name, const std::string& value)
{
    cvar_t* cvar = GetVar(name, value, true); // create new if not in system
    cvar->string = value;
    cvar->value = (float)atof(value.c_str());

    return cvar;
}

cvar_t* CConfig::GetVar(const std::string& name, const std::string default_value, bool createnew)
{
    std::map<std::string, cvar_t*>::const_iterator iter;
    cvar_t* cvar;

    iter = m_var.find(name);
    if(iter == m_var.end()) // var is unknown so far
    {
        if(!createnew)
            return NULL;

        cvar = new cvar_t;
        cvar->name = name;
        cvar->string = default_value;
        cvar->value = (float)atof(default_value.c_str());
        m_var[name] = cvar;

        return cvar;
    }

    return (*iter).second;
}

cvar_t* CConfig::GetVarFloat(const std::string& name, const float default_value, bool createnew)
{
    cvar_t* cvar = GetVar(name, "", false);
    if(!cvar)
    {
        if(createnew)
            cvar = AddVarFloat(name, default_value);
        else
            cvar = NULL;
    }

    return cvar;
}

int CConfig::GetVarAsInt(const std::string& name, const int default_value, bool createnew)
{
    const cvar_t* cvar = GetVarFloat(name, (float)default_value, createnew);
    if(cvar == NULL)
        return default_value;
    else
        return (int)(cvar->value+0.5f);
}

std::string CConfig::GetVarAsStr(const std::string& name, const std::string& default_value, bool createnew)
{
    const cvar_t* cvar = GetVar(name, default_value, createnew);
    if(cvar == NULL)
        return default_value;
    else
        return cvar->string;
}

