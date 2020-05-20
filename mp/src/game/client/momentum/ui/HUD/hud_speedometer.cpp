#include "cbase.h"

#include "hud_speedometer.h"

#include "baseviewport.h"
#include "hud_comparisons.h"
#include "hud_macros.h"
#include "iclientmode.h"
#include "utlvector.h"
#include "filesystem.h"

#include <vgui/ILocalize.h>
#include <vgui/IScheme.h>
#include <vgui/ISurface.h>
#include <vgui_controls/Panel.h>
#include <vgui_controls/AnimationController.h>

#include "mom_player_shared.h"
#include "mom_system_gamemode.h"
#include "mom_shareddefs.h"
#include "momentum/util/mom_util.h"
#include "c_mom_replay_entity.h"

#include "c_baseplayer.h"
#include "movevars_shared.h"
#include "run/run_compare.h"

#include "tier0/memdbgon.h"

using namespace vgui;

static MAKE_TOGGLE_CONVAR(mom_hud_speedometer, "1", FLAG_HUD_CVAR | FCVAR_CLIENTCMD_CAN_EXECUTE,
                          "Toggles displaying the speedometer. 0 = OFF, 1 = ON\n");


static MAKE_CONVAR(mom_hud_speedometer_units, "0", FLAG_HUD_CVAR | FCVAR_CLIENTCMD_CAN_EXECUTE,
                   "Changes the units of measurement of the speedometer.\n 0 = Units per second\n 1 = "
                   "Kilometers per hour\n 2 = Miles per hour\n 3 = Energy", 0, 3);

static MAKE_CONVAR(mom_hud_speedometer_colorize, "0", FLAG_HUD_CVAR | FCVAR_CLIENTCMD_CAN_EXECUTE,
                   "Toggles speedometer colorization. 0 = OFF, 1 = ON (Based on Range), 2 = ON (Based on comparison), "
                   "3 = ON (Based on comparison which is written to a separate label)\n", 0, 3);

static MAKE_TOGGLE_CONVAR(mom_hud_speedometer_unit_labels, "0", FLAG_HUD_CVAR | FCVAR_CLIENTCMD_CAN_EXECUTE,
                          "Toggles showing the unit value labels (KM/H, MPH, Energy, UPS). 0 = OFF, 1 = ON\n");

static MAKE_TOGGLE_CONVAR(mom_hud_speedometer_showenterspeed, "1", FLAG_HUD_CVAR | FCVAR_CLIENTCMD_CAN_EXECUTE,
    "Toggles showing the stage/checkpoint enter speed (and comparison, if existent). 0 = OFF, 1 = ON\n");

static MAKE_TOGGLE_CONVAR(mom_hud_speedometer_horiz, "1", FLAG_HUD_CVAR | FCVAR_CLIENTCMD_CAN_EXECUTE,
                          "Toggles displaying the speedometer. 0 = OFF, 1 = ON\n");

static MAKE_TOGGLE_CONVAR(mom_hud_speedometer_lastjumpvel, "1", FLAG_HUD_CVAR | FCVAR_CLIENTCMD_CAN_EXECUTE,
                          "Toggles showing player velocity at last jump (XY only). 0 = OFF, 1 = ON\n");

static MAKE_TOGGLE_CONVAR(mom_hud_velocity_type, "0", FLAG_HUD_CVAR | FCVAR_CLIENTCMD_CAN_EXECUTE,
                   "Toggles the velocity type used in comparisons and map finished dialog. 0 = ABSOLUTE, 1 = HORIZONTAL\n");

CON_COMMAND_F(mom_hud_speedometer_savecfg, "Writes the current speedometer setup for the current gamemode to file.\n"
                                           "Optionally takes in the gamemode to write to.\n",
              FLAG_HUD_CVAR | FCVAR_CLIENTCMD_CAN_EXECUTE)
{
    if (args.ArgC() > 1)
    {
        for (auto i = 0; i < sizeof(g_szGameModeIdentifiers) / sizeof(*g_szGameModeIdentifiers); i++)
        {
            if (!Q_strcasecmp(args[1], g_pVGuiLocalize->FindAsUTF8(g_szGameModeIdentifiers[i])))
            {
                g_pSpeedometer->SaveGamemodeData(g_pGameModeSystem->GetGameMode(i + 1)->GetType());
                return;
            }
        }
    }
    else
    {
        g_pSpeedometer->SaveGamemodeData(g_pGameModeSystem->GetGameMode()->GetType());
    }
}

CON_COMMAND_F(mom_hud_speedometer_loadcfg, "Loads the speedometer setup for the current gamemode from file.\n"
                                           "Optionally takes in the gamemode to load from.",
              FLAG_HUD_CVAR | FCVAR_CLIENTCMD_CAN_EXECUTE)
{
    if (args.ArgC() > 1)
    {
        for (auto i = 0; i < sizeof(g_szGameModeIdentifiers) / sizeof(*g_szGameModeIdentifiers); i++)
        {
            if (!Q_strcasecmp(args[1], g_pVGuiLocalize->FindAsUTF8(g_szGameModeIdentifiers[i])))
            {
                g_pSpeedometer->LoadGamemodeData(g_pGameModeSystem->GetGameMode(i + 1)->GetType());
                return;
            }
        }
    }
    else
    {
        g_pSpeedometer->LoadGamemodeData(g_pGameModeSystem->GetGameMode()->GetType());
    }
}

#define SPEEDOMETER_FILENAME "cfg/speedometer.vdf"
#define SPEEDOMETER_DEFAULT_FILENAME "cfg/speedometer_default.vdf"

static CHudElement *Create_CHudSpeedMeter(void)
{
    auto pPanel = new CHudSpeedMeter("HudSpeedMeter");
    g_pSpeedometer = pPanel;
    return pPanel;
};
static CHudElementHelper g_CHudSpeedMeter_Helper(Create_CHudSpeedMeter, 50);

CHudSpeedMeter *g_pSpeedometer = nullptr;

CHudSpeedMeter::CHudSpeedMeter(const char *pElementName)
    : CHudElement(pElementName), EditablePanel(g_pClientMode->GetViewport(), "HudSpeedMeter"), 
    m_cvarTimeScale("mom_replay_timescale"), m_pGamemodeSetupData(nullptr), m_pRunStats(nullptr), m_pRunEntData(nullptr),
    m_CurrentlyLoadedGamemodeSettings(GAMEMODE_UNKNOWN)
{
    ListenForGameEvent("zone_exit");
    ListenForGameEvent("zone_enter");
    ListenForGameEvent("player_jumped");
    
    SetProportional(true);
    SetKeyBoardInputEnabled(false);
    SetMouseInputEnabled(false);
    SetHiddenBits(HIDEHUD_LEADERBOARDS);

    m_pAbsSpeedoLabel = new SpeedometerLabel(this, "AbsSpeedometer", SPEEDOMETER_COLORIZE_COMPARISON, true);
    m_pHorizSpeedoLabel = new SpeedometerLabel(this, "HorizSpeedometer", SPEEDOMETER_COLORIZE_COMPARISON);
    m_pLastJumpVelLabel = new SpeedometerLabel(this, "LastJumpVelocity", SPEEDOMETER_COLORIZE_COMPARISON, 
                                               false, "FadeOutLastJumpVel", &m_fLastJumpVelAlpha);
    m_pStageEnterExitVelLabel = new SpeedometerLabel(this, "StageEnterExitVelocity", SPEEDOMETER_COLORIZE_COMPARISON_SEPARATE,
                                            false, "FadeOutStageVel", &m_fStageVelAlpha);

    m_Labels[0] = m_pAbsSpeedoLabel;
    m_Labels[1] = m_pHorizSpeedoLabel;
    m_Labels[2] = m_pLastJumpVelLabel;
    m_Labels[3] = m_pStageEnterExitVelLabel;

    ResetLabelOrder();

    LoadControlSettings("resource/ui/Speedometer.res");
}

void CHudSpeedMeter::Init()
{
    m_pRunStats = nullptr;
    m_pRunEntData = nullptr;
}

void CHudSpeedMeter::Reset()
{
    m_pRunStats = nullptr;
    m_pRunEntData = nullptr;
    LoadGamemodeData(g_pGameModeSystem->GetGameMode()->GetType());
}

void CHudSpeedMeter::FireGameEvent(IGameEvent *pEvent)
{
    C_MomentumPlayer *pLocal = C_MomentumPlayer::GetLocalMomPlayer();

    if (pLocal)
    {
        if (FStrEq(pEvent->GetName(), "player_jumped") && m_pRunEntData)
        {
            m_pLastJumpVelLabel->Update(m_pRunEntData->m_flLastJumpVel);
            return;
        }

        if (!m_pRunStats || !m_pRunEntData)
            return;

        // zone enter/exit
        const auto ent = pEvent->GetInt("ent");
        if (ent == pLocal->GetCurrentUIEntity()->GetEntIndex())
        {
            const auto bExit = FStrEq(pEvent->GetName(), "zone_exit");
            const auto bLinear = pLocal->m_iLinearTracks.Get(pLocal->m_Data.m_iCurrentTrack);

            // Logical XOR; equivalent to (bLinear && !bExit) || (!bLinear && bExit)
            if (m_pRunEntData->m_bTimerRunning && (bLinear != bExit))
            {
                int velType = mom_hud_velocity_type.GetInt();
                float act = m_pRunStats->GetZoneEnterSpeed(m_pRunEntData->m_iCurrentZone, velType);
                bool bComparisonLoaded = g_pMOMRunCompare->LoadedComparison();
                if (bComparisonLoaded)
                {
                    // set the label's custom diff
                    float diff = act - g_pMOMRunCompare->GetRunComparisons()->runStats.GetZoneEnterSpeed(
                                            m_pRunEntData->m_iCurrentZone, velType);
                    m_pStageEnterExitVelLabel->SetCustomDiff(diff);
                }
                m_pStageEnterExitVelLabel->SetDrawComparison(bComparisonLoaded);
                m_pStageEnterExitVelLabel->Update(act);
            }
            else if (m_pRunEntData->m_bIsInZone && m_pRunEntData->m_iCurrentZone == 1 && FStrEq(pEvent->GetName(), "zone_enter"))
            {   // disappear when entering start zone
                m_fLastJumpVelAlpha = 0.0f;
                m_fStageVelAlpha = 0.0f;
            }
        }
    }
}

void CHudSpeedMeter::ApplySchemeSettings(IScheme *pScheme)
{
    BaseClass::ApplySchemeSettings(pScheme);
    m_NormalColor = GetSchemeColor("MOM.Speedometer.Normal", pScheme);
    m_IncreaseColor = GetSchemeColor("MOM.Speedometer.Increase", pScheme);
    m_DecreaseColor = GetSchemeColor("MOM.Speedometer.Decrease", pScheme);
    SetFgColor(GetSchemeColor("MOM.Panel.Fg", pScheme));
    SetBgColor(m_bgColor);
}

void CHudSpeedMeter::PerformLayout()
{
    int iHeightAcc = 0;
    for (auto i = 0; i < m_LabelOrderList.Count(); i++)
    {
        SpeedometerLabel *pLabel = m_LabelOrderList[i];
        if (pLabel->IsVisible())
        {
            pLabel->SetPos(pLabel->GetXPos(), iHeightAcc);
            iHeightAcc += surface()->GetFontTall(pLabel->GetFont());
        }
    }
    SetTall(iHeightAcc);
}

void CHudSpeedMeter::OnThink()
{
    const auto pPlayer = C_MomentumPlayer::GetLocalMomPlayer();
    if (!pPlayer)
        return;

    m_pRunEntData = pPlayer->GetCurrentUIEntData();
    m_pRunStats = pPlayer->GetCurrentUIEntStats();
    // Note: Velocity is also set to the player when watching first person
    Vector velocity = pPlayer->GetAbsVelocity();

    if (pPlayer->IsObserver() && pPlayer->GetCurrentUIEntity()->GetEntType() == RUN_ENT_REPLAY)
    {
        const float fReplayTimeScale = m_cvarTimeScale.GetFloat();
        if (fReplayTimeScale < 1.0f)
        {
            velocity /= fReplayTimeScale;
        }
    }

    float absVel = static_cast<float>(velocity.Length());
    float horizVel = static_cast<float>(Vector(velocity.x, velocity.y, 0).Length());

    m_pAbsSpeedoLabel->Update(absVel);
    m_pHorizSpeedoLabel->Update(horizVel);
}

void CHudSpeedMeter::LoadGamemodeData(GameMode_t gametype)
{
    if (m_pGamemodeSetupData)
        m_pGamemodeSetupData->deleteThis();

    m_pGamemodeSetupData = new KeyValues("Gamemodes");

    if (!g_pFullFileSystem->FileExists(SPEEDOMETER_FILENAME))
    {
        KeyValues *pKVTemp = new KeyValues(SPEEDOMETER_FILENAME);
        pKVTemp->LoadFromFile(g_pFullFileSystem, SPEEDOMETER_DEFAULT_FILENAME, "MOD");
        pKVTemp->SaveToFile(g_pFullFileSystem, SPEEDOMETER_FILENAME, "MOD");
        pKVTemp->deleteThis();
    }

    m_pGamemodeSetupData->LoadFromFile(g_pFullFileSystem, SPEEDOMETER_FILENAME, "MOD");

    KeyValues *pGamemodeKV = GetGamemodeKVs(gametype);
    if (pGamemodeKV) // game mode is known
    {
        m_CurrentlyLoadedGamemodeSettings = gametype;

        // get ordering
        KeyValues *pOrderKV = pGamemodeKV->FindKey("order");
        if (pOrderKV)
        {
            m_LabelOrderList.RemoveAll();
            char szIndex[BUFSIZELOCL];
            for (unsigned int i = 1; i <= SPEEDOMETER_MAX_LABELS; i++)
            {
                Q_snprintf(szIndex, BUFSIZELOCL, "%i", i);
                const char *szSpeedo = pOrderKV->GetString(szIndex);
                if (szSpeedo)
                {
                    SpeedometerLabel *label = GetLabelFromName(szSpeedo);
                    if (label)
                    {
                        m_LabelOrderList.AddToTail(label);
                    }
                }
            }
        }
        else
        {
            ResetLabelOrder();
        }

        // get speedometer label setups
        for (KeyValues *pSpeedoItem = pGamemodeKV->GetFirstSubKey(); pSpeedoItem != nullptr; pSpeedoItem = pSpeedoItem->GetNextKey())
        {
            const char *speedoname = pSpeedoItem->GetName();
            SpeedometerLabel *pLabel = GetLabelFromName(speedoname);
            KeyValues *pSpeedoKV = pGamemodeKV->FindKey(speedoname);
            if (!pLabel || !pSpeedoKV)
                continue;

            pLabel->SetVisible(pSpeedoKV->GetBool("visible", false));
            if (!pLabel->IsVisible())
                continue;

            int colorize = pSpeedoKV->GetInt("colorize", 0);
            if (colorize == SPEEDOMETER_COLORIZE_RANGE)
            {
                KeyValues *pRangesKV = pSpeedoKV->FindKey("ranges");
                if (!pRangesKV)
                    continue;

                SPEEDOMETER_RANGE_LIST_T *m_pRangeList = new SPEEDOMETER_RANGE_LIST_T();
                for (KeyValues *pRangeItem = pRangesKV->GetFirstSubKey(); pRangeItem != nullptr; pRangeItem = pRangeItem->GetNextKey())
                {
                    Range_t range;
                    range.min = pRangeItem->GetInt("min", 0);
                    range.max = pRangeItem->GetInt("max", 0);
                    range.color = pRangeItem->GetColor("color");
                    m_pRangeList->AddToTail(range);
                }
                pLabel->SetRangeList(m_pRangeList);
            }
            pLabel->SetColorizeType(colorize);
            pLabel->SetUnitType(pSpeedoKV->GetInt("units", 0));
        }
    }
}

