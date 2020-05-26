#pragma once

#include <vgui/IScheme.h>
#include <vgui_controls/Label.h>
#include "mom_player_shared.h"
#include "mom_shareddefs.h"
#include "utlvector.h"

#define SPEEDOMETER_RANGE_LIST_T CUtlVector<Range_t>

// for ranged based coloring
struct Range_t
{
    int min;
    int max;
    Color color;
};

class SpeedometerLabel : public vgui::Label
{
    DECLARE_CLASS_SIMPLE(SpeedometerLabel, vgui::Label);

  public:
    SpeedometerLabel(Panel *parent, const char *panelName, SpeedometerColorize_t colorizeType,
                     bool supportsEnergyUnits = false, char *animationName = nullptr, float *animationAlpha = nullptr);

    void ApplySchemeSettings(vgui::IScheme *pScheme) OVERRIDE;
    void OnThink() OVERRIDE; // for applying fadeout
    
    // parent's layout depends on the visiblity of this
    void SetVisible(bool bVisible) OVERRIDE { BaseClass::SetVisible(bVisible); GetParent()->InvalidateLayout(); }

    void Update(float value); // main function; called when updating the label
    void Reset();
    void SetText(int value);
    void SetText(float value) { SetText(RoundFloatToInt(value)); }


    // fadeout related functions
    void SetFadeOutAnimation(char *animationName, float *animationAlpha) { m_pSzAnimationName = animationName; m_pFlAlpha = animationAlpha; }
    void ClearFadeOutAnimation() { m_pSzAnimationName = nullptr; m_pFlAlpha = nullptr; }
    bool HasFadeOutAnimation() { return m_pSzAnimationName && m_pSzAnimationName[0] && m_pFlAlpha; }

    // getter/setters
    SpeedometerUnits_t GetUnitType() { return m_eUnitType; }
    void SetUnitType(SpeedometerUnits_t type) { m_eUnitType = type; }
    void SetUnitType(int type) { SetUnitType(SpeedometerUnits_t(type)); }

    SpeedometerColorize_t GetColorizeType() { return m_eColorizeType; }
    void SetColorizeType(SpeedometerColorize_t type) { m_eColorizeType = type; Reset(); }
    void SetColorizeType(int type) { SetColorizeType(SpeedometerColorize_t(type)); }

    SPEEDOMETER_RANGE_LIST_T *GetRangeList() { return m_pRangeList; }
    void SetRangeList(SPEEDOMETER_RANGE_LIST_T *pRangeList)
    {
        if (m_pRangeList)
            delete m_pRangeList;
        m_pRangeList = pRangeList;
    }

    bool GetSupportsEnergyUnits() { return m_bSupportsEnergyUnits; }

  private:
    void ConvertUnits();
    void Colorize();
    bool StartFadeout();

    float m_flCurrentValue;
    float m_flPastValue;
    float m_flDiff;

    bool m_bSupportsEnergyUnits;

    // fadeout animation fields
    float *m_pFlAlpha;
    char *m_pSzAnimationName;
    bool m_bDoneFading;

    Color m_NormalColor, m_IncreaseColor, m_DecreaseColor;

    SPEEDOMETER_RANGE_LIST_T *m_pRangeList;

    SpeedometerUnits_t m_eUnitType;
    SpeedometerColorize_t m_eColorizeType;

    ConVarRef m_cvarGravity;
};