#include "DpiHelper.h"
#include <Windows.h>
#include <cstringt.h>
#include <iostream>
#include <map>
#include <string>
#include <vector>
using namespace std;


/*Get default DPI scaling percentage.
The OS recommented value.
*/
int GetRecommendedDPIScaling()
{
    int dpi = 0;

    auto retval = SystemParametersInfo(SPI_GETLOGICALDPIOVERRIDE, 0, (LPVOID)&dpi, 1);
    if (retval != 0)
    {
        int currDPI = DpiVals[dpi * -1];
        return currDPI;
    }

    return -1;
}



void SetDpiScaling(int percentScaleToSet)
{
    int recommendedDpiScale = GetRecommendedDPIScaling();

    if (recommendedDpiScale > 0)
    {
        int index = 0, recIndex = 0, setIndex = 0;
        for (const auto& scale : DpiVals)
        {
            if (recommendedDpiScale == scale)
            {
                recIndex = index;
            }
            if (percentScaleToSet == scale)
            {
                setIndex = index;
            }
            index++;
        }

        int relativeIndex = setIndex - recIndex;
        SystemParametersInfo(SPI_SETLOGICALDPIOVERRIDE, relativeIndex, (LPVOID)0, 1);
    }
}
//to store display info along with corresponding list item
struct DisplayData {
    LUID m_adapterId;
    int m_targetID;
    int m_sourceID;

    DisplayData()
    {
        m_adapterId = {};
        m_targetID = m_sourceID = -1;
    }
};

std::vector<DisplayData> GetDisplayData()
{
    std::vector<DisplayData> displayDataCache;
    std::vector<DISPLAYCONFIG_PATH_INFO> pathsV;
    std::vector<DISPLAYCONFIG_MODE_INFO> modesV;
    int flags = QDC_ONLY_ACTIVE_PATHS;
    if (false == DpiHelper::GetPathsAndModes(pathsV, modesV, flags))
    {
        cout << "DpiHelper::GetPathsAndModes() failed\n";
    }
    displayDataCache.resize(pathsV.size());
    for (int idx = 0; const auto &path : pathsV)
    {
        //get display name
        auto adapterLUID = path.targetInfo.adapterId;
        auto targetID = path.targetInfo.id;
        auto sourceID = path.sourceInfo.id;

        DISPLAYCONFIG_TARGET_DEVICE_NAME deviceName;
        deviceName.header.size = sizeof(deviceName);
        deviceName.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_TARGET_NAME;
        deviceName.header.adapterId = adapterLUID;
        deviceName.header.id = targetID;
        if (ERROR_SUCCESS != DisplayConfigGetDeviceInfo(&deviceName.header))
        {
            cout << "DisplayConfigGetDeviceInfo() failed";
        }
        else
        {
            std::wstring nameString = std::to_wstring(idx) + std::wstring(L". ") + deviceName.monitorFriendlyDeviceName;
            if (DISPLAYCONFIG_OUTPUT_TECHNOLOGY_INTERNAL == deviceName.outputTechnology)
            {
                nameString += L"(internal display)";
            }
            DisplayData dd = {};
            dd.m_adapterId = adapterLUID;
            dd.m_sourceID = sourceID;
            dd.m_targetID = targetID;

            displayDataCache[idx] = dd;
        }
        idx += 1;
    }
    return displayDataCache;
}

bool DPIFound(int val)
{
    bool found = false;
    for (int i = 0; i < 12; i++)
    {
        if (val == DpiVals[i])
        {
            found = true;
            break;
        }
    }
    return found;
}

int GetMonitorID(int index)
{
    std::vector<DisplayData> displayDataCache;
    std::vector<DISPLAYCONFIG_PATH_INFO> pathsV;
    std::vector<DISPLAYCONFIG_MODE_INFO> modesV;
    int flags = QDC_ONLY_ACTIVE_PATHS;
    if (false == DpiHelper::GetPathsAndModes(pathsV, modesV, flags))
    {
        return -1;
    }
    displayDataCache.resize(pathsV.size());
    if ((index+1) > pathsV.size()) return -1;
    auto path = pathsV[index];
    auto targetID = path.targetInfo.id;
    return targetID;

}

int GetMonitorDPI(int index)
{
    auto dpiToSet = 0;
    auto displayIndex = index;
    auto displayDataCache = GetDisplayData();


    if ((index + 1) > displayDataCache.size()) return -1;

    auto currentResolution = DpiHelper::GetDPIScalingInfo(displayDataCache[displayIndex].m_adapterId, displayDataCache[displayIndex].m_sourceID);
    return currentResolution.current;
}

bool SetMonitorDPI(int displayIndex, int dpiToSet)
{
    auto displayDataCache = GetDisplayData();
    if ((displayIndex + 1) > displayDataCache.size()) return false;

    auto success = DpiHelper::SetDPIScaling(displayDataCache[displayIndex].m_adapterId, displayDataCache[displayIndex].m_sourceID, dpiToSet);
    if (success == false)
    {
        return false;
    }
    else return true;

}

extern "C"
{
    __declspec(dllexport) int __stdcall dpi_GetRecommendedDPIScaling()
    {
        return GetRecommendedDPIScaling();
    }

    __declspec(dllexport) int __stdcall dpi_GetMonitorID(int index)
    {
        return GetMonitorID(index);
    }
    
    __declspec(dllexport) int __stdcall dpi_GetMonitorDPI(int index)
    {
        return GetMonitorDPI(index);
    }
    __declspec(dllexport) bool __stdcall dpi_SetMonitorDPI(int index, int dpi)
    {
        return SetMonitorDPI(index, dpi);
    }

}
