#include "cbase.h"

#include "hud_speedometer_label.h"

#include <vgui_controls/AnimationController.h>

#include "hud_comparisons.h"
#include "iclientmode.h"
#include "momentum/util/mom_util.h"

#include "tier0/memdbgon.h"

// 1 unit = 19.05mm -> 0.01905m -> 0.00001905Km(/s) -> 0.06858Km(/h)
#define UPS_TO_KMH_FACTOR 0.06858f
// 1 unit = 0.75", 1 mile = 63360. 0.75 / 63360 ~~> 0.00001184"(/s) ~~> 0.04262MPH
#define UPS_TO_MPH_FACTOR 0.04262f

// velocity has to change this much before it is colored as increase/decrease
#define COLORIZE_DEADZONE 0.15f

using namespace vgui;

SpeedometerLabel::SpeedometerLabel(Panel *parent, const char *panelName, SpeedometerColorize_t colorizeType,
                                   bool supportsEnergyUnits, char *animationName, float *animationAlpha)
    : Label(parent, panelName, ""), m_eColorizeType(colorizeType), m_pFlAlpha(animationAlpha),
      m_pSzAnimationName(animationName), m_bSupportsEnergyUnits(supportsEnergyUnits), m_bDoneFading(false),
      m_eUnitType(SPEEDOMETER_UNITS_UPS), m_pRangeList(nullptr), m_bDrawComparison(true), m_cvarGravity("sv_gravity")
{
    CreateComparisonLabel();
    Reset();
}

// creates a separate label for the comparison & pins it to this
void SpeedometerLabel::CreateComparisonLabel()
{
    char name[BUFSIZELOCL];
    Q_snprintf(name, sizeof(name), "%sComparison", GetName());
    m_pComparisonLabel = new Label(GetParent(), name, "");
    m_pComparisonLabel->SetName(name);
    m_pComparisonLabel->SetPos(0, 0);
    m_pComparisonLabel->PinToSibling(GetName(), PIN_TOPLEFT, PIN_TOPRIGHT);
    m_pComparisonLabel->SetFont(GetFont());
    m_pComparisonLabel->SetAutoTall(true);
    m_pComparisonLabel->SetAutoWide(true);
    m_pComparisonLabel->SetContentAlignment(a_center);
    m_pComparisonLabel->SetPinCorner(PIN_TOPLEFT, 0, 0);
}

void SpeedometerLabel::ApplySchemeSettings(IScheme *pScheme)
{
    BaseClass::ApplySchemeSettings(pScheme);
    m_NormalColor = GetSchemeColor("MOM.Speedometer.Normal", pScheme);
    m_IncreaseColor = GetSchemeColor("MOM.Speedometer.Increase", pScheme);
    m_DecreaseColor = GetSchemeColor("MOM.Speedometer.Decrease", pScheme);
    m_pComparisonLabel->SetFont(GetFont());
}

void SpeedometerLabel::PerformLayout()
{
    BaseClass::PerformLayout();

    // keep centered in parent
    char szMain[BUFSIZELOCL];
    GetText(szMain, BUFSIZELOCL);

    HFont labelFont = GetFont();
    int combinedLength = UTIL_ComputeStringWidth(labelFont, szMain);

    if (m_eColorizeType == SPEEDOMETER_COLORIZE_COMPARISON_SEPARATE && m_bDrawComparison)
    {
        char szComparison[BUFSIZELOCL];
        m_pComparisonLabel->GetText(szComparison, BUFSIZELOCL);
        combinedLength += UTIL_ComputeStringWidth(labelFont, szComparison);
    }

    int xOffset = (GetParent()->GetWide() - combinedLength) / 2;
    SetPos(xOffset, GetYPos());
}

void SpeedometerLabel::OnThink()
{
    if (HasFadeOutAnimation() && !m_bDoneFading)
    {
        SetAlpha(*m_pFlAlpha);
        m_pComparisonLabel->SetAlpha(*m_pFlAlpha);

        if (CloseEnough(*m_pFlAlpha, 0.0f, FLT_EPSILON))
        {
            m_bDoneFading = true;
            Reset();
        }
    }
}

void SpeedometerLabel::Reset()
{
    m_flCurrentValue = 0.0f;
    m_flPastValue = 0.0f;
    m_flDiff = 0.0f;

    m_bCustomDiff = false;
    m_bDrawComparison = true;
    m_bDoneFading = true;

    m_pComparisonLabel->SetText("");
    BaseClass::SetText("");

    SetAlpha(HasFadeOutAnimation() ? 0 : 255);
}

void SpeedometerLabel::SetText(int value)
{
    char szValue[BUFSIZELOCL];
    Q_snprintf(szValue, sizeof(szValue), "%i", value);
    BaseClass::SetText(szValue);
}

void SpeedometerLabel::Update(float value)
{
    m_flCurrentValue = value;

    ConvertUnits();
    SetText(m_flCurrentValue);
    Colorize();
    m_bDoneFading = HasFadeOutAnimation() ? !StartFadeout() : false;

    m_flPastValue = m_flCurrentValue;
}

void SpeedometerLabel::ConvertUnits()
{
    if (m_eUnitType == SPEEDOMETER_UNITS_UPS)
        return;

    switch (m_eUnitType)
    {
    case SPEEDOMETER_UNITS_KMH:
        m_flCurrentValue *= UPS_TO_KMH_FACTOR;
        m_flDiff *= UPS_TO_KMH_FACTOR;
        break;
    case SPEEDOMETER_UNITS_MPH:
        m_flCurrentValue *= UPS_TO_MPH_FACTOR;
        m_flDiff *= UPS_TO_MPH_FACTOR;
        break;
    case SPEEDOMETER_UNITS_ENERGY:
        // outlier as it requires more than just 1 velocity
        if (m_bSupportsEnergyUnits)
        {
            const auto gravity = m_cvarGravity.GetFloat();
            const auto pPlayer = C_MomentumPlayer::GetLocalMomPlayer();
            Vector velocity = pPlayer->GetAbsVelocity();
            const auto pEntData = pPlayer->GetCurrentUIEntData();
            if (!pEntData)
                break;

            m_flCurrentValue =
                (velocity.LengthSqr() / 2.0f + gravity * (pPlayer->GetLocalOrigin().z - pEntData->m_flLastJumpZPos)) /
                gravity;
        }
        break;
    case SPEEDOMETER_UNITS_UPS:
    default:
        break;
    }
}

void SpeedometerLabel::Colorize()
{
    switch (m_eColorizeType)
    {
    case SPEEDOMETER_COLORIZE_NONE:
    {
        SetFgColor(m_NormalColor);
    } break;
    case SPEEDOMETER_COLORIZE_RANGE:
    {
        int roundedCurrentValue = RoundFloatToInt(m_flCurrentValue);
        Color newColor = m_NormalColor;
        if (m_pRangeList)
        {
            for (auto i = 0; i < m_pRangeList->Count(); i++)
            {
                Range_t range = (*m_pRangeList)[i];
                if (roundedCurrentValue >= range.min && roundedCurrentValue <= range.max)
                {
                    newColor = range.color;
                    break;
                }
            }
        }
        SetFgColor(newColor);
    }
    break;
    case SPEEDOMETER_COLORIZE_COMPARISON:
    {
        if (m_eUnitType == SPEEDOMETER_UNITS_ENERGY && m_bSupportsEnergyUnits)
        {
            m_flDiff = m_flCurrentValue - m_flPastValue;
        }
        else
        {
            m_flDiff = fabs(m_flCurrentValue) - fabs(m_flPastValue);
        }
        SetFgColor(MomUtil::GetColorFromVariation(m_flDiff, COLORIZE_DEADZONE, m_NormalColor, m_IncreaseColor,
                                                  m_DecreaseColor));
    }
    break;
    case SPEEDOMETER_COLORIZE_COMPARISON_SEPARATE:
    {
        SetFgColor(m_NormalColor);
        if (m_bDrawComparison)
        {
            if (!m_bCustomDiff) // calculate diff unless explicity told not to
            {
                m_flDiff = fabs(m_flCurrentValue) - fabs(m_flPastValue);
            }
            m_bCustomDiff = false;

            Color compareColor;
            g_pMOMRunCompare->GetDiffColor(m_flDiff, &compareColor, true);

            char diffChar = m_flDiff > 0.0f ? '+' : '-';
            char szText[BUFSIZELOCL];
            Q_snprintf(szText, BUFSIZELOCL, " (%c %i)", diffChar, RoundFloatToInt(fabs(m_flDiff)));

            m_pComparisonLabel->SetText(szText);
            m_pComparisonLabel->SetFgColor(compareColor);
        }
    }
    break;
    default:
        break;
    }
}

bool SpeedometerLabel::StartFadeout()
{
    Assert(HasFadeOutAnimation());
    g_pClientMode->GetViewportAnimationController()->StopAnimationSequence(GetParent(), m_pSzAnimationName);
    *m_pFlAlpha = 255.0f;
    return g_pClientMode->GetViewportAnimationController()->StartAnimationSequence(m_pSzAnimationName);
}
