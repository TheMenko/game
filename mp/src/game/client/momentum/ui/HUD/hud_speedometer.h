#pragma once

#include "hud_speedometer_label.h"

#include "hudelement.h"

#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/Label.h>

#define SPEEDOMETER_MAX_LABELS 4
#define SPEEDOMETER_LABEL_ORDER_LIST_T CUtlVector <SpeedometerLabel *>

class CHudSpeedMeter : public CHudElement, public vgui::EditablePanel
{
    DECLARE_CLASS_SIMPLE(CHudSpeedMeter, vgui::EditablePanel);

  public:
    CHudSpeedMeter(const char *pElementName);

    void OnThink() OVERRIDE;
    void Init() OVERRIDE;
    void Reset() OVERRIDE;
    void FireGameEvent(IGameEvent *pEvent) OVERRIDE;
    void ApplySchemeSettings(vgui::IScheme *pScheme) OVERRIDE;
    void PerformLayout() OVERRIDE;

    void LoadGamemodeData(GameMode_t gametype);
    void SaveGamemodeData(GameMode_t gametype);

    SpeedometerLabel **GetLabelList() { return m_Labels; }
    SPEEDOMETER_LABEL_ORDER_LIST_T *GetLabelOrderListPtr() { return &m_LabelOrderList; }
    // gets what game mode pertains to the settings currently loaded
    GameMode_t GetCurrentlyLoadedGameMode() { return m_CurrentlyLoadedGamemodeSettings; }

  private:
    SpeedometerLabel *GetLabelFromName(const char* name);
    KeyValues *GetGamemodeKVs(GameMode_t gametype);
    void ResetLabelOrder();

    Color m_NormalColor, m_IncreaseColor, m_DecreaseColor;

    SpeedometerLabel *m_pAbsSpeedoLabel, *m_pHorizSpeedoLabel, *m_pLastJumpVelLabel, *m_pStageEnterExitVelLabel;

    KeyValues *m_pGamemodeSetupData;
    GameMode_t m_CurrentlyLoadedGamemodeSettings;

    SPEEDOMETER_LABEL_ORDER_LIST_T m_LabelOrderList;

    SpeedometerLabel *m_Labels[SPEEDOMETER_MAX_LABELS] 
        { m_pAbsSpeedoLabel, m_pHorizSpeedoLabel, m_pLastJumpVelLabel, m_pStageEnterExitVelLabel };

    CMomRunStats *m_pRunStats;
    CMomRunEntityData *m_pRunEntData;

    ConVarRef m_cvarTimeScale;

  protected:
    CPanelAnimationVar(Color, m_bgColor, "BgColor", "Blank");
    CPanelAnimationVar(float, m_fLastJumpVelAlpha, "LastJumpVelAlpha", "0.0");
    CPanelAnimationVar(float, m_fStageVelAlpha, "StageVelAlpha", "0.0");
};

extern CHudSpeedMeter *g_pSpeedometer;