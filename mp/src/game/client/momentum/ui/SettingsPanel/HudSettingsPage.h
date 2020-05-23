#pragma once

#include "SettingsPage.h"
#include "hud_speedometer.h"

class HudSettingsPage : public SettingsPage
{
    DECLARE_CLASS_SIMPLE(HudSettingsPage, SettingsPage);

    HudSettingsPage(Panel *pParent);
    ~HudSettingsPage() {}

    void OnCheckboxChecked(Panel *pPanel) OVERRIDE;
    void OnTextChanged(Panel *pPanel) OVERRIDE;
    void LoadSettings() OVERRIDE;

    // to handle speedo changes through console
    void OnPageShow() OVERRIDE { LoadSettings(); }
    void OnSetFocus() OVERRIDE { LoadSettings(); }

private:
    // speedo helper methods
    SpeedometerLabel *GetSpeedoLabelFromType();
    void LoadSpeedoSetup();
    void ExecuteSpeedoCmd(char *szText, bool bExecuteImmediately);

    vgui::CvarComboBox *m_pSyncType, *m_pSyncColorize;

    vgui::CvarToggleCheckButton *m_pSyncShow, *m_pSyncShowBar, *m_pButtonsShow, *m_pShowVersion, *m_pTimerShow, *m_pTimerSoundFailEnable,
        *m_pTimerSoundStartEnable, *m_pTimerSoundStopEnable, *m_pTimerSoundFinishEnable;

    // speedo controls
    vgui::ComboBox *m_pSpeedometerGameType, *m_pSpeedometerType, *m_pSpeedometerUnits, *m_pSpeedometerColorize;
    vgui::CheckButton *m_pSpeedometerShow;
};