void CHudSpeedMeter::SaveGamemodeData(GameMode_t gametype)
{
    if (!g_pFullFileSystem)
        return;

    KeyValues *pGamemodeKV = GetGamemodeKVs(gametype);
    if (pGamemodeKV) // game mode is known
    {
        // set speedometer label setups
        for (KeyValues *pSpeedoItem = pGamemodeKV->GetFirstSubKey(); pSpeedoItem != nullptr; pSpeedoItem = pSpeedoItem->GetNextKey())
        {
            const char *speedoname = pSpeedoItem->GetName();
            SpeedometerLabel *pLabel = GetLabelFromName(speedoname);
            KeyValues *pSpeedoKV = pGamemodeKV->FindKey(speedoname, true);
            if (!pLabel || !pSpeedoKV)
                continue;

            pSpeedoKV->Clear();
            pSpeedoKV->SetBool("visible", pLabel->IsVisible());
            pSpeedoKV->SetInt("colorize", pLabel->GetColorizeType());
            SPEEDOMETER_RANGE_LIST_T *pRangeList = pLabel->GetRangeList();
            if (pRangeList)
            {
                KeyValues *pRangesKV = pSpeedoKV->FindKey("ranges", true);
                pRangesKV->Clear();
                char tmp[BUFSIZELOCL];
                for (auto i = 0; i < pRangeList->Count(); i++)
                {
                    Range_t *range = &(*pRangeList)[i];
                    Q_snprintf(tmp, BUFSIZELOCL, "%i", i + 1);
                    KeyValues *rangeKV = pRangesKV->FindKey(tmp, true);
                    rangeKV->SetInt("min", range->min);
                    rangeKV->SetInt("max", range->max);
                    rangeKV->SetColor("color", range->color);
                }
            }
            pSpeedoKV->SetInt("units", pLabel->GetUnitType());
        }

        // set ordering
        KeyValues *pOrderKVs = pGamemodeKV->FindKey("order", true);
        pOrderKVs->Clear();
        char tmpBuf[BUFSIZELOCL];
        for (auto i = 0; i < m_LabelOrderList.Count(); i++)
        {
            Q_snprintf(tmpBuf, BUFSIZELOCL, "%i", i + 1);
            pOrderKVs->SetString(tmpBuf, m_LabelOrderList[i]->GetName());
        }

        m_pGamemodeSetupData->SaveToFile(g_pFullFileSystem, SPEEDOMETER_FILENAME, "MOD");
    }
}

SpeedometerLabel *CHudSpeedMeter::GetLabelFromName(const char *name) 
{
    for (int i = 0; i < SPEEDOMETER_MAX_LABELS; i++)
    {
        if (!Q_strcmp(m_Labels[i]->GetName(), name))
            return m_Labels[i];
    }
    return nullptr;
}

KeyValues* CHudSpeedMeter::GetGamemodeKVs(GameMode_t gametype)
{
    KeyValues *pGamemodeKV = nullptr;
    switch (gametype)
    {
    case GAMEMODE_UNKNOWN:
    case GAMEMODE_SURF:
        pGamemodeKV = m_pGamemodeSetupData->FindKey("Surf");
        break;
    case GAMEMODE_BHOP:
        pGamemodeKV = m_pGamemodeSetupData->FindKey("Bhop");
        break;
    case GAMEMODE_RJ:
        pGamemodeKV = m_pGamemodeSetupData->FindKey("RJ");
        break;
    case GAMEMODE_SJ:
        pGamemodeKV = m_pGamemodeSetupData->FindKey("SJ");
        break;
    default:
        break;
    }
    return pGamemodeKV;
}

void CHudSpeedMeter::ResetLabelOrder()
{
    m_LabelOrderList.RemoveAll();
    for (int i = 0; i < SPEEDOMETER_MAX_LABELS; i++)
    {
        m_LabelOrderList.AddToTail(m_Labels[i]);
    }
}
