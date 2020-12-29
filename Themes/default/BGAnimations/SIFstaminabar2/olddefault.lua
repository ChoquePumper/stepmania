local lifeP1bar;
local lifeP2bar;
function PLife(pn)
	return SCREENMAN:GetTopScreen():GetLifeMeter(pn):GetLife();
end;

function SetColorHP(life,object,heart)
	local lifeII = math.floor(life*100);
	lifeII = lifeII/100;
		if lifeII==0 then
			if heart==false then
				object:visible(false);
			end
		else
									--
									--if lifeP1==1 then
									--	self:effecttiming(0,0,0,0);
									--else
									--	self:effectmagnitude(,0,0);
									--end;
									
									-- COLORS?
			if lifeII>0.2 then
				if lifeII>0.4 then
					if lifeII>0.6 then
						if lifeII>0.8 then
							object:effectcolor1( color("#3FFF6FFF") );
							--object:effectcolor2( color("#3FFF6F66") );
							--object:effecttiming(0,0,0,1);
							if lifeII==1 then
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

function UpdateHPbar(pn,object,heart,tip)
	if pn==PLAYER_1 and GAMESTATE:IsHumanPlayer(pn)==true then
		lifeP1bar = PLife(PLAYER_1);
		SetColorHP(lifeP1bar,object,heart);
		if tip==true then
			SetColorHP(lifeP1bar,object:GetChild("Body"):GetChild("TipGlow"),false);
			object:playcommand("MoveTip");
		end
	end
	
	if pn==PLAYER_2 and GAMESTATE:IsHumanPlayer(pn)==true then
		lifeP2bar = PLife(PLAYER_2);
		SetColorHP(lifeP2bar,object,heart);
	end
	
	if heart==false and GAMESTATE:IsHumanPlayer(pn)==true then
		object:playcommand("HPcrop");
	end
							
end;

local t = Def.ActorFrame {};
local function UpdateTime(self)
	--[[
	local c = self:GetChildren();
	for pn in ivalues(PlayerNumber) do
		local vStats = STATSMAN:GetCurStageStats():GetPlayerStageStats( pn );
		local vTime;
		local obj = self:GetChild( string.format("RemainingTime" .. PlayerNumberToString(pn) ) );
		if vStats and obj then
			vTime = vStats:GetLifeRemainingSeconds()
			obj:settext( SecondsToMMSSMsMs( vTime ) );
		end;
	end;
	--]]
	if GAMESTATE:IsHumanPlayer(PLAYER_1)==true then
		UpdateHPbar(PLAYER_1,self:GetChild("P1HPbar"),false,false);
		UpdateHPbar(PLAYER_1,self:GetChild("P1Tip"),false,true);
		UpdateHPbar(PLAYER_1,self:GetChild("P1Heart"),true,false);
		self:GetChild("P1HP"):settext(string.format("%i",lifeP1bar*100));
	end;
	if GAMESTATE:IsHumanPlayer(PLAYER_2)==true then
		UpdateHPbar(PLAYER_2,self:GetChild("P2HPbar"),false,false);
		UpdateHPbar(PLAYER_2,self:GetChild("P2Heart"),true,false);
		self:GetChild("P2HP"):settext(string.format("%i",lifeP2bar*100));
	end;
end
--[[
if GAMESTATE:GetCurrentCourse() then
	if GAMESTATE:GetCurrentCourse():GetCourseType() == "CourseType_Survival" then
		-- RemainingTime
		for pn in ivalues(PlayerNumber) do
			local MetricsName = "RemainingTime" .. PlayerNumberToString(pn);
			t[#t+1] = LoadActor( THEME:GetPathG( Var "LoadingScreen", "RemainingTime"), pn ) .. {
				InitCommand=function(self) 
					self:player(pn); 
					self:name(MetricsName); 
					ActorUtil.LoadAllCommandsAndSetXY(self,Var "LoadingScreen"); 
				end;
			};
		end
		for pn in ivalues(PlayerNumber) do
			local MetricsName = "DeltaSeconds" .. PlayerNumberToString(pn);
			t[#t+1] = LoadActor( THEME:GetPathG( Var "LoadingScreen", "DeltaSeconds"), pn ) .. {
				InitCommand=function(self) 
					self:player(pn); 
					self:name(MetricsName); 
					ActorUtil.LoadAllCommandsAndSetXY(self,Var "LoadingScreen"); 
				end;
			};
		end
	end;
end; --]]

