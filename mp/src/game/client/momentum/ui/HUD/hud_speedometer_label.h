#pragma once

#include <vgui/IScheme.h>
#include <vgui_controls/Label.h>
#include "mom_player_shared.h"
#include "mom_shareddefs.h"

class SpeedometerLabel : public vgui::Label
{
    DECLARE_CLASS_SIMPLE(SpeedometerLabel, vgui::Label);

  public:
    SpeedometerLabel(Panel *parent, const char *panelName);
    
    // parent's layout depends on the visiblity of this
    void SetVisible(bool bVisible) OVERRIDE { BaseClass::SetVisible(bVisible); GetParent()->InvalidateLayout(); }

    void Update(float value); // main function; called when updating the label
    void Reset();
    void SetText(int value);
    void SetText(float value) { SetText(RoundFloatToInt(value)); }

  private:

    float m_flCurrentValue;
    float m_flPastValue;
    float m_flDiff;
};