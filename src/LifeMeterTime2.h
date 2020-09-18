#ifndef LifeMeterTime2_H
#define LifeMeterTime2_H

#include "LifeMeter.h"
#include "Sprite.h"
#include "BitmapText.h"
#include "RageSound.h"
#include "PercentageDisplay.h"
#include "AutoActor.h"
#include "MeterDisplay.h"
#include "Quad.h"

// Custom
typedef struct {
	float length, divfactor;
} NoTimeZone;

class LifeMeterTime2: public LifeMeterTime
{
public:
	LifeMeterTime2();
	// virtual void Load( const PlayerState *pPlayerState, PlayerStageStats *pPlayerStageStats );
	virtual void Update( float fDeltaTime );

	virtual void OnLoadSong();
	virtual void OnSongEnded();
	virtual void ChangeLife( TapNoteScore score );
	virtual void ChangeLife( HoldNoteScore score, TapNoteScore tscore );
	// virtual void ChangeLife(float delta);
	// virtual void SetLife(float value);
	// virtual void HandleTapScoreNone();
	// virtual bool IsInDanger() const;
	// virtual bool IsHot() const;
	// virtual bool IsFailing() const;
	virtual float GetLife() const;
	
	virtual float GetLife_w_overload() const;
	
protected
	float GetLifeSeconds() const; // From parent class
private:
	float m_maxSeconds;
	
	float		m_customlifechange[12];
	void		SetCustomLifeChange();
	float		m_fSongTotalStopSeconds;
	map<float,NoTimeZone>	m_notimezones;
	float m_firstSecondCummulativeStop;
	float m_fCurrentCummulativeStop;
	bool m_bLockLife;
	void fillNoTimes();
	void AgregarStop( float timebeat, float length, float divfactor=1.0f );
	friend int lua_AgregarStop( lua_State *L );
	float getCummulativeStopSecs( float time ) const;
};


#endif
