local pPlayer = ...;
local pLife;

local t = Def.ActorFrame {};

local HeartWidth = 46;
local HeartHeight = 40;
local HPbarWidth = 300;
local HPbarHeight = HeartHeight;
local constant = 1
local bHot = false

local timingdata = GAMESTATE:GetCurrentSteps(pPlayer):GetTimingData()

local lm; -- LifeMeter
local function PLifeOverloadFunction(unused_param)
    return lm:GetLifeWithOverload();
end

local PLife;
PLife = function(pn)
    lm = SCREENMAN:GetTopScreen():GetLifeMeter(pn)
    if lm.GetLifeWithOverload ~= nil then
        PLife = PLifeOverloadFunction
    else
        PLife = PLifeGlobal
    end
    return PLife(pn)
end


--[[
local ststats = STATSMAN:GetCurStageStats()

local ststops = timingdata:GetStops()
local stdelays = timingdata:GetDelays()

local notimezones = {}


local function fillNoTimes( t )
    for i,pair in pairs(t) do
        local arg1,arg2 = string.match(pair,"(.+)=(.+)")
        local beat = tonumber(arg1)
        local length = tonumber(arg2)
        --io.write(string.format("%s %s", beat, length)) io.flush()
        local timebeat = tostring(timingdata:GetElapsedTimeFromBeat( beat ))
        if notimezones[timebeat] then
            notimezones[ timebeat ] = notimezones[ timebeat ] + length
        else
            notimezones[ timebeat ] = length
        end
    end
end
fillNoTimes( ststops )
fillNoTimes( stdelays )

local function getCummulativeNotime( time )
    local total = 0
    for timebeat,length in pairs( notimezones ) do
        local numtimebeat = tonumber(timebeat)
        if numtimebeat < time then
            total = total + math.min( time-numtimebeat, length )
        end
    end
    return total
end     
]]
function GetCustomAlign(plnumb)
	if plnumb==PLAYER_2 then
		return 1.0;
	end
	return 0.0;
end

function GetCustomHPos(plnumb, value)
	local value2 = value;
	if plnumb==PLAYER_2 then
		value2 = -value2;
	end
	return value2;
end

function GetCustomHPosSide(plnumb)
	local avar;
	if plnumb==PLAYER_1 then
		avar = SCREEN_LEFT;
	else
		avar = 0;
	end
	return avar;
end


local function SetColorHP(life,object,heart)
	--local lifeII = math.floor(life*100);
	local lifeII = life;
	--lifeII = lifeII/100;
		if lifeII==0 then
			if heart==false then
				object:visible(false);
			end
		else
			if lifeII>0.2/constant then
				if lifeII>0.4/constant then
					if lifeII>0.6/constant then
						if lifeII>0.8/constant then
							object:effectcolor1( color("#3FFF6FFF") );
							if bHot then
								object:effectcolor2( color("#80FFA0FF") );
								object:effecttiming(1,0,1,0);
							else
								object:effectcolor2( color("#3FFF6F66") );
								object:effecttiming(0,0,0,1);
							end
						else
							object:effectcolor1( color("#95FF2BFF") );
							object:effectcolor2( color("#95FF2B66") );
							object:effecttiming(0,0,0,1);
						end
					else
						object:effectcolor1( color("#FFE33FFF") );
						object:effectcolor2( color("#FFE33F66") );
						object:effecttiming(0,0,0,1);
					end
				else
					object:effectcolor1( color("#FF873FFF") );
					object:effectcolor2( color("#FF873F66") );
					object:effecttiming(1,0,1,0);
				end
			else
				object:effectcolor1( color("#FF3F3FFF") );
				object:effectcolor2( color("#FF3F3F66") );
				object:effecttiming(0.5,0,0.5,0);
			end
							
			object:visible(true);
		end;
end;

-- Parts of Life bar
local tHeart;
local tBar;
local tNum;


