#include "global.h"
#include "ScreenOptions.h"
#include "RageUtil.h"
#include "ScreenManager.h"
#include "PrefsManager.h"
#include "GameConstantsAndTypes.h"
#include "RageLog.h"
#include "GameState.h"
#include "ThemeManager.h"
#include "InputMapper.h"
#include "ActorUtil.h"
#include "ProfileManager.h"
#include "song.h"
#include "Course.h"
#include "Style.h"
#include "ScreenDimensions.h"
#include "Command.h"
#include "CommonMetrics.h"


/*
 * These navigation types are provided:
 *
 * All modes:
 *  left, right -> change option
 *  up, down -> change row
 *   (in toggle modes, focus on nearest item to old focus)
 *
 * NAV_THREE_KEY:
 *  start -> move to next row
 *  left+right+start -> move to prev row
 *  (next screen via "exit" entry)
 * This is the minimal navigation, for using menus with only three buttons.
 *
 * NAV_FIVE_KEY:
 *  start -> next screen
 * This is a much more convenient navigation, requiring five keys.
 *
 * NAV_TOGGLE_THREE_KEY:
 *  start -> on first choice, move to next row; otherewise toggle option and move to first choice
 *  left+right+start -> move to prev row
 *
 * NAV_TOGGLE_FIVE_KEY:
 *  start -> toggle option
 *
 * Regular modes and toggle modes must be enabled by the theme.  We could simply
 * automatically switch to toggle mode for multiselect rows, but that would be strange
 * for non-multiselect rows (eg. scroll speed).
 *
 * THREE_KEY modes are navigatable with only MenuLeft, MenuRight and MenuStart, and
 * are used when PREFSMAN->m_bArcadeOptionsNavigation is enabled.  However, they can
 * still use MenuUp and MenuDown for nonessential behavior.
 *
 * NAV_THREE_KEY_MENU:
 *  left, right -> change row
 *  up, down -> change row
 *  start -> next screen
 * This is a specialized navigation for ScreenOptionsMenu.  It must be enabled to
 * allow screens that use rows to select other screens to work with only three
 * buttons.  (It's also used when in five-key mode.)
 *
 * We don't want to simply allow left/right to move up and down on single-entry
 * rows when in NAV_THREE_KEY, becasue left and right shouldn't exit the "exit" row
 * in player options menus, but it should in the options menu.
 */

const float TWEEN_SECONDS = 0.3f;

static CString ROW_Y_NAME( size_t r )					{ return ssprintf("Row%dY",r+1); }
static CString EXPLANATION_X_NAME( size_t p )			{ return ssprintf("ExplanationP%dX",p+1); }
static CString EXPLANATION_Y_NAME( size_t p )			{ return ssprintf("ExplanationP%dY",p+1); }
static CString EXPLANATION_ON_COMMAND_NAME( size_t p )	{ return ssprintf("ExplanationP%dOnCommand",p+1); }


//REGISTER_SCREEN_CLASS( ScreenOptions );	// can't be instantiated
ScreenOptions::ScreenOptions( CString sClassName ) : ScreenWithMenuElements(sClassName),
	NUM_ROWS_SHOWN					(m_sName,"NumRowsShown"),
	ROW_Y							(m_sName,ROW_Y_NAME,NUM_ROWS_SHOWN),
	ROW_Y_OFF_SCREEN_TOP			(m_sName,"RowYOffScreenTop"),
	ROW_Y_OFF_SCREEN_CENTER			(m_sName,"RowYOffScreenCenter"),
	ROW_Y_OFF_SCREEN_BOTTOM			(m_sName,"RowYOffScreenBottom"),
	EXPLANATION_X					(m_sName,EXPLANATION_X_NAME,NUM_PLAYERS),
	EXPLANATION_Y					(m_sName,EXPLANATION_Y_NAME,NUM_PLAYERS),
	EXPLANATION_ON_COMMAND			(m_sName,EXPLANATION_ON_COMMAND_NAME,NUM_PLAYERS),
	EXPLANATION_TOGETHER_X			(m_sName,"ExplanationTogetherX"),
	EXPLANATION_TOGETHER_Y			(m_sName,"ExplanationTogetherY"),
	EXPLANATION_TOGETHER_ON_COMMAND	(m_sName,"ExplanationTogetherOnCommand"),
	SHOW_SCROLL_BAR					(m_sName,"ShowScrollBar"),
	SCROLL_BAR_HEIGHT				(m_sName,"ScrollBarHeight"),
	SCROLL_BAR_TIME					(m_sName,"ScrollBarTime"),
	EXPLANATION_ZOOM				(m_sName,"ExplanationZoom"),
	SHOW_BPM_IN_SPEED_TITLE			(m_sName,"ShowBpmInSpeedTitle"),
	FRAME_ON_COMMAND				(m_sName,"FrameOnCommand"),
	FRAME_OFF_COMMAND				(m_sName,"FrameOffCommand"),
	SHOW_EXIT_ROW					(m_sName,"ShowExitRow"),
	SEPARATE_EXIT_ROW				(m_sName,"SeparateExitRow"),
	SEPARATE_EXIT_ROW_Y				(m_sName,"SeparateExitRowY"),
	OPTION_ROW_TYPE					(m_sName,"OptionRowType")
{
	m_fLockInputSecs = 0.0001f;	// always lock for a tiny amount of time so that we throw away any queued inputs during the load.
	
	LOG->Trace( "ScreenOptions::ScreenOptions()" );
}


void ScreenOptions::Init()
{
	ScreenWithMenuElements::Init();

	m_OptionsNavigation = PREFSMAN->m_bArcadeOptionsNavigation? NAV_THREE_KEY:NAV_FIVE_KEY;

	m_SoundChangeCol.Load( THEME->GetPathS(m_sName,"change"), true );
	m_SoundNextRow.Load( THEME->GetPathS(m_sName,"next"), true );
	m_SoundPrevRow.Load( THEME->GetPathS(m_sName,"prev"), true );
	m_SoundToggleOn.Load( THEME->GetPathS(m_sName,"toggle on"), true );
	m_SoundToggleOff.Load( THEME->GetPathS(m_sName,"toggle off"), true );

	// add everything to m_framePage so we can animate everything at once
	this->AddChild( &m_framePage );

	m_bMoreShown = false;
	FOREACH_PlayerNumber( p )
	{
		m_iCurrentRow[p] = 0;
		m_iFocusX[p] = 0;
		m_bWasOnExit[p] = false;
		m_bGotAtLeastOneStartPressed[p] = false;
	}

	m_framePage.RunCommands( FRAME_ON_COMMAND );
}

void ScreenOptions::LoadOptionIcon( PlayerNumber pn, int iRow, CString sText )
{
	m_Rows[iRow]->LoadOptionIcon( pn, sText );
}

void ScreenOptions::InitMenu( InputMode im, const vector<OptionRowDefinition> &vDefs, const vector<OptionRowHandler*> &vHands )
{
	LOG->Trace( "ScreenOptions::Set()" );

	ASSERT( vDefs.size() == vHands.size() );

	m_InputMode = im;

	for( unsigned r=0; r<vDefs.size(); r++ )		// foreach row
	{
		m_Rows.push_back( new OptionRow() );
		OptionRow &row = *m_Rows.back();
		
		bool bFirstRowGoesDown = m_OptionsNavigation==NAV_TOGGLE_THREE_KEY;

		row.LoadMetrics( OPTION_ROW_TYPE );
		row.LoadNormal( vDefs[r], vHands[r], bFirstRowGoesDown );

		FOREACH_HumanPlayer( p )
			this->ImportOptions( r, p );
	
		CHECKPOINT_M( ssprintf("row %i: %s", r, row.GetRowDef().name.c_str()) );

		unsigned pos = r;
		CLAMP( pos, 0, NUM_ROWS_SHOWN-1 );
		const float fY = ROW_Y.GetValue( pos );

		row.AfterImportOptions( 
			GetExplanationTitle( r ),
			fY
			);
	}

	m_sprPage.Load( THEME->GetPathG(m_sName,"page") );
	m_sprPage->SetName( "Page" );
	SET_XY_AND_ON_COMMAND( m_sprPage );
	m_framePage.AddChild( m_sprPage );

	// init line highlights
	FOREACH_HumanPlayer( p )
	{
		m_sprLineHighlight[p].Load( THEME->GetPathG(m_sName,"line highlight") );
		m_sprLineHighlight[p].SetName( "LineHighlight" );
		m_sprLineHighlight[p].SetX( SCREEN_CENTER_X );
		m_framePage.AddChild( &m_sprLineHighlight[p] );
		ON_COMMAND( m_sprLineHighlight[p] );
	}
	
	// init highlights
	FOREACH_HumanPlayer( p )
	{
		m_Highlight[p].Load( p, false );
		m_framePage.AddChild( &m_Highlight[p] );
	}
	
	for( unsigned r=0; r<m_Rows.size(); r++ )		// foreach row
	{
		OptionRow &row = *m_Rows[r];
		m_framePage.AddChild( &row );
	}

	if( SHOW_EXIT_ROW )
	{
		// TRICKY:  Add "EXIT" item
		m_Rows.push_back( new OptionRow() );
		OptionRow &row = *m_Rows.back();
		row.LoadMetrics( OPTION_ROW_TYPE );
		row.LoadExit();
		m_framePage.AddChild( &row );
	}

	// add explanation here so it appears on top
	FOREACH_PlayerNumber( p )
	{
		m_textExplanation[p].LoadFromFont( THEME->GetPathF(m_sName,"explanation") );
		m_textExplanation[p].SetZoom( EXPLANATION_ZOOM );
		m_textExplanation[p].SetShadowLength( 0 );
		m_framePage.AddChild( &m_textExplanation[p] );
	}

	if( SHOW_SCROLL_BAR )
	{
		m_ScrollBar.SetName( "DualScrollBar", "ScrollBar" );
		m_ScrollBar.SetBarHeight( SCROLL_BAR_HEIGHT );
		m_ScrollBar.SetBarTime( SCROLL_BAR_TIME );
		FOREACH_PlayerNumber( p )
			m_ScrollBar.EnablePlayer( p, GAMESTATE->IsHumanPlayer(p) );
		m_ScrollBar.Load();
		SET_XY( m_ScrollBar );
		m_framePage.AddChild( &m_ScrollBar );
	}

	m_sprMore.Load( THEME->GetPathG( m_sName,"more") );
	m_sprMore->SetName( m_sName, "More" );
	SET_XY_AND_ON_COMMAND( m_sprMore );
	COMMAND( m_sprMore, m_bMoreShown? "ShowMore":"HideMore" );
	m_framePage.AddChild( m_sprMore );

	switch( m_InputMode )
	{
	case INPUTMODE_INDIVIDUAL:
		{
			FOREACH_PlayerNumber( p )
				m_textExplanation[p].SetXY( EXPLANATION_X.GetValue(p), EXPLANATION_Y.GetValue(p) );
		}
		break;
	case INPUTMODE_SHARE_CURSOR:
		m_textExplanation[0].SetXY( EXPLANATION_TOGETHER_X, EXPLANATION_TOGETHER_Y );
		break;
	default:
		ASSERT(0);
	}

	FOREACH_PlayerNumber( p )
	{
		m_sprDisqualify[p].Load( THEME->GetPathG(m_sName,"disqualify") );
		m_sprDisqualify[p]->SetName( m_sName, ssprintf("DisqualifyP%i",p+1) );
		SET_XY_AND_ON_COMMAND( m_sprDisqualify[p] );
		m_sprDisqualify[p]->SetHidden( true );	// unhide later if handicapping options are discovered
		m_framePage.AddChild( m_sprDisqualify[p] );
	}

	// poke once at all the explanation metrics so that we catch missing ones early
	for( int r=0; r<(int)m_Rows.size(); r++ )		// foreach row
	{
		GetExplanationText( r );
		GetExplanationTitle( r );
	}

	CHECKPOINT;

	PositionItems();
	PositionAllUnderlines();
	PositionIcons();
	RefreshAllIcons();
	PositionCursors();
	UpdateEnabledDisabled();

	FOREACH_PlayerNumber( p )
		OnChange( p );

	CHECKPOINT;

	/* It's tweening into position, but on the initial tween-in we only want to
	 * tween in the whole page at once.  Since the tweens are nontrivial, it's
	 * easiest to queue the tweens and then force them to finish. */
	for( int r=0; r<(int) m_Rows.size(); r++ )	// foreach options line
	{
		OptionRow &row = *m_Rows[r];
		row.FinishTweening();
	}

	m_sprMore->FinishTweening();

	this->SortByDrawOrder();
}

ScreenOptions::~ScreenOptions()
{
	LOG->Trace( "ScreenOptions::~ScreenOptions()" );
	for( unsigned i=0; i<m_Rows.size(); i++ )
		SAFE_DELETE( m_Rows[i] );
}

CString ScreenOptions::GetExplanationText( int iRow ) const
{
	OptionRow &row = *m_Rows[iRow];

	CString sLineName = row.GetRowDef().name;
	ASSERT( !sLineName.empty() );
	sLineName.Replace("\n-","");
	sLineName.Replace("\n","");
	sLineName.Replace(" ","");
	return THEME->GetMetric( "OptionExplanations", sLineName+"Help" );
}

CString ScreenOptions::GetExplanationTitle( int iRow ) const
{
	OptionRow &row = *m_Rows[iRow];
	
	CString sLineName = row.GetRowDef().name;
	sLineName.Replace("\n-","");
	sLineName.Replace("\n","");
	sLineName.Replace(" ","");
	CString sTitle = THEME->GetMetric( "OptionTitles", sLineName+"Title" );

	// HACK: tack the BPM onto the name of the speed line
	if( sLineName.CompareNoCase("speed")==0 )
	{
		if( SHOW_BPM_IN_SPEED_TITLE )
		{
			DisplayBpms bpms;
			if( GAMESTATE->m_pCurSong )
			{
				Song* pSong = GAMESTATE->m_pCurSong;
				pSong->GetDisplayBpms( bpms );
			}
			else if( GAMESTATE->m_pCurCourse )
			{
				Course *pCourse = GAMESTATE->m_pCurCourse;
				StepsType st = GAMESTATE->GetCurrentStyle()->m_StepsType;
				Trail* pTrail = pCourse->GetTrail( st );
				ASSERT( pTrail );
				pTrail->GetDisplayBpms( bpms );
			}

			if( (int) bpms.vfBpms.size() > MAX_COURSE_ENTRIES_BEFORE_VARIOUS )
			{}	// add nothing
			else if( bpms.IsSecret() )
				sTitle += ssprintf( " (??" "?)" ); /* split so gcc doesn't think this is a trigraph */
			else if( bpms.BpmIsConstant() )
				sTitle += ssprintf( " (%.0f)", bpms.GetMin() );
			else
				sTitle += ssprintf( " (%.0f-%.0f)", bpms.GetMin(), bpms.GetMax() );
		}
	}

	return sTitle;
}

BitmapText &ScreenOptions::GetTextItemForRow( PlayerNumber pn, int iRow, int iChoiceOnRow )
{
	ASSERT_M( iRow < (int)m_Rows.size(), ssprintf("%i < %i", iRow, (int)m_Rows.size() ) );
	OptionRow &row = *m_Rows[iRow];
	return row.GetTextItemForRow( pn, iChoiceOnRow );
}

void ScreenOptions::GetWidthXY( PlayerNumber pn, int iRow, int iChoiceOnRow, int &iWidthOut, int &iXOut, int &iYOut )
{
	ASSERT_M( iRow < (int)m_Rows.size(), ssprintf("%i < %i", iRow, (int)m_Rows.size() ) );
	OptionRow &row = *m_Rows[iRow];
	row.GetWidthXY( pn, iChoiceOnRow, iWidthOut, iXOut, iYOut );
}

void ScreenOptions::PositionUnderlines( int r, PlayerNumber pn )
{
	OptionRow &row = *m_Rows[r];
	row.PositionUnderlines( pn );
}

void ScreenOptions::PositionAllUnderlines()
{
	for( unsigned r=0; r<m_Rows.size(); r++ )
		FOREACH_HumanPlayer( p )
			PositionUnderlines( r, p );
}

void ScreenOptions::PositionIcons()
{
	for( unsigned i=0; i<m_Rows.size(); i++ )	// foreach options line
	{
		OptionRow &row = *m_Rows[i];
		row.PositionIcons();
			continue;
	}
}

void ScreenOptions::RefreshIcons( int row, PlayerNumber pn )
{
	// overridden by ScreenOptionsMaster
}

void ScreenOptions::RefreshAllIcons()
{
	FOREACH_HumanPlayer( p )
        for( unsigned r=0; r<m_Rows.size(); ++r )
			this->RefreshIcons( r, p );
}

void ScreenOptions::PositionCursors()
{
	// Set the position of the highlight showing the current option the user is changing.
	// Set the position of the underscores showing the current choice for each option line.
	FOREACH_HumanPlayer( pn )	// foreach player
	{
		const int iRow = m_iCurrentRow[pn];
		ASSERT_M( iRow < (int)m_Rows.size(), ssprintf("%i < %i", iRow, (int)m_Rows.size() ) );
		OptionRow &OptionRow = *m_Rows[iRow];

		OptionsCursor &highlight = m_Highlight[pn];

		const int iChoiceWithFocus = OptionRow.GetChoiceInRowWithFocus(pn);

		int iWidth, iX, iY;
		GetWidthXY( pn, iRow, iChoiceWithFocus, iWidth, iX, iY );
		highlight.SetBarWidth( iWidth );
		highlight.SetXY( (float)iX, (float)iY );
	}
}

void ScreenOptions::TweenCursor( PlayerNumber pn )
{
	// Set the position of the highlight showing the current option the user is changing.
	const int iRow = m_iCurrentRow[pn];
	ASSERT_M( iRow < (int)m_Rows.size(), ssprintf("%i < %i", iRow, (int)m_Rows.size() ) );

	const OptionRow &row = *m_Rows[iRow];
	const int iChoiceWithFocus = row.GetChoiceInRowWithFocus(pn);

	int iWidth, iX, iY;
	GetWidthXY( pn, iRow, iChoiceWithFocus, iWidth, iX, iY );

	OptionsCursor &highlight = m_Highlight[pn];
	highlight.StopTweening();
	highlight.BeginTweening( TWEEN_SECONDS );
	highlight.TweenBarWidth( iWidth );
	highlight.SetXY( (float)iX, (float)iY );

	if( GAMESTATE->IsHumanPlayer(pn) )  
	{
		COMMAND( m_sprLineHighlight[pn], "Change" );
		if( row.GetRowType() == OptionRow::ROW_EXIT )
			COMMAND( m_sprLineHighlight[pn], "ChangeToExit" );
		m_sprLineHighlight[pn].SetY( (float)iY );
	}
}

/* For "long row-style" rows, update the text on screen to contain the currently-
 * focused options. 
 *
 * This used to update a single player, but it's not always clear what that means
 * when dealing with bOneChoiceForAllPlayers and disabled players, which was brittle.
 * Update the whole row. */
void ScreenOptions::UpdateText( int iRow )
{
	OptionRow &row = *m_Rows[iRow];
	row.UpdateText();
}

void ScreenOptions::UpdateEnabledDisabled( int r )
{
	OptionRow &row = *m_Rows[r];

	bool bRowHasFocus[NUM_PLAYERS];
	FOREACH_PlayerNumber( pn )
		bRowHasFocus[pn] = GAMESTATE->IsHumanPlayer(pn) && m_iCurrentRow[pn] == (int)r;

	row.SetRowFocus( bRowHasFocus );
	row.UpdateEnabledDisabled();
}

void ScreenOptions::UpdateEnabledDisabled()
{
	for( unsigned r=0; r<m_Rows.size(); r++ )
		UpdateEnabledDisabled( r );
}

void ScreenOptions::Update( float fDeltaTime )
{
	//LOG->Trace( "ScreenOptions::Update(%f)", fDeltaTime );

	m_fLockInputSecs = max( 0, m_fLockInputSecs-fDeltaTime );

	ScreenWithMenuElements::Update( fDeltaTime );
}

void ScreenOptions::DrawPrimitives()
{
	Screen::DrawPrimitives();
}

void ScreenOptions::Input( const DeviceInput& DeviceI, const InputEventType selectType, const GameInput &GameI, const MenuInput &MenuI, const StyleInput &StyleI )
{
	/* Allow input when transitioning in (m_In.IsTransitioning()), but ignore it
	 * when we're transitioning out. */
	if( m_Back.IsTransitioning() || m_Out.IsTransitioning() || m_fLockInputSecs > 0 )
		return;

	if( selectType == IET_RELEASE )
	{
		switch( MenuI.button )
		{
		case MENU_BUTTON_START:
		case MENU_BUTTON_RIGHT:
		case MENU_BUTTON_LEFT:
			INPUTMAPPER->ResetKeyRepeat( MenuInput(MenuI.player, MENU_BUTTON_START) );
			INPUTMAPPER->ResetKeyRepeat( MenuInput(MenuI.player, MENU_BUTTON_RIGHT) );
			INPUTMAPPER->ResetKeyRepeat( MenuInput(MenuI.player, MENU_BUTTON_LEFT) );
		}
	}

	// default input handler
	Screen::Input( DeviceI, selectType, GameI, MenuI, StyleI );
}

void ScreenOptions::HandleScreenMessage( const ScreenMessage SM )
{
	switch( SM )
	{
	case SM_MenuTimer:
		this->BeginFadingOut();
		break;
	case SM_GoToPrevScreen:
//		this->ExportOptions();	// Don't save options if we're going back!
		this->GoToPrevScreen();
		break;
	case SM_GoToNextScreen:
		for( unsigned r=0; r<m_Rows.size(); r++ )		// foreach row
			FOREACH_HumanPlayer( p )
				this->ExportOptions( r, p );
		this->GoToNextScreen();
		break;
	case SM_BeginFadingOut:
		if(IsTransitioning())
			return; /* already transitioning */
		StartTransitioning( SM_GoToNextScreen );

		SCREENMAN->PlayStartSound();

		m_framePage.RunCommands( FRAME_OFF_COMMAND );
		break;
	case SM_GainFocus:
		INPUTFILTER->SetRepeatRate( 0.25f, 12, 0.25f, 12 );
		break;
	case SM_LoseFocus:
		INPUTFILTER->ResetRepeatRate();
		break;
	}
}


void ScreenOptions::PositionItems()
{
	const int total = NUM_ROWS_SHOWN;
	const int halfsize = total / 2;

	int first_start, first_end, second_start, second_end;

	/* Choices for each player.  If only one player is active, it's the same for both. */
	int P1Choice = GAMESTATE->IsHumanPlayer(PLAYER_1)? m_iCurrentRow[PLAYER_1]: m_iCurrentRow[PLAYER_2];
	int P2Choice = GAMESTATE->IsHumanPlayer(PLAYER_2)? m_iCurrentRow[PLAYER_2]: m_iCurrentRow[PLAYER_1];

	vector<OptionRow*> Rows( m_Rows );
	OptionRow *ExitRow = NULL;

	if( (bool)SEPARATE_EXIT_ROW && Rows.back()->GetRowType() == OptionRow::ROW_EXIT )
	{
		ExitRow = &*Rows.back();

		/* Remove the exit row for purposes of positioning everything else. */
		if( P1Choice == (int) Rows.size()-1 )
			--P1Choice;
		if( P2Choice == (int) Rows.size()-1 )
			--P2Choice;

		Rows.erase( Rows.begin()+Rows.size()-1, Rows.end() );
	}

	const bool BothPlayersActivated = GAMESTATE->IsHumanPlayer(PLAYER_1) && GAMESTATE->IsHumanPlayer(PLAYER_2);
	if( m_InputMode == INPUTMODE_SHARE_CURSOR || !BothPlayersActivated )
	{
		/* Simply center the cursor. */
		first_start = max( P1Choice - halfsize, 0 );
		first_end = first_start + total;
		second_start = second_end = first_end;
	} else {
		/* First half: */
		const int earliest = min( P1Choice, P2Choice );
		first_start = max( earliest - halfsize/2, 0 );
		first_end = first_start + halfsize;

		/* Second half: */
		const int latest = max( P1Choice, P2Choice );

		second_start = max( latest - halfsize/2, 0 );

		/* Don't overlap. */
		second_start = max( second_start, first_end );

		second_end = second_start + halfsize;
	}

	first_end = min( first_end, (int) Rows.size() );
	second_end = min( second_end, (int) Rows.size() );

	/* If less than total (and Rows.size()) are displayed, fill in the empty
	 * space intelligently. */
	while(1)
	{
		const int sum = (first_end - first_start) + (second_end - second_start);
		if( sum >= (int) Rows.size() || sum >= total)
			break; /* nothing more to display, or no room */

		/* First priority: expand the top of the second half until it meets
		 * the first half. */
		if( second_start > first_end )
			second_start--;
		/* Otherwise, expand either end. */
		else if( first_start > 0 )
			first_start--;
		else if( second_end < (int) Rows.size() )
			second_end++;
		else
			ASSERT(0); /* do we have room to grow or don't we? */
	}

	int pos = 0;
	for( int i=0; i<(int) Rows.size(); i++ )		// foreach row
	{
		float fY;
		if( i < first_start )
			fY = ROW_Y_OFF_SCREEN_TOP;
		else if( i < first_end )
			fY = ROW_Y.GetValue( pos++ );
		else if( i < second_start )
			fY = ROW_Y_OFF_SCREEN_CENTER;
		else if( i < second_end )
			fY = ROW_Y.GetValue( pos++ );
		else
			fY = ROW_Y_OFF_SCREEN_BOTTOM;
			
		OptionRow &row = *Rows[i];

		row.SetRowY( fY );
		bool bHidden = 
			i < first_start ||
			(i >= first_end && i < second_start) ||
			i >= second_end;
		row.SetRowHidden( bHidden );
	}

	if( ExitRow )
	{
		ExitRow->SetRowY( SEPARATE_EXIT_ROW_Y );
		ExitRow->SetRowHidden( second_end != (int) Rows.size() );
	}
}


void ScreenOptions::OnChange( PlayerNumber pn )
{
	if( !GAMESTATE->IsHumanPlayer(pn) )
		return;

	const int iCurRow = m_iCurrentRow[pn];
	
	/* Update m_fY and m_bHidden[]. */
	PositionItems();

	/* Do positioning. */
	PositionAllUnderlines();
	RefreshIcons( iCurRow, pn );
	PositionIcons();
	UpdateEnabledDisabled();

	if( SHOW_SCROLL_BAR )
	{
		float fPercent = 0;
		if( m_Rows.size() > 1 )
			fPercent = m_iCurrentRow[pn] / float(m_Rows.size()-1);
		m_ScrollBar.SetPercentage( pn, fPercent );
	}


	/* If the last row is EXIT, and is hidden, then show MORE. */
	const bool ShowMore = m_Rows.back()->GetRowType() == OptionRow::ROW_EXIT && m_Rows.back()->GetHidden();
	if( m_bMoreShown != ShowMore )
	{
		m_bMoreShown = ShowMore;
		COMMAND( m_sprMore, m_bMoreShown? "ShowMore":"HideMore" );
	}

	/* Update all players, since changing one player can move both cursors. */
	FOREACH_PlayerNumber( p )
	{
		if( GAMESTATE->IsHumanPlayer(p) )
			TweenCursor( p );

		int iCurrentRow = m_iCurrentRow[pn];
		OptionRow &row = *m_Rows[iCurrentRow];
		const bool ExitSelected = row.GetRowType() == OptionRow::ROW_EXIT;
		if( p == pn || GAMESTATE->GetNumHumanPlayers() == 1 )
		{
			if( m_bWasOnExit[p] != ExitSelected )
			{
				m_bWasOnExit[p] = ExitSelected;
				COMMAND( m_sprMore, ssprintf("Exit%sP%i", ExitSelected? "Selected":"Unselected", p+1) );
			}
		}
	}

	const CString text = GetExplanationText( iCurRow );

	BitmapText *pText = NULL;
	switch( m_InputMode )
	{
	case INPUTMODE_INDIVIDUAL:
		pText = &m_textExplanation[pn];
		if( pText->GetText() != text )
		{
			pText->FinishTweening();
			pText->RunCommands( EXPLANATION_ON_COMMAND.GetValue(pn) );
			pText->SetText( text );
		}
		break;
	case INPUTMODE_SHARE_CURSOR:
		pText = &m_textExplanation[0];
		if( pText->GetText() != text )
		{
			pText->FinishTweening();
			pText->RunCommands( EXPLANATION_TOGETHER_ON_COMMAND );
			pText->SetText( text );
		}
		break;
	}
}


void ScreenOptions::MenuBack( PlayerNumber pn )
{
	Screen::MenuBack( pn );

	Back( SM_GoToPrevScreen );
}

bool ScreenOptions::IsOnLastRow( PlayerNumber pn ) const
{
	int iCurRow = m_iCurrentRow[pn];
	return iCurRow == (int)(m_Rows.size()-1);
}

bool ScreenOptions::AllAreOnLastRow() const
{
	FOREACH_HumanPlayer( p )
	{
		if( !IsOnLastRow(p) )
			return false;
	}
	return true;
}

void ScreenOptions::MenuStart( PlayerNumber pn, const InputEventType selectType )
{
	switch( selectType )
	{
	case IET_FIRST_PRESS:
		m_bGotAtLeastOneStartPressed[pn] = true;
		break;
	case IET_RELEASE:
		return;	// ignore
	default:	// repeat selectType
		if( !m_bGotAtLeastOneStartPressed[pn] )
			return;	// don't allow repeat
		break;
	}
	
	int iCurRow = m_iCurrentRow[pn];
	OptionRow &row = *m_Rows[iCurRow];
	

	/* If we are in a three-button mode, check to see if MENU_BUTTON_LEFT and
	 * MENU_BUTTON_RIGHT are being held. */
	switch( m_OptionsNavigation )
	{
	case NAV_THREE_KEY:
	case NAV_TOGGLE_THREE_KEY:
		{
			bool bHoldingLeftAndRight = 
				INPUTMAPPER->IsButtonDown( MenuInput(pn, MENU_BUTTON_RIGHT) ) &&
				INPUTMAPPER->IsButtonDown( MenuInput(pn, MENU_BUTTON_LEFT) );
			if( bHoldingLeftAndRight )
			{
				MoveRow( pn, -1, selectType != IET_FIRST_PRESS );		
				return;
			}
		}
	}


	// If on exit, check it all players are on "Exit"
	if( IsOnLastRow(pn) )
	{
		/* Don't accept START to go to the next screen if we're still transitioning in. */
		if( AllAreOnLastRow()  &&  selectType == IET_FIRST_PRESS && !IsTransitioning() )
			this->BeginFadingOut();
		return;
	}


	if( row.GetFirstItemGoesDown() )
	{
		int iChoiceInRow = row.GetChoiceInRowWithFocus(pn);
		if( iChoiceInRow == 0 )
		{
			MenuDown( pn, selectType );
			return;
		}
	}
	
	if( row.GetRowDef().selectType == SELECT_MULTIPLE )
	{
		int iChoiceInRow = row.GetChoiceInRowWithFocus(pn);
		bool bSelected = !row.GetSelected( pn, iChoiceInRow );
		row.SetSelected( pn, iChoiceInRow, bSelected );

		if( bSelected )
			m_SoundToggleOn.Play();
		else
			m_SoundToggleOff.Play();

		PositionUnderlines( iCurRow, pn );
		RefreshIcons( iCurRow, pn );

		if( row.GetFirstItemGoesDown() )
		{
			// move to the first choice in the row
			ChangeValueInRow( pn, -row.GetChoiceInRowWithFocus(pn), selectType != IET_FIRST_PRESS );
		}
	}
	else	// data.selectType != SELECT_MULTIPLE
	{
		switch( m_OptionsNavigation )
		{
		case NAV_THREE_KEY:
			MenuDown( pn, selectType );
			break;
		case NAV_TOGGLE_THREE_KEY:
		case NAV_TOGGLE_FIVE_KEY:
			if( row.GetRowDef().selectType != SELECT_MULTIPLE )
			{
				int iChoiceInRow = row.GetChoiceInRowWithFocus(pn);
				if( row.GetRowDef().bOneChoiceForAllPlayers )
					row.SetOneSharedSelection( iChoiceInRow );
				else
					row.SetOneSelection( pn, iChoiceInRow );
			}
			if( row.GetFirstItemGoesDown() )
				ChangeValueInRow( pn, -row.GetChoiceInRowWithFocus(pn), selectType != IET_FIRST_PRESS );	// move to the first choice
			else
				ChangeValueInRow( pn, 0, selectType != IET_FIRST_PRESS );
			break;
		case NAV_THREE_KEY_MENU:
			/* Don't accept START to go to the next screen if we're still transitioning in. */
			if( selectType == IET_FIRST_PRESS && !IsTransitioning() )
				this->BeginFadingOut();
			break;
		case NAV_FIVE_KEY:
			/* Jump to the exit row.  (If everyone's already on the exit row, then
			 * we'll have already gone to the next screen above.) */
			MoveRow( pn, m_Rows.size()-m_iCurrentRow[pn]-1, selectType != IET_FIRST_PRESS );
			break;
		}
	}
}

void ScreenOptions::StoreFocus( PlayerNumber pn )
{
	/* Long rows always put us in the center, so don't update the focus. */
	int iCurrentRow = m_iCurrentRow[pn];
	const OptionRow &row = *m_Rows[iCurrentRow];
	if( row.GetRowDef().layoutType == LAYOUT_SHOW_ONE_IN_ROW )
		return;

	int iWidth, iY;
	GetWidthXY( pn, m_iCurrentRow[pn], row.GetChoiceInRowWithFocus(pn), iWidth, m_iFocusX[pn], iY );
	LOG->Trace("cur selection %ix%i @ %i", 
		m_iCurrentRow[pn], row.GetChoiceInRowWithFocus(pn), m_iFocusX[pn]);
}

/* Left/right */
void ScreenOptions::ChangeValueInRow( PlayerNumber pn, int iDelta, bool Repeat )
{
	const int iCurRow = m_iCurrentRow[pn];
	OptionRow &row = *m_Rows[iCurRow];
	
	if( row.GetRowType() == OptionRow::ROW_EXIT	)	// EXIT is selected
		return;		// don't allow a move

	const int iNumOptions = row.GetRowDef().choices.size();
	if( m_OptionsNavigation == NAV_THREE_KEY_MENU && iNumOptions <= 1 )	// 1 or 0
	{
		/* There are no other options on the row; move up or down instead of left and right.
		 * This allows navigating the options menu with left/right/start. 
		 *
		 * XXX: Only allow repeats if the opposite key isn't pressed; otherwise, holding both
		 * directions will repeat in place continuously, which is weird. */
		MoveRow( pn, iDelta, Repeat );
		return;
	}

	if( Repeat )
		return;

	bool bOneChanged = false;


	int iCurrentChoiceWithFocus = row.GetChoiceInRowWithFocus(pn);
	int iNewChoiceWithFocus = iCurrentChoiceWithFocus + iDelta;
	wrap( iNewChoiceWithFocus, iNumOptions );
	
	if( iCurrentChoiceWithFocus != iNewChoiceWithFocus )
		bOneChanged = true;

	row.SetChoiceInRowWithFocus( pn, iNewChoiceWithFocus );
	StoreFocus( pn );

	if( row.GetRowDef().bOneChoiceForAllPlayers )
	{
		/* If this row is bOneChoiceForAllPlayers, then lock the cursors together
		 * for this row.  Don't do this in toggle modes, since the current selection
		 * and the current focus are detached. */
		bool bForceFocusedChoiceTogether = false;
		if( m_OptionsNavigation!=NAV_TOGGLE_THREE_KEY &&
			m_OptionsNavigation!=NAV_TOGGLE_FIVE_KEY &&
			row.GetRowDef().bOneChoiceForAllPlayers )
		{
			bForceFocusedChoiceTogether = true;
		}

		/* Also lock focus if the screen is explicitly set to share cursors. */
		if( m_InputMode == INPUTMODE_SHARE_CURSOR )
			bForceFocusedChoiceTogether = true;

		if( bForceFocusedChoiceTogether )
		{
			// lock focus together
			FOREACH_HumanPlayer( p )
			{
				row.SetChoiceInRowWithFocus( p, iNewChoiceWithFocus );
				StoreFocus( p );
			}
		}

		FOREACH_PlayerNumber( p )
		{
			if( m_OptionsNavigation==NAV_TOGGLE_THREE_KEY || m_OptionsNavigation==NAV_TOGGLE_FIVE_KEY )
			{
				;	// do nothing
			}
			else
			{
				if( row.GetRowDef().selectType == SELECT_MULTIPLE )
					;	// do nothing.  User must press Start to toggle the selection.
				else
					row.SetOneSelection( p, iNewChoiceWithFocus );			
			}
		}
	}
	else
	{
		if( m_OptionsNavigation==NAV_TOGGLE_THREE_KEY || m_OptionsNavigation==NAV_TOGGLE_FIVE_KEY )
		{
			;	// do nothing
		}
		else
		{
			if( row.GetRowDef().selectType == SELECT_MULTIPLE )
				;	// do nothing.  User must press Start to toggle the selection.
			else
				row.SetOneSelection( pn, iNewChoiceWithFocus );
		}
	}

	UpdateText( iCurRow );

	OnChange( pn );

	if( m_OptionsNavigation != NAV_THREE_KEY_MENU )
		m_SoundChangeCol.Play();

	if( row.GetRowDef().m_bExportOnChange )
		FOREACH_HumanPlayer( p )
			ExportOptions( iCurRow, p );
}


/* Up/down */
void ScreenOptions::MoveRow( PlayerNumber pn, int dir, bool Repeat ) 
{
	const int iCurrentRow = m_iCurrentRow[pn];
	OptionRow &row = *m_Rows[iCurrentRow];

	if( row.GetFirstItemGoesDown() )
	{
		// If moving from a bFirstChoiceGoesDown row, put focus back on 
		// the first choice before moving.
		row.SetChoiceInRowWithFocus( pn, 0 );
	}

	LOG->Trace("move pn %i, dir %i, rep %i", pn, dir, Repeat);
	bool changed = false;
	FOREACH_PlayerNumber( p )
	{
		if( m_InputMode == INPUTMODE_INDIVIDUAL && p != pn )
			continue;	// skip

		int r = m_iCurrentRow[p] + dir;
		if( Repeat && ( r == -1 || r == (int) m_Rows.size() ) )
			continue; // don't wrap while repeating

		wrap( r, m_Rows.size() );

		const unsigned iOldSelection = row.GetChoiceInRowWithFocus(p);

		m_iCurrentRow[p] = r;
		ASSERT( r >= 0 && r < (int)m_Rows.size() );

		OptionRow &row = *m_Rows[r];

		switch( m_OptionsNavigation )
		{
		case NAV_TOGGLE_THREE_KEY:
		case NAV_TOGGLE_FIVE_KEY:
			if( row.GetRowDef().layoutType != LAYOUT_SHOW_ONE_IN_ROW )
			{
				int iSelectionDist = -1;
				for( unsigned i = 0; i < row.GetTextItemsSize(); ++i )
				{
					int iWidth, iX, iY;
					GetWidthXY( p, m_iCurrentRow[p], i, iWidth, iX, iY );
					const int iDist = abs( iX-m_iFocusX[p] );
					if( iSelectionDist == -1 || iDist < iSelectionDist )
					{
						iSelectionDist = iDist;
						row.SetChoiceInRowWithFocus( p, i );
					}
				}
				row.SetChoiceInRowWithFocus( p, min( iOldSelection, row.GetTextItemsSize()-1 ) );
			}
		}

		OnChange( p );
		changed = true;
	}
	if( changed )
	{
		if( dir < 0 )
			m_SoundPrevRow.Play();
		else
			m_SoundNextRow.Play();
	}
}

void ScreenOptions::MenuUp( PlayerNumber pn, const InputEventType type )
{
	int r;
	bool bFoundDest = false;
	for( r=m_iCurrentRow[pn]-1; r>=0; r-- )
	{
		OptionRow &row = *m_Rows[r];
		if( row.GetRowDef().EnabledForPlayer(pn) )
		{
			bFoundDest = true;
			break;
		}
	}
	int iDelta = bFoundDest ? r-m_iCurrentRow[pn] : -1;
	MoveRow( pn, iDelta, type != IET_FIRST_PRESS );
}

void ScreenOptions::MenuDown( PlayerNumber pn, const InputEventType type )
{
	unsigned r;
	bool bFoundDest = false;
	for( r=m_iCurrentRow[pn]+1; r<m_Rows.size(); r++ )
	{
		OptionRow &row = *m_Rows[r];
		if( row.GetRowDef().EnabledForPlayer(pn) )
		{
			bFoundDest = true;
			break;
		}
	}
	int iDelta = bFoundDest ? r-m_iCurrentRow[pn] : +1;
	MoveRow( pn, iDelta, type != IET_FIRST_PRESS );
}


int ScreenOptions::GetCurrentRow( PlayerNumber pn ) const
{
	const int r = m_iCurrentRow[pn];
	return r;
}


/*
 * (c) 2001-2004 Chris Danford, Glenn Maynard
 * All rights reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, and/or sell copies of the Software, and to permit persons to
 * whom the Software is furnished to do so, provided that the above
 * copyright notice(s) and this permission notice appear in all copies of
 * the Software and that both the above copyright notice(s) and this
 * permission notice appear in supporting documentation.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF
 * THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS
 * INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