--CustomHPbar
local GeneralYpos = 8;
local GeneralXpos = 6;
local GeneralZoom = 0.6;
-- The following values are set to the half
local HeartWidth = 23;
local HeartHeight = 20;
local HPbarWidth = 150;
--local HeartP1pos = 

--PLAYER 1

t[#t+1] = LoadActor("HeartBack") .. {
	InitCommand=cmd(visible,GAMESTATE:IsHumanPlayer(PLAYER_1));
	OnCommand=cmd(valign,0.5;x,GeneralXpos+GeneralZoom*HeartWidth;y,SCREEN_TOP+GeneralYpos+GeneralZoom*HeartHeight;zoom,GeneralZoom);
}

t[#t+1] = LoadActor("HeartIn") .. {
	Name="P1Heart";
	InitCommand=cmd(visible,GAMESTATE:IsHumanPlayer(PLAYER_1);valign,0.5;x,GeneralXpos+GeneralZoom*HeartWidth;y,SCREEN_TOP+GeneralYpos+GeneralZoom*HeartHeight;zoom,GeneralZoom);
	OnCommand=cmd(effectclock,"bgm";effecttiming,0,0,0,1;diffuseshift;effectcolor1,color("#FFFFFFFF");effectcolor2,color("#FFFFFF66"));
	HealthStateChangedMessageCommand=function(self,params)
		if params.HealthState=='HealthState_Dead' then
			self:visible(false);
		end
	end;
	--[[
	LifeChangedMessageCommand=function(self,params)
					lifeP1 = params.LifeMeter:GetLife();
							if GAMESTATE:IsHumanPlayer(PLAYER_1)==true then
								if lifeP1==0 then
									--self:visible(false);
								else
									--
									--if lifeP1==1 then
									--	self:effecttiming(0,0,0,0);
									--else
									--	self:effectmagnitude(,0,0);
									--end;
									
									-- COLORS?
									if lifeP1<=0.8 then
										if lifeP1<=0.6 then
											if lifeP1<=0.4 then
												if lifeP1<=0.2 then
													self:effectcolor1( color("#FF3F3FFF") );
													self:effectcolor2( color("#FF3F3F66") );
													self:effecttiming(0.5,0,0.5,0);
												else
													self:effectcolor1( color("#FF873FFF") );
													self:effectcolor2( color("#FF873F66") );
													self:effecttiming(1,0,1,0);
												end
											else
												self:effectcolor1( color("#FFE33FFF") );
												self:effectcolor2( color("#FFE33F66") );
												self:effecttiming(0,0,0,1);
											end
										else
											self:effectcolor1( color("#95FF2BFF") );
											self:effectcolor2( color("#95FF2B66") );
											self:effecttiming(0,0,0,1);
										end
									else
										self:effectcolor1( color("#3FFF6FFF") );
										self:effectcolor2( color("#3FFF6F66") );
										self:effecttiming(0,0,0,1);
									end
							
									self:visible(true);
								end
							end
							
					end;
					]]
}
--
t[#t+1] = LoadActor("HeartFrame") .. {
	InitCommand=cmd(visible,GAMESTATE:IsHumanPlayer(PLAYER_1));
	OnCommand=cmd(valign,0.5;x,GeneralXpos+GeneralZoom*HeartWidth;y,SCREEN_TOP+GeneralYpos+GeneralZoom*HeartHeight;zoom,GeneralZoom;blend,Blend.Add);
}

t[#t+1] = LoadActor("HPbarBack") .. {
	InitCommand=cmd(visible,GAMESTATE:IsHumanPlayer(PLAYER_1));
	OnCommand=cmd(horizalign,left;x,SCREEN_LEFT+GeneralXpos-2+GeneralZoom*HeartWidth*2;valign,0.5;y,SCREEN_TOP+GeneralYpos+GeneralZoom*HeartHeight;zoom,GeneralZoom); 
}

t[#t+1] = LoadActor("HPbarLife") .. {
	Name="P1HPbar";
	InitCommand=cmd(visible,GAMESTATE:IsHumanPlayer(PLAYER_1);horizalign,left;x,SCREEN_LEFT+GeneralXpos-2+GeneralZoom*HeartWidth*2;valign,0.5;y,SCREEN_TOP+GeneralYpos+GeneralZoom*HeartHeight;zoom,GeneralZoom;cropright,1;addcommand,"HPcrop"); 
	OnCommand=cmd(effectclock,"bgm";effecttiming,0,0,0,1;diffuseshift;effectcolor1,color("#FFFFFFFF");effectcolor2,color("#FFFFFF66"));
	HPcropCommand=cmd(stoptweening;decelerate,0.1;cropright,1-lifeP1bar);
	--[[LifeChangedMessageCommand=function(self,params)
					lifeP1bar = params.LifeMeter:GetLife();
							if GAMESTATE:IsHumanPlayer(PLAYER_1)==true then
								if lifeP1bar==0 then
									self:visible(false);
								else
									--
									--if lifeP1==1 then
									--	self:effecttiming(0,0,0,0);
									--else
									--	self:effectmagnitude(,0,0);
									--end;
									
									-- COLORS?
									if lifeP1bar<=0.8 then
										if lifeP1bar<=0.6 then
											if lifeP1bar<=0.4 then
												if lifeP1bar<=0.2 then
													self:effectcolor1( color("#FF3F3FFF") );
													self:effectcolor2( color("#FF3F3F66") );
													self:effecttiming(0.5,0,0.5,0);
												else
													self:effectcolor1( color("#FF873FFF") );
													self:effectcolor2( color("#FF873F66") );
													self:effecttiming(1,0,1,0);
												end
											else
												self:effectcolor1( color("#FFE33FFF") );
												self:effectcolor2( color("#FFE33F66") );
												self:effecttiming(0,0,0,1);
											end
										else
											self:effectcolor1( color("#95FF2BFF") );
											self:effectcolor2( color("#95FF2B66") );
											self:effecttiming(0,0,0,1);
										end
									else
										self:effectcolor1( color("#3FFF6FFF") );
										self:effectcolor2( color("#3FFF6F66") );
										self:effecttiming(0,0,0,1);
									end
							
									self:visible(true);
								end
							end
							
							self:playcommand("HPcrop");
							
					end;]]
	--end;
}

local SepI;
for SepI=1,4,1 do
	t[#t+1] = LoadActor("HPbarSpl") .. {
		InitCommand=cmd(visible,GAMESTATE:IsHumanPlayer(PLAYER_1));
		OnCommand=cmd(horizalign,center;x,SCREEN_LEFT+GeneralXpos-2+GeneralZoom*HeartWidth*2+(300*GeneralZoom/5)*SepI;valign,0.5;y,SCREEN_TOP+GeneralYpos+GeneralZoom*HeartHeight;zoom,GeneralZoom;blend,Blend.Add); 
	}
end

t[#t+1] = LoadActor("HPbarFrame") .. {
	InitCommand=cmd(visible,GAMESTATE:IsHumanPlayer(PLAYER_1));
	OnCommand=cmd(horizalign,left;x,SCREEN_LEFT+GeneralXpos-2+GeneralZoom*HeartWidth*2;valign,0.5;y,SCREEN_TOP+GeneralYpos+GeneralZoom*HeartHeight;zoom,GeneralZoom;blend,Blend.Add); 
}

-- Tip
t[#t+1] = Def.ActorFrame {
	Name="P1Tip";
	InitCommand=cmd(visible,GAMESTATE:IsHumanPlayer(PLAYER_1); player,PLAYER_1);
	OnCommand=cmd(horizalign,left;x,SCREEN_LEFT+GeneralXpos-2+GeneralZoom*HeartWidth*2;valign,0.5;horizalign,center;y,SCREEN_TOP+GeneralYpos+GeneralZoom*HeartHeight;zoom,GeneralZoom);
	MoveTipCommand=function(self)
		local l_x = SCREEN_LEFT+GeneralXpos-2+GeneralZoom*HeartWidth*2;
		local l_w = SCREEN_LEFT+GeneralXpos-2+GeneralZoom*(HeartWidth+HPbarWidth)*2;
		self:stoptweening();
		self:decelerate(0.1);
		self:x(l_x+(l_w-l_x)*lifeP1bar);
	end;
	
	Def.ActorFrame {
	
	Name="Body";
	InitCommand=cmd(visible,true);
	JudgmentMessageCommand=function(self,params)
		if params.TapNoteScore=='TapNoteScore_W1' or params.TapNoteScore=='TapNoteScore_W2' or params.TapNoteScore=='TapNoteScore_W3' then
			self:stoptweening();
			self:zoom(1.2);
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

t[#t+1] = LoadFont("../../Default/Fonts/_roboto", "Bold") .. {
	Name="P1HP";		
	InitCommand=cmd(visible,GAMESTATE:IsHumanPlayer(PLAYER_1));
	OnCommand=cmd(horizalign,right;x,SCREEN_LEFT+GeneralXpos-2+GeneralZoom*(HeartWidth+HPbarWidth)*2+GeneralZoom*74;valign,0.5;y,SCREEN_TOP+GeneralYpos+HeartHeight*GeneralZoom+3*GeneralZoom;zoom,GeneralZoom*0.8); 
	
}


--PLAYER 2

t[#t+1] = LoadActor("HeartBack") .. {
	InitCommand=cmd(visible,GAMESTATE:IsHumanPlayer(PLAYER_2));
	OnCommand=cmd(valign,0.5;x,SCREEN_RIGHT-GeneralXpos-GeneralZoom*HeartWidth;y,SCREEN_TOP+GeneralYpos+GeneralZoom*HeartHeight;zoom,GeneralZoom);
}

t[#t+1] = LoadActor("HeartIn") .. {
	Name="P2Heart";
	InitCommand=cmd(visible,GAMESTATE:IsHumanPlayer(PLAYER_2);valign,0.5;x,SCREEN_RIGHT-GeneralXpos-GeneralZoom*HeartWidth;y,SCREEN_TOP+GeneralYpos+GeneralZoom*HeartHeight;zoom,GeneralZoom);
	OnCommand=cmd(effectclock,"bgm";effecttiming,0,0,0,1;diffuseshift;effectcolor1,color("#FFFFFFFF");effectcolor2,color("#FFFFFF66"));
	HealthStateChangedMessageCommand=function(self,params)
		if params.HealthState=='HealthState_Dead' then
			self:visible(false);
		end
	end;
	--[[
	LifeChangedMessageCommand=function(self,params)
					lifeP1 = params.LifeMeter:GetLife();
							if GAMESTATE:IsHumanPlayer(PLAYER_1)==true then
								if lifeP1==0 then
									--self:visible(false);
								else
									--
									--if lifeP1==1 then
									--	self:effecttiming(0,0,0,0);
									--else
									--	self:effectmagnitude(,0,0);
									--end;
									
									-- COLORS?
									if lifeP1<=0.8 then
										if lifeP1<=0.6 then
											if lifeP1<=0.4 then
												if lifeP1<=0.2 then
													self:effectcolor1( color("#FF3F3FFF") );
													self:effectcolor2( color("#FF3F3F66") );
													self:effecttiming(0.5,0,0.5,0);
												else
													self:effectcolor1( color("#FF873FFF") );
													self:effectcolor2( color("#FF873F66") );
													self:effecttiming(1,0,1,0);
												end
											else
												self:effectcolor1( color("#FFE33FFF") );
												self:effectcolor2( color("#FFE33F66") );
												self:effecttiming(0,0,0,1);
											end
										else
											self:effectcolor1( color("#95FF2BFF") );
											self:effectcolor2( color("#95FF2B66") );
											self:effecttiming(0,0,0,1);
										end
									else
										self:effectcolor1( color("#3FFF6FFF") );
										self:effectcolor2( color("#3FFF6F66") );
										self:effecttiming(0,0,0,1);
									end
							
									self:visible(true);
								end
							end
							
					end;
					]]
}
--
t[#t+1] = LoadActor("HeartFrame") .. {
	InitCommand=cmd(visible,GAMESTATE:IsHumanPlayer(PLAYER_2));
	OnCommand=cmd(valign,0.5;x,SCREEN_RIGHT-GeneralXpos-GeneralZoom*HeartWidth;y,SCREEN_TOP+GeneralYpos+GeneralZoom*HeartHeight;zoom,GeneralZoom;blend,Blend.Add);
}

t[#t+1] = LoadActor("HPbarBack") .. {
	InitCommand=cmd(visible,GAMESTATE:IsHumanPlayer(PLAYER_2));
	OnCommand=cmd(horizalign,right;x,SCREEN_RIGHT-GeneralXpos+2-GeneralZoom*HeartWidth*2;valign,0.5;y,SCREEN_TOP+GeneralYpos+GeneralZoom*HeartHeight;zoom,GeneralZoom); 
}

t[#t+1] = LoadActor("HPbarLife") .. {
	Name="P2HPbar";
	InitCommand=cmd(visible,GAMESTATE:IsHumanPlayer(PLAYER_2);horizalign,right;x,SCREEN_RIGHT-GeneralXpos+2-GeneralZoom*HeartWidth*2;valign,0.5;y,SCREEN_TOP+GeneralYpos+GeneralZoom*HeartHeight;zoom,GeneralZoom;cropleft,1;addcommand,"HPcrop"); 
	OnCommand=cmd(effectclock,"bgm";effecttiming,0,0,0,1;diffuseshift;effectcolor1,color("#FFFFFFFF");effectcolor2,color("#FFFFFF66"));
	HPcropCommand=cmd(stoptweening;decelerate,0.1;cropleft,1-lifeP2bar);
	--[[LifeChangedMessageCommand=function(self,params)
					lifeP1bar = params.LifeMeter:GetLife();
							if GAMESTATE:IsHumanPlayer(PLAYER_1)==true then
								if lifeP1bar==0 then
									self:visible(false);
								else
									--
									--if lifeP1==1 then
									--	self:effecttiming(0,0,0,0);
									--else
									--	self:effectmagnitude(,0,0);
									--end;
									
									-- COLORS?
									if lifeP1bar<=0.8 then
										if lifeP1bar<=0.6 then
											if lifeP1bar<=0.4 then
												if lifeP1bar<=0.2 then
													self:effectcolor1( color("#FF3F3FFF") );
													self:effectcolor2( color("#FF3F3F66") );
													self:effecttiming(0.5,0,0.5,0);
												else
													self:effectcolor1( color("#FF873FFF") );
													self:effectcolor2( color("#FF873F66") );
													self:effecttiming(1,0,1,0);
												end
											else
												self:effectcolor1( color("#FFE33FFF") );
												self:effectcolor2( color("#FFE33F66") );
												self:effecttiming(0,0,0,1);
											end
										else
											self:effectcolor1( color("#95FF2BFF") );
											self:effectcolor2( color("#95FF2B66") );
											self:effecttiming(0,0,0,1);
										end
									else
										self:effectcolor1( color("#3FFF6FFF") );
										self:effectcolor2( color("#3FFF6F66") );
										self:effecttiming(0,0,0,1);
									end
							
									self:visible(true);
								end
							end
							
							self:playcommand("HPcrop");
							
					end;]]
	--end;
}

for SepI=1,4,1 do
	t[#t+1] = LoadActor("HPbarSpl") .. {
		InitCommand=cmd(visible,GAMESTATE:IsHumanPlayer(PLAYER_2));
		OnCommand=cmd(horizalign,center;x,SCREEN_RIGHT-GeneralXpos+2-GeneralZoom*HeartWidth*2-(300*GeneralZoom/5)*SepI;valign,0.5;y,SCREEN_TOP+GeneralYpos+GeneralZoom*HeartHeight;zoom,GeneralZoom;blend,Blend.Add); 
	}
end

t[#t+1] = LoadActor("HPbarFrame") .. {
	InitCommand=cmd(visible,GAMESTATE:IsHumanPlayer(PLAYER_2));
	OnCommand=cmd(horizalign,right;x,SCREEN_RIGHT-GeneralXpos+2-GeneralZoom*HeartWidth*2;valign,0.5;y,SCREEN_TOP+GeneralYpos+GeneralZoom*HeartHeight;zoom,GeneralZoom;blend,Blend.Add); 
}

t[#t+1] = LoadFont("../../Default/Fonts/_roboto", "Bold") .. {
	Name="P2HP";		
	InitCommand=cmd(visible,GAMESTATE:IsHumanPlayer(PLAYER_2));
	OnCommand=cmd(horizalign,right;x,SCREEN_RIGHT-GeneralXpos-GeneralZoom*(HeartWidth+HPbarWidth)*2;valign,0.5;y,SCREEN_TOP+GeneralYpos+HeartHeight*GeneralZoom+3*GeneralZoom;zoom,GeneralZoom*0.8); 
	
}



t.InitCommand=cmd(SetUpdateFunction,UpdateTime);
return t
