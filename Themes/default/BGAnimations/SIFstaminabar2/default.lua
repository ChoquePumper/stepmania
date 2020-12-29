local t = Def.ActorFrame {};

function PLifeGlobal(pn)
	return SCREENMAN:GetTopScreen():GetLifeMeter(pn):GetLife();
end;

if GAMESTATE:IsHumanPlayer(PLAYER_1)==true then
t[#t+1] = LoadActor("PlayerBar.lua", PLAYER_1) .. {
	InitCommand=cmd(visible,true);
	OnCommand=cmd(horizalign, left; valign,0.5; x,SCREEN_LEFT+2; y,SCREEN_TOP+16; zoom,0.6);
};
end;

if GAMESTATE:IsHumanPlayer(PLAYER_2)==true then
t[#t+1] = LoadActor("PlayerBar.lua", PLAYER_2) .. {
	InitCommand=cmd(visible,true);
	OnCommand=cmd(horizalign, right; valign,0.5; x,SCREEN_RIGHT-2; y,SCREEN_TOP+16; zoom,0.6);
};
end

return t;
