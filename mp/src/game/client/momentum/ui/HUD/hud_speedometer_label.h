#pragma once

#include <vgui/IScheme.h>
#include <vgui_controls/Label.h>
#include "mom_player_shared.h"
#include "mom_shareddefs.h"

class SpeedometerLabel : public vgui::Label
{
    DECLARE_CLASS_SIMPLE(SpeedometerLabel, vgui::Label);

  public:
    SpeedometerLabel(Panel *parent, const char *panelName, bool supportsEnergyUnits = false,
                     char *animationName = nullptr, float *animationAlpha = nullptr);
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

    bool GetSupportsEnergyUnits() { return m_bSupportsEnergyUnits; }

  private:
    void ConvertUnits();
    bool StartFadeout();

    float m_flCurrentValue;
    float m_flPastValue;
    float m_flDiff;

    bool m_bSupportsEnergyUnits;

    // fadeout animation fields
    float *m_pFlAlpha;
    char *m_pSzAnimationName;
    bool m_bDoneFading;


    SpeedometerUnits_t m_eUnitType;

    ConVarRef m_cvarGravity;
};