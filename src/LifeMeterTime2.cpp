#include "global.h"
#include "LifeMeterTime2.h"
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
static ThemeMetric<float> METER_DIVIDER_ON_OVERLOAD	("LifeMeterTime2","MeterDividerOnOverload");

// From LifeMeterTime
static ThemeMetric<float> METER_WIDTH		("LifeMeterTime","MeterWidth");
static ThemeMetric<float> METER_HEIGHT		("LifeMeterTime","MeterHeight");
static ThemeMetric<float> DANGER_THRESHOLD	("LifeMeterTime","DangerThreshold");
static ThemeMetric<float> INITIAL_VALUE		("LifeMeterTime","InitialValue");


// This implementation makes LifeMeterTime to behave (a bit) similiar to osu! HP bar.
static const float g_fTimeMeter2SecondsChangeInit[] =
{
	+0.05f, // SE_CheckpointHit
	+0.22f, // SE_W1
	+0.112f, // SE_W2
	+0.04f, // SE_W3
	-0.18f, // SE_W4
	-1.3f, // SE_W5
	-2.25f, // SE_Miss
	-2.2f, // SE_HitMine
	-0.85f, // SE_CheckpointMiss
	+0.1f, // SE_Held
	-2.25f, // SE_LetGo
	-0.0f, // SE_Missed
};
COMPILE_ASSERT( ARRAYLEN(g_fTimeMeter2SecondsChangeInit) == NUM_ScoreEvent );

static void TimeMeter2SecondsChangeInit( size_t /*ScoreEvent*/ i, RString &sNameOut, float &defaultValueOut )
{
	sNameOut = "TimeMeter2SecondsChange" + ScoreEventToString( (ScoreEvent)i );
	defaultValueOut = g_fTimeMeter2SecondsChangeInit[i];
}

static Preference1D<float>	g_fTimeMeter2SecondsChange( TimeMeter2SecondsChangeInit, NUM_ScoreEvent );

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
	m_fMaxSeconds = MAX_TIME;
	for(int i=0; i<12; i++) m_customlifechange[i] = g_fTimeMeter2SecondsChange[i];
	//m_fLifeTotalGainedSeconds = 0;	// Private
	//m_fLifeTotalLostSeconds = 0;	// Private
	m_fSongTotalStopSeconds = 0;
	m_fCurrentCummulativeStop = 0;
	m_fDividerOnOverload = 2;
	m_bLockLife = false;
	
	//m_pStream = NULL; // Private
}

void LifeMeterTime2::Load( const PlayerState *pPlayerState, PlayerStageStats *pPlayerStageStats )
{
	LifeMeterTime::Load( pPlayerState, pPlayerStageStats );
}

void LifeMeterTime2::Update( float fDeltaTime )
{
	if (!m_bLockLife) {
		m_fCurrentCummulativeStop = getCummulativeStopSecs( (float)GAMESTATE->m_Position.m_fMusicSeconds ) - m_firstSecondCummulativeStop;
		
	}
	LifeMeterTime::Update( fDeltaTime );
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
		fMaxGainPerTap = fMaxGainPerTap*(CONSTANT_BONUS+(float)scorable_things/8000.0f)+(0.007f*min(song_len,240)/60);
		
	}
	m_customlifechange[SE_W1] = fMaxGainPerTap;
	m_customlifechange[SE_W2] = fMaxGainPerTap*0.5f;
	m_customlifechange[SE_W3] = fMaxGainPerTap*0.04f;
	printf("LifeMeterTime2::OnLoadSong: fMaxGainPerTap:\n %f\n %f\n %f\n",
		m_customlifechange[SE_W1], m_customlifechange[SE_W2], m_customlifechange[SE_W3]);
	
	for(int i=0; i<12; i++) {
		float fGainPerTap = m_customlifechange[i];
		if (fGainPerTap > 0) {
			float fDeltaCustom = g_fTimeMeter2SecondsChange[i] - fGainPerTap;
			if (fDeltaCustom > 0) {
				m_customlifechange[i] = g_fTimeMeter2SecondsChange[i] - fDeltaCustom*ALLOW_LOWER_GAIN_PER_TAP;
			}
		}
	}
	
	m_fDividerOnOverload = METER_DIVIDER_ON_OVERLOAD;

	if( MIN_LIFE_TIME > fGainSeconds )
		fGainSeconds = MIN_LIFE_TIME;
	m_fLifeTotalGainedSeconds += fGainSeconds;
	m_soundGainLife.Play(false);
	SendLifeChangedMessage( fOldLife, TapNoteScore_Invalid, HoldNoteScore_Invalid );
	m_bLockLife = false;
	
}

void LifeMeterTime2::OnSongEnded()
{
	m_bLockLife = true;
}

void LifeMeterTime2::ChangeLife( TapNoteScore tns, int nCol )
{
	float fLifeSeconds = GetLifeSeconds();
	if( fLifeSeconds <= 0 )
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
	
	if( tns != TNS_CheckpointHit && tns != TNS_CheckpointMiss ) {
		if (fMeterChange>0)
			fMeterChange *= g_fColsPosFactor[(nCol>7 ? 7 : (nCol<0? 0:nCol))];
		else
			fMeterChange *= g_fColsNegFactor[(nCol>7 ? 7 : (nCol<0? 0:nCol))];
	}
	
	fMeterChange = ChangeMeterChangeOnAboveMax( fMeterChange, fLifeSeconds );
	printf("LifeMeterTime2::ChangeLife: fMeterChange = %f\n", fMeterChange);
	float fOldLife = m_fLifeTotalLostSeconds;
	m_fLifeTotalLostSeconds -= fMeterChange;
	SendLifeChangedMessage( fOldLife, tns, HoldNoteScore_Invalid );
}

void LifeMeterTime2::ChangeLife( HoldNoteScore hns, TapNoteScore tns )
{
	float fLifeSeconds = GetLifeSeconds();
	if( fLifeSeconds <= 0 )
		return;

	float fMeterChange = 0;
	switch( hns )
	{
	default:
		FAIL_M(ssprintf("Invalid HoldNoteScore: %i", hns));
	case HNS_Held:	fMeterChange = g_fTimeMeter2SecondsChange[SE_Held];	break;
	case HNS_LetGo:	fMeterChange = g_fTimeMeter2SecondsChange[SE_LetGo];	break;
	case HNS_Missed:	fMeterChange = g_fTimeMeter2SecondsChange[SE_Missed];	break;
	}
	
	fMeterChange = ChangeMeterChangeOnAboveMax( fMeterChange, fLifeSeconds ); // :)

	float fOldLife = m_fLifeTotalLostSeconds;
	m_fLifeTotalLostSeconds -= fMeterChange;
	SendLifeChangedMessage( fOldLife, tns, hns );
}


float LifeMeterTime2::GetLife() const
{
	float fLifeWithOverload = GetLife_w_overload();
	return clamp( fLifeWithOverload, 0, 1 );
}

float LifeMeterTime2::GetLife_w_overload() const
{
	float fPercent = GetLifeSeconds() / m_fMaxSeconds;
	return ( fPercent < 0 ? 0 : fPercent );
}

// Protected
float LifeMeterTime2::GetLifeSeconds() const
{
	float secs = m_fLifeTotalGainedSeconds - (m_fLifeTotalLostSeconds + STATSMAN->m_CurStageStats.m_fStepsSeconds);
	secs += clamp(m_fCurrentCummulativeStop, 0, m_fSongTotalStopSeconds);
	return secs;
	//return m_fLifeTotalGainedSeconds - (m_fLifeTotalLostSeconds + STATSMAN->m_CurStageStats.m_fStepsSeconds);
}

float LifeMeterTime2::ChangeMeterChangeOnAboveMax( float fMeterChange, float fLifeSeconds )
{
	// Any meter change above MaxSeconds is divided by 2. Example:
	// [===========|_] 2.8s/3.0s; fMeterChange = 0.25
	//                 3.05s	is 0.05 above MaxSeconds. Divide the difference by 2.
	// [=============] 3.025s/3.0s; final fMeterChange = 0.225
	if ( fMeterChange > 0 ) {
		float fRemainingForMaxSeconds = m_fMaxSeconds - fLifeSeconds;
		if ( fRemainingForMaxSeconds < 0 ) { // if LifeSeconds is already above MaxSeconds.
			// Divide all the fMeterChange by 2.
			fMeterChange /= m_fDividerOnOverload;
		}
		else if (fRemainingForMaxSeconds < fMeterChange) {
			// If LifeSeconds is too low to get above MaxSeconds, do nothing.
			// Otherwhise, calculate the difference and divide it by 2.
			float fDiffAboveMax = fMeterChange - fRemainingForMaxSeconds;
			//fMeterChange -= fDiffAboveMax / m_fDividerOnOverload; // bad
			fMeterChange = fMeterChange - fDiffAboveMax + fDiffAboveMax / m_fDividerOnOverload;
			// Just in case that fMeterChange result negative by bad precision, set it to 0.
			fMeterChange = fMeterChange < 0 ? 0 : fMeterChange;
		}
	}
	return fMeterChange;
}



void LifeMeterTime2::AgregarStop( float timebeat, float length, float divfactor )
{
	NoTimeZone ntz = {length,divfactor};
	if ( m_notimezones.count(timebeat) > 0 && m_notimezones[timebeat].divfactor==divfactor ) {
		ntz.length += m_notimezones[timebeat].length;
	}
	m_notimezones[timebeat] = ntz;
}

int lua_AgregarStop( lua_State *L )
{
	LifeMeterTime2 *lmt2 = (LifeMeterTime2*)lua_touserdata(L, lua_upvalueindex(1));
	float timebeat = luaL_checknumber(L,1), amount=luaL_checknumber(L,2);
	float divfactor = luaL_optnumber(L,3, 1.0f);
	lmt2->AgregarStop( timebeat, amount, divfactor );
	return 0;
}

void LifeMeterTime2::fillNoTimes()
{
	TimingData* timingdata = GAMESTATE->m_pCurSteps[m_pPlayerState->m_PlayerNumber]->GetTimingData();
	m_notimezones.clear();
	bool done = false;
	{
		Lua *L = LUA->Get();
		FILL_NO_TIMES_FUNC.PushSelf(L);	// La función a llamar
		lua_pushlightuserdata(L, (void*)this);
		lua_pushcclosure(L, (lua_CFunction)lua_AgregarStop, 1);	// param 1: Funcion para agregar
		timingdata->PushSelf(L);	// param 2: TimingData
		LuaHelpers::Push(L, m_pPlayerState->m_PlayerNumber);	// param 3: PlayerNumber
		RString error= "Error running FillNoTimesFunc callback: ";
		done = LuaHelpers::RunScriptOnStack(L, error, 3, 0, true); // Llamar la función
			// La función devuelve una tabla
		if (!done) {
			m_notimezones.clear();
		}
		// 
		LUA->Release(L);
	}

	if (!done) {
		const vector<TimingSegment*> segs1 = timingdata->GetTimingSegments( SEGMENT_STOP );
		std::vector<TimingSegment*>::const_iterator it;
		for (it = segs1.begin() ; it != segs1.end(); ++it) {
			StopSegment* ts = (StopSegment*)(*it);
			float beat=ts->GetBeat(), length=ts->GetPause();
			float timebeat = timingdata->GetElapsedTimeFromBeat(beat);
			AgregarStop(timebeat,length);
		}

		const vector<TimingSegment*> segs2 = timingdata->GetTimingSegments( SEGMENT_DELAY );
		std::vector<TimingSegment*>::const_iterator it2;
		for (it2 = segs2.begin() ; it2 != segs2.end(); ++it2) {
			DelaySegment* ts = (DelaySegment*)(*it2);
			float beat=ts->GetBeat(), length=ts->GetPause();
			float timebeat = timingdata->GetElapsedTimeFromBeat(beat);
			AgregarStop(timebeat,length);
		}
	}
}

float LifeMeterTime2::getCummulativeStopSecs( float time ) const
{
	float total = 0;
	for (map<float,NoTimeZone>::const_iterator it = m_notimezones.begin(); it!=m_notimezones.end(); ++it) {
		float timebeat = it->first;
		NoTimeZone ntz = it->second;
		if (timebeat < time) {
			float difference = time-timebeat;
			total += min( difference, ntz.length ) / ntz.divfactor;
		}
	}
	return total;
}


// lua start
#include "LuaBinding.h"

/** @brief Allow Lua to have access to the LifeMeterTime2. */
class LunaLifeMeterTime2: public Luna<LifeMeterTime2>
{
public:
	static int GetLifeWithOverload( T* p, lua_State *L )
	{
		LuaHelpers::Push( L, p->GetLife_w_overload() );
		return 1;
	}

	LunaLifeMeterTime2()
	{
		ADD_METHOD( GetLifeWithOverload );
	}
};

LUA_REGISTER_DERIVED_CLASS( LifeMeterTime2, LifeMeter )

