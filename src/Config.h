#pragma once
#include <string>
#include <map>

struct cvar_t
{
    std::string name;
    std::string string;
    float       value;
};

/*
 *   The Lynx config system.
 *   Steps: Load a file with AddLine:
 *
 *   <code>
 *     CConfig cfg;
 *     cfg.AddFile("game.cfg");
 *   </code>
 *
 *   Example: static check for screen width, default value is 1024:
 *
 *   <code>
 *     int screen_width;
 *     const int screen_width_default = 1024;
 *     const bool createnew = true;
 *     screen_width = cfg.GetVarAsInt("width", screen_width_default, createnew);
 *   </code>
 *
 *   The parameter createnew tells the config system to create a cvar_t for
 *   if it is not already in the system. The new cvar_t is then set to the
 *   supplied default value (here 1024).
 *
 */

class CConfig
{
public:
    CConfig(void);
    virtual ~CConfig(void);

    // Add stuff to the config system
    bool          AddFile(const std::string& file); // add every line from the file to the config system. e.g. "game.cfg"
    bool          AddLine(const std::string& line); // add a single line to the config system. e.g. "width 1024"
    void          Unload();

    //
    cvar_t*       GetVar(const std::string& name, const std::string default_value, bool createnew=true);
    cvar_t*       GetVarFloat(const std::string& name, const float default_value, bool createnew=true);
    int           GetVarAsInt(const std::string& name, const int default_value, bool createnew=true);
    std::string   GetVarAsStr(const std::string& name, const std::string& default_value, bool createnew=true);

private:
    cvar_t*       AddVarFloat(const std::string& name, const float value);
    cvar_t*       AddVar(const std::string& name, const std::string& value);

    std::map<std::string, cvar_t*> m_var;
};

