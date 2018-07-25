//******************************************************************************
//******************************************************************************

#include "xroutersettings.h"
#include "xrouterlogger.h"

#include "../main.h"

#include <algorithm>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>
#include <iostream>

namespace xrouter
{  

//******************************************************************************
//******************************************************************************
bool IniConfig::read(const char * fileName)
{
    try
    {
        if (fileName)
        {
            m_fileName = std::string(fileName);
        }

        if (m_fileName.empty())
        {
            return false;
        }

        boost::property_tree::ini_parser::read_ini(m_fileName, m_pt);
        
        ifstream ifs(m_fileName.c_str(), ios::in | ios::binary | ios::ate);

        ifstream::pos_type fileSize = ifs.tellg();
        ifs.seekg(0, ios::beg);

        vector<char> bytes(fileSize);
        ifs.read(bytes.data(), fileSize);

        this->rawtext = string(bytes.data(), fileSize);
    }
    catch (std::exception & e)
    {
        LOG() << e.what();
        return false;
    }

    return true;
}

bool IniConfig::read(std::string config)
{
    try
    {
        istringstream istr(config.c_str());
        boost::property_tree::ini_parser::read_ini(istr, m_pt);
        this->rawtext = config;
    }
    catch (std::exception & e)
    {
        LOG() << e.what();
        return false;
    }

    return true;
}

//******************************************************************************
//******************************************************************************
void XRouterSettings::loadPlugins()
{
    std::vector<std::string> plugins;
    std::string pstr = get<std::string>("Main.plugins", "");
    boost::split(plugins, pstr, boost::is_any_of(","));
    for(std::string s : plugins)
        if(loadPlugin(s))
            pluginList.push_back(s);
}

bool XRouterSettings::loadPlugin(std::string name)
{
    std::string filename = pluginPath() + name + ".conf";
    XRouterPluginSettings settings;
    LOG() << "Trying to load plugin " << name + ".conf";
    if(!settings.read(filename.c_str()))
        return false;
    this->plugins[name] = settings;
    LOG() << "Successfully loaded plugin " << name;
    return true;
}

std::string XRouterSettings::pluginPath() const
{
    return std::string(GetDataDir(false).string()) + "/plugins/";
}

bool XRouterSettings::walletEnabled(std::string currency)
{
    std::vector<string> wallets;
    std::string wstr = get<std::string>("Main.wallets", "");
    boost::split(wallets, wstr, boost::is_any_of(","));
    if (std::find(wallets.begin(), wallets.end(), currency) != wallets.end())
        return true;
    else
        return false;
}

bool XRouterSettings::isAvailableCommand(XRouterCommand c, std::string currency, bool def)
{
    int res = 0;
    if (def)
        res = 1;
    res = get<int>(std::string(XRouterCommand_ToString(c)) + ".run", res);
    if (!currency.empty())
        res = get<int>(currency + "::" + std::string(XRouterCommand_ToString(c)) + ".run", res);
    if (res)
        return true;
    else
        return false;
}

double XRouterSettings::getCommandFee(XRouterCommand c, std::string currency, double def)
{
    double res = get<double>(std::string(XRouterCommand_ToString(c)) + ".fee", def);
    if (!currency.empty())
        res = get<double>(currency + "::" + std::string(XRouterCommand_ToString(c)) + ".fee", res);
    return res;
}

double XRouterSettings::getCommandTimeout(XRouterCommand c, std::string currency, double def)
{
    double res = get<double>("Main.timeout", def);
    res = get<double>(std::string(XRouterCommand_ToString(c)) + ".timeout", def);
    if (!currency.empty())
        res = get<double>(currency + "::" + std::string(XRouterCommand_ToString(c)) + ".timeout", res);
    return res;
}

bool XRouterSettings::hasPlugin(std::string name)
{
    return plugins.count(name) > 0;
}

bool XRouterPluginSettings::read(const char * fileName)
{
    if (!IniConfig::read(fileName))
        return false;
    
    formPublicText();
    return true;
}

bool XRouterPluginSettings::read(std::string config)
{
    if (!IniConfig::read(config))
        return false;
    
    formPublicText();
    return true;
}

void XRouterPluginSettings::formPublicText()
{
    std::vector<string> lines;
    boost::split(lines, this->rawtext, boost::is_any_of("\n"));
    this->publictext = "";
    std::string prefix = "private::";
    for (std::string line : lines) {
        if (line.compare(0, prefix.size(), prefix))
            this->publictext += line + "\n";
        else {
            this->publictext += line.erase(0, prefix.size()) + "\n"; 
        }
    }
}

std::string XRouterPluginSettings::getParam(std::string param, std::string def)
{
    try
    {
        return m_pt.get<std::string>(param);
    }
    catch (std::exception & e)
    {
        return get<std::string>("private::" + param, def);
    }
}

double XRouterPluginSettings::getFee()
{
    return get<double>("fee", 0.0);
}

int XRouterPluginSettings::getParamCount()
{
    return get<int>("paramsCount", 0);
}

} // namespace xrouter