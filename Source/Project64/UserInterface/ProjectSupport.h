#pragma once
#include <string>
#include <vector>

class CProjectSupport
{

public:
    inline CProjectSupport(){};

    inline bool RequestCode(const char * Email)
    {
        return true;
    };
    inline bool ValidateCode(const char * Code)
    {
        return true;
    };
    inline void IncrementRunCount(){};
    inline bool ShowSupportWindow()
    {
        return false;
    };

    inline uint32_t RunCount() const
    {
        return 0x0;
    }
    inline const char * Name(void) const
    {
        return "";
    }
    inline const char * MachineID(void) const
    {
        return "";
    }
    inline bool Validated(void) const
    {
        return true;
    }

private:
    inline CProjectSupport(const CProjectSupport &){};
    inline CProjectSupport & operator=(const CProjectSupport &)
    {
        return *this;
    };

    inline std::string GenerateMachineID()
    {
        return std::string();
    };
    inline bool PerformRequest(const wchar_t * Url, const std::string & PostData, DWORD & StatusCode, std::vector<std::string> & Headers)
    {
        return true;
    };

    inline void SaveSupportInfo(void){};
    inline void LoadSupportInfo(void){};
};
