#include "global.h"
#include "LifeMeterTime.h"
#include "ThemeManager.h"
#include "Song.h"
#include "Steps.h"
#include "ActorUtil.h"
#include "Course.h"
#include "Preference.h"
#include "StreamDisplay.h"
#include "GameState.h"
#include "StatsManager.h"
#include "PlayerState.h"
#include "MessageManager.h"
// Custom
#include "TimingData.h"

static ThemeMetric<float> MIN_LIFE_TIME		("LifeMeterTime2","MinLifeTime");
static ThemeMetric<float> MAX_TIME			("LifeMeterTime2","MaxLifeTime");
static ThemeMetric<float> CONSTANT_BONUS	("LifeMeterTime2","ConstantBonus");
static ThemeMetric<LuaReference> GAIN_PER_TAP_FUNC	("LifeMeterTime2","GainPerTapFunc");
static ThemeMetric<LuaReference> FILL_NO_TIMES_FUNC	("LifeMeterTime2","FillNoTimesFunc");
static ThemeMetric<float> ALLOW_LOWER_GAIN_PER_TAP	("LifeMeterTime2","AllowLowerGainPerTap");
						// 0.0f = false,	1.0f = true
// From LifeMeterTime
static ThemeMetric<float> METER_WIDTH;		("LifeMeterTime","MeterWidth");
static ThemeMetric<float> METER_HEIGHT;		("LifeMeterTime","MeterHeight");
static ThemeMetric<float> DANGER_THRESHOLD;	("LifeMeterTime","DangerThreshold");
static ThemeMetric<float> INITIAL_VALUE;		("LifeMeterTime","InitialValue");


// This implementation makes LifeMeterTime to behave (a bit) similiar to osu! HP bar.

extern Preference1D<float>	g_fTimeMeterSecondsChange;

static const float g_fColsPosFactor[] =
{
	1.0f,
	1.0f, // 1 col
	1.1f, // 2 cols
	1.2f, // 3 cols
	1.3f, // ...
	1.4f,
	1.5f,
	1.6f
};

static const float g_fColsNegFactor[] =
{
	1.0f,
	1.0f, // 1 col
	1.06f, // 2 cols
	1.11f, // 3 cols
	1.13f,
	1.15f,
	1.18f,
	1.21f
};


LifeMeterTime2::LifeMeterTime2()
	: LifeMeterTime()
{
	m_maxSeconds = MAX_TIME;
	for(int i=0; i<12; i++) m_customlifechange[i] = g_fTimeMeterSecondsChange[i];
	//m_fLifeTotalGainedSeconds = 0;	// Private
	//m_fLifeTotalLostSeconds = 0;	// Private
	//Custom
	m_fSongTotalStopSeconds = 0;
	m_fCurrentCummulativeStop = 0;
	m_bLockLife = false;
	
	//m_pStream = NULL; // Private
}

void LifeMeterTime2::Load( const PlayerState *pPlayerState, PlayerStageStats *pPlayerStageStats )
{
	//SetCustomLifeChange();
	LifeMeterTime::Load( pPlayerState, pPlayerStageStats );
}

void LifeMeterTime2::OnLoadSong()
{
	if( GetLifeSeconds() <= 0 && GAMESTATE->GetCourseSongIndex() > 0 )
		return;

	float fOldLife = m_fLifeTotalLostSeconds;
	float fGainSeconds = 0;
	if(GAMESTATE->IsCourseMode())
	{
		Course* pCourse = GAMESTATE->m_pCurCourse;
		ASSERT( pCourse != NULL );
		fGainSeconds= pCourse->m_vEntries[GAMESTATE->GetCourseSongIndex()].fGainSeconds;
	}

	float fMaxGainPerTap;
	Song* song= GAMESTATE->m_pCurSong;
	m_fLifeTotalLostSeconds -= m_fSongTotalStopSeconds;
	fillNoTimes();
	m_firstSecondCummulativeStop = getCummulativeStopSecs(song->GetFirstSecond());
	m_fSongTotalStopSeconds =
		getCummulativeStopSecs(song->GetLastSecond()) - m_firstSecondCummulativeStop;
	
	if (GAIN_PER_TAP_FUNC.IsLoaded()) {
		// Call the function if is not nil
		Lua *L= LUA->Get();
		GAIN_PER_TAP_FUNC.PushSelf(L);
		PushSelf(L);
		LuaHelpers::Push(L, m_pPlayerState->m_PlayerNumber);
		RString error= "Error running GainPerTapFunc callback: ";
		LuaHelpers::RunScriptOnStack(L, error, 2, 1, true);
		fMaxGainPerTap = luaL_optnumber(L, -1, -1);
		lua_settop(L, 0);
		LUA->Release(L);
	}
	if (fMaxGainPerTap <= -1) {
		ASSERT(song != NULL);
		float song_len= song->GetLastSecond() - song->GetFirstSecond() - m_firstSecondCummulativeStop;
		Steps* steps= GAMESTATE->m_pCurSteps[m_pPlayerState->m_PlayerNumber];
		ASSERT(steps != NULL);
		RadarValues radars= steps->GetRadarValues(m_pPlayerState->m_PlayerNumber);
		float scorable_things= radars[RadarCategory_TapsAndHolds] + radars[RadarCategory_Lifts];
	
		fMaxGainPerTap = song_len/scorable_things;
		// fGainPerTap = fGainPerTap*(1.232f+(float)scorable_things/8000.0f)+(0.008f*min(song_len,240)/60);
		fMaxGainPerTap = fGainPerTap*(CONSTANT_BONUS+(float)scorable_things/8000.0f)+(0.007f*min(song_len,240)/60);
		
	}
	m_customlifechange[SE_W1] = fGainPerTap;
	m_customlifechange[SE_W2] = fGainPerTap*0.5f;
	m_customlifechange[SE_W3] = fGainPerTap*0.02f;
	
	for(int i=0; i<12; i++) {
		float fGainPerTap = m_customlifechange[i];
		if (fGainPerTap > 0) {
			float fDeltaCustom = g_fTimeMeterSecondsChange[i] - fGainPerTap;
			if (fDeltaCustom > 0) {
				m_customlifechange[i] = g_fTimeMeterSecondsChange[i] - fDeltaCustom*ALLOW_LOWER_GAIN_PER_TAP;
			}
		}
	}
	

	if( MIN_LIFE_TIME > fGainSeconds )
		fGainSeconds = MIN_LIFE_TIME;
	m_fLifeTotalGainedSeconds += fGainSeconds;
	m_soundGainLife.Play(false);
	SendLifeChangedMessage( fOldLife, TapNoteScore_Invalid, HoldNoteScore_Invalid );
}


void LifeMeterTime2::ChangeLife( TapNoteScore tns )
{	// The same as in the base class, but...
	if( GetLifeSeconds() <= 0 )
		return;

	float fMeterChange = 0;
	switch( tns )
	{
	default:
		FAIL_M(ssprintf("Invalid TapNoteScore: %i", tns));
	case TNS_W1:		fMeterChange = m_customlifechange[SE_W1];		break;
	case TNS_W2:		fMeterChange = m_customlifechange[SE_W2];		break;
	case TNS_W3:		fMeterChange = m_customlifechange[SE_W3];		break;
	case TNS_W4:		fMeterChange = m_customlifechange[SE_W4];		break;
	case TNS_W5:		fMeterChange = m_customlifechange[SE_W5];		break;
	case TNS_Miss:		fMeterChange = m_customlifechange[SE_Miss];		break;
	case TNS_HitMine:	fMeterChange = m_customlifechange[SE_HitMine];		break;
	case TNS_CheckpointHit:	fMeterChange = m_customlifechange[SE_CheckpointHit];	break;
	case TNS_CheckpointMiss:fMeterChange = m_customlifechange[SE_CheckpointMiss];	break;
	}

	float fOldLife = m_fLifeTotalLostSeconds;
	m_fLifeTotalLostSeconds -= fMeterChange;
	SendLifeChangedMessage( fOldLife, tns, HoldNoteScore_Invalid );
}


// Protected
float LifeMeterTime2::GetLifeSeconds() const
{
	float secs = m_fLifeTotalGainedSeconds - (m_fLifeTotalLostSeconds + STATSMAN->m_CurStageStats.m_fStepsSeconds);
	//float stoptime = ;
	secs += clamp(m_fCurrentCummulativeStop, 0, m_fSongTotalStopSeconds);
	return secs;
	//return m_fLifeTotalGainedSeconds - (m_fLifeTotalLostSeconds + STATSMAN->m_CurStageStats.m_fStepsSeconds);
}

