#pragma once

// 由 generate_icon_enum.py 脚本自动生成，请勿手动修改
// 在 build/ 目录执行 cmake --build . --target update_icon_defines 以重新生成
// 生成时间: 2026-05-06T13:57:57.618144

#include <string>
#include <map>

enum class IconID {
    kGeneralRefresh = 0,
    kLayerCurrent,
    kLayerFrozen,
    kLayerLocked,
    kLayerOther,
    kLayerUnFrozen,
    kLayerUnlocked,
    kStatusBarAxes,
    kStatusBarDynamicMode,
    kStatusBarFullScreen,
    kStatusBarGrid,
    kStatusBarGridSnap,
    kStatusBarLineWeight,
    kStatusBarOrthogonal,
    kStatusBarPolarTracking,
    kStatusBarSettings,
    kStatusBarSnap,
    kStatusBarTransParency,
    kCOUNT
};

// 图标路径映射表，得到的是相对于可执行文件所在目录的相对路径
inline const std::map<IconID, std::string> g_iconPaths = {
    { IconID::kGeneralRefresh, "res/icons/GeneralRefresh.png" },
    { IconID::kLayerCurrent, "res/icons/LayerCurrent.png" },
    { IconID::kLayerFrozen, "res/icons/LayerFrozen.png" },
    { IconID::kLayerLocked, "res/icons/LayerLocked.png" },
    { IconID::kLayerOther, "res/icons/LayerOther.png" },
    { IconID::kLayerUnFrozen, "res/icons/LayerUnFrozen.png" },
    { IconID::kLayerUnlocked, "res/icons/LayerUnlocked.png" },
    { IconID::kStatusBarAxes, "res/icons/StatusBarAxes.png" },
    { IconID::kStatusBarDynamicMode, "res/icons/StatusBarDynamicMode.png" },
    { IconID::kStatusBarFullScreen, "res/icons/StatusBarFullScreen.png" },
    { IconID::kStatusBarGrid, "res/icons/StatusBarGrid.png" },
    { IconID::kStatusBarGridSnap, "res/icons/StatusBarGridSnap.png" },
    { IconID::kStatusBarLineWeight, "res/icons/StatusBarLineWeight.png" },
    { IconID::kStatusBarOrthogonal, "res/icons/StatusBarOrthogonal.png" },
    { IconID::kStatusBarPolarTracking, "res/icons/StatusBarPolarTracking.png" },
    { IconID::kStatusBarSettings, "res/icons/StatusBarSettings.png" },
    { IconID::kStatusBarSnap, "res/icons/StatusBarSnap.png" },
    { IconID::kStatusBarTransParency, "res/icons/StatusBarTransParency.png" },
};