local function UpdateTime(self)
	if GAMESTATE:IsHumanPlayer(pPlayer)==true then
		pLife = PLife(pPlayer);
		bHot = pLife>0.6 and (bHot or pLife>=1);
        
		SetColorHP(pLife, tHeart:GetChild("hColour"),true);
		SetColorHP(pLife, tBar:GetChild("bColour"),false);

		local tipObj = tBar:GetChild("Tip");
		SetColorHP(pLife,tipObj:GetChild("Body"):GetChild("TipGlow"),false);
		tipObj:playcommand("MoveTip");
	
		tBar:GetChild("bColour"):playcommand("HPcrop");
		tBar:GetChild("bColourOverload"):playcommand("HPcrop");

		tNum:settextf("%i",pLife*1000);

       	--local music_secs = GAMESTATE:GetCurMusicSeconds()
		--local beat_from_time = timingdata:GetBeatFromElapsedTime( music_secs )
		--local time_from_beat = timingdata:GetElapsedTimeFromBeat( beat_from_time )
		
        --local totalstop = getCummulativeNotime( music_secs )
        --music_secs = music_secs - totalstop
        
        --self:GetChild("CurrentMuSecs"):settextf("%f %f", music_secs, totalstop )
        
		--self:GetChild("CurrentMuSecs"):settextf("%f  %f", beat_from_time, music_secs );
		--
		--local css = STATSMAN:GetCurStageStats();
		--self:GetChild("CurrSec"):settext( string.format("%0.3f  %0.3f", css:GetStepsSeconds(), css:GetGameplaySeconds() ) );
	end
end;

------------- tHeart ------------
tHeart = Def.ActorFrame
{
	Name="Heart";
	InitCommand=cmd(visible,GAMESTATE:IsHumanPlayer(pPlayer));
	OnCommand=cmd(valign,0.5; halign, GetCustomAlign(pPlayer); x,0; y,0);

    LoadActor("HeartBack") .. {
        InitCommand=cmd(visible,true);
        OnCommand=cmd(halign,GetCustomAlign(pPlayer));
    };

    LoadActor("HeartIn") .. {
        Name="hColour";
        InitCommand=cmd(visible,true);
        OnCommand=cmd(halign,GetCustomAlign(pPlayer); effectclock,"bgm";effecttiming,0,0,0,1;diffuseshift;effectcolor1,color("#FFFFFFFF");effectcolor2,color("#FFFFFF66"));
        HealthStateChangedMessageCommand=function(self,params)
            if params.HealthState=='HealthState_Dead' then
                self:visible(false);
            end
        end;
    };

    LoadActor("HeartFrame") .. {
        InitCommand=cmd(visible,true);
        OnCommand=cmd(halign,GetCustomAlign(pPlayer); blend,Blend.Add);
    };

};

------------ tBar ---------------------
tBar = Def.ActorFrame {};

tBar.Name = "HPbar";
--tBar.OnCommand=cmd(halign,GetCustomAlign(pPlayer));

tBar[#tBar+1] = LoadActor("HPbarBack") .. {
	InitCommand=cmd(visible,GAMESTATE:IsHumanPlayer(pPlayer));
	OnCommand=cmd(halign,GetCustomAlign(pPlayer);x,0;valign,0.5;y,0); 
}

if pPlayer==PLAYER_2 then
    tBar[#tBar+1] = LoadActor("HPbarLife") .. {
        Name="bColour";
        InitCommand=cmd(visible,GAMESTATE:IsHumanPlayer(pPlayer);halign,1.0;x,0;valign,0.5;y,0;cropleft,1); 
        OnCommand=cmd(effectclock,"bgm";effecttiming,0,0,0,1;diffuseshift;effectcolor1,color("#FFFFFFFF");effectcolor2,color("#FFFFFF66"));
        HPcropCommand=cmd(stoptweening;decelerate,0.1;cropleft,1-pLife);
        
    };
    tBar[#tBar+1] = LoadActor("HPbarLife") .. {
        Name="bColourOverload";
        InitCommand=cmd(visible,GAMESTATE:IsHumanPlayer(pPlayer);halign,1.0;x,0;valign,0.5;y,0;cropleft,1); 
        OnCommand=cmd(effectclock,"bgm";effecttiming,0,0,0,1;diffuseshift;effectcolor1,color("#00C0FFFF");effectcolor2,color("#00C0FF66"));
        HPcropCommand=cmd(stoptweening;decelerate,0.1;cropleft,1-pLife+1);
        
    };
else
    tBar[#tBar+1] = LoadActor("HPbarLife") .. {
        Name="bColour";
        InitCommand=cmd(visible,GAMESTATE:IsHumanPlayer(pPlayer);halign,GetCustomAlign(pPlayer);x,0;valign,0.5;y,0;cropright,1); 
        OnCommand=cmd(effectclock,"bgm";effecttiming,0,0,0,1;diffuseshift;effectcolor1,color("#FFFFFFFF");effectcolor2,color("#FFFFFF66"));
        HPcropCommand=cmd(stoptweening;decelerate,0.1;cropright,math.max((1-pLife*constant),0));
        
    };
    tBar[#tBar+1] = LoadActor("HPbarLife") .. {
        Name="bColourOverload";
        InitCommand=cmd(visible,GAMESTATE:IsHumanPlayer(pPlayer);halign,GetCustomAlign(pPlayer);x,0;valign,0.5;y,0;cropright,1); 
        OnCommand=cmd(effectclock,"bgm";effecttiming,1,0,1,0;diffuseshift;effectcolor1,color("#00C0FFFF");effectcolor2,color("#00C0FF66"));
        HPcropCommand=cmd(stoptweening;decelerate,0.1;cropright,math.max((1-pLife*constant)+1,0));
        
    };
end

for SepI=1,4,1 do
	tBar[#tBar+1] = LoadActor("HPbarSpl") .. {
		InitCommand=cmd(visible,GAMESTATE:IsHumanPlayer(pPlayer));
		OnCommand=cmd(halign,0.5;x,GetCustomHPos(pPlayer,((300/5)*SepI)); valign,0.5;y,0;blend,Blend.Add); 
	}
end

tBar[#tBar+1] = LoadActor("HPbarFrame") .. {
	InitCommand=cmd(visible,GAMESTATE:IsHumanPlayer(pPlayer));
	OnCommand=cmd(halign,GetCustomAlign(pPlayer);x,0;valign,0.5;y,0;blend,Blend.Add); 
}

-- Tip
tBar[#tBar+1] = Def.ActorFrame {
	Name="Tip";
	InitCommand=cmd(visible,GAMESTATE:IsHumanPlayer(pPlayer); player,pPlayer);
	OnCommand=cmd(x,0;valign,0.5;horizalign,center;y,0);
	MoveTipCommand=cmd(stoptweening;decelerate,0.1;x,GetCustomHPos(pPlayer,math.min(HPbarWidth*pLife*constant, HPbarWidth)););

	Def.ActorFrame {
	
	Name="Body";
	InitCommand=cmd(visible,true, player,pPlayer);
	JudgmentMessageCommand=function(self,params)
		local tpn = params.TapNoteScore;
		if params.Player==pPlayer and (tpn=='TapNoteScore_W1' or tpn=='TapNoteScore_W2' or tpn=='TapNoteScore_W3' or tpn=='TapNoteScore_CheckpointHit') then
			self:stoptweening();
			self:zoom(1.4);
			self:linear(0.1);
			self:zoom(1);
		--elseif params.TapNoteScore=='TapNoteScore_W2' then
			--self:settext("W2");
		--elseif params.TapNoteScore=='TapNoteScore_W3' then
			--self:settext("W3");
		--elseif params.TapNoteScore=='TapNoteScore_W4' then
			--self:settext("W4");
		--elseif params.TapNoteScore=='TapNoteScore_W5' then
			--self:settext("W5");
		end;
	end;
	HealthStateChangedMessageCommand=function(self,params)
		if params.PlayerNumber==pPlayer then
			if params.HealthState=='HealthState_Dead' then
				self:linear(0.1);
				self:diffusealpha(0);
			else
				self:diffusealpha(1);
			end
		end
	end;
	
	LoadActor("TipGlow.png") .. {
		Name="TipGlow";
		InitCommand=cmd(visible,true; horizalign,center; valign,0.5);
		OnCommand=cmd(effectclock,"bgm";effecttiming,0,0,0,1;diffuseshift;effectcolor1,color("#FFFFFFFF");effectcolor2,color("#FFFFFF66"))
	};
	
	LoadActor("Tip.png") .. {
		InitCommand=cmd(visible,true; horizalign,center; valign,0.5);
	};
	
	};
};

-- HPbar -- 

----------- tNum --------------
tNum = LoadFont("../../Default/Fonts/_roboto", "Bold") .. {
	Name="HPnum";		
	InitCommand=cmd(visible,GAMESTATE:IsHumanPlayer(pPlayer));
	OnCommand=cmd(horizalign,right;valign,0.5); 
}

--[[
local tCurrentMuSecs = LoadFont("../../Default/Fonts/_roboto", "Bold") .. {
	Name="CurrentMuSecs";		
	InitCommand=cmd(visible,GAMESTATE:IsHumanPlayer(pPlayer));
	OnCommand=cmd(horizalign,left;valign,0.5; zoom,0.5); 
}
]]



t[#t+1] = tHeart;
t[#t+1] = tBar;
t[#t+1] = tNum;
--t[#t+1] = tCurrentMuSecs;

--[[
t[#t+1] = LoadFont("../../Default/Fonts/_roboto", "Bold") .. {
	Name="CurrSec";		
	InitCommand=cmd(visible,GAMESTATE:IsHumanPlayer(pPlayer));
	OnCommand=cmd(horizalign,right;valign,0.5; addx,280;addy,40; zoom, 0.5); 
}
]]
t.InitCommand= function(self)
    self:player(pPlayer):visible(GAMESTATE:IsHumanPlayer(pPlayer));
    tHeart = self:GetChild("Heart");
    tBar = self:GetChild("HPbar");
    tNum = self:GetChild("HPnum");
    self:SetUpdateFunction(UpdateTime);
end

t.OnCommand = function(self)
	local l_x1 = GetCustomHPosSide(pPlayer)+GetCustomHPos(pPlayer,HeartWidth);
	local l_x2;

	if pPlayer==PLAYER_2 then
		l_x2 = 3;
	else
		l_x2 = 92;
	end

	self:valign(0.5);
	
	self:GetChild("HPbar"):x(l_x1);
	self:GetChild("HPnum"):x(GetCustomHPosSide(pPlayer)+GetCustomHPos(pPlayer,HeartWidth+HPbarWidth+l_x2));
	self:GetChild("HPnum"):y(5);
	
	--self:GetChild("CurrentMuSecs"):x(2):y(40)
end;

t.HealthStateChangedMessageCommand=function(self,params)
		if params.PlayerNumber==pPlayer then
			bHot = bHot and not (params.HealthState=='HealthState_Dead' or params.HealthState=='HealthState_Danger') -- or params.HealthState=='HealthState_Hot');
		end
end;

t.JudgmentMessageCommand=function(self,params)
		local tpn = params.TapNoteScore;
		if params.Player==pPlayer and not tpn=='TapNoteScore_None' then
			if (
				tpn=='TapNoteScore_W4' or
				tpn=='TapNoteScore_W5' or
				tpn=='TapNoteScore_Miss' or
				tpn=='TapNoteScore_CheckpointMiss' or
				tpn=='TapNoteScore_HitMine'
			) then
				bHot = false
			end
		end;
end;

return t;
