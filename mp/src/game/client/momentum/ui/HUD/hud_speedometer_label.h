#pragma once

#include <vgui/IScheme.h>
#include <vgui_controls/Label.h>
#include "mom_player_shared.h"
#include "mom_shareddefs.h"

class SpeedometerLabel : public vgui::Label
{
    DECLARE_CLASS_SIMPLE(SpeedometerLabel, vgui::Label);

  public:
    SpeedometerLabel(Panel *parent, const char *panelName, bool supportsEnergyUnits = false);
    
    // parent's layout depends on the visiblity of this
    void SetVisible(bool bVisible) OVERRIDE { BaseClass::SetVisible(bVisible); GetParent()->InvalidateLayout(); }

    void Update(float value); // main function; called when updating the label
    void Reset();
    void SetText(int value);
    void SetText(float value) { SetText(RoundFloatToInt(value)); }


    // getter/setters
    SpeedometerUnits_t GetUnitType() { return m_eUnitType; }
    void SetUnitType(SpeedometerUnits_t type) { m_eUnitType = type; }
    void SetUnitType(int type) { SetUnitType(SpeedometerUnits_t(type)); }

    bool GetSupportsEnergyUnits() { return m_bSupportsEnergyUnits; }

  private:
    void ConvertUnits();

    float m_flCurrentValue;
    float m_flPastValue;
    float m_flDiff;

    bool m_bSupportsEnergyUnits;

    SpeedometerUnits_t m_eUnitType;

    ConVarRef m_cvarGravity;
};