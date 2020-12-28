function funcMinSecondsToMusic()
	local song = GAMESTATE:GetCurrentSong()
	local bgc = song:GetBGChanges()
	local td = song:GetTimingData()
	--if (type(s)=="nil") or type(bgc)=="nil" or type(td)=="nil" then
	--	return 4.0
	--end
	
	local function isVideo(filename)
		return not (string.find(filename,".mov")==nil and string.find(filename,".avi")==nil and string.find(filename,".mp4")==nil)
	end
	
	local j = 1
	local flag = false
	for i=1,#bgc do
		local f1 = bgc[i]["file1"]
		if isVideo(f1) then
			flag=true
			j = i
			break
		end
	end
	local res = 0.0
	if flag then
		res = song:GetFirstSecond() - td:GetElapsedTimeFromBeat( bgc[j]["start_beat"] )
	end
	-- Calc based on first bpm (max 5.0 seconds)
	res = math.max( math.min(400/td:GetBPMs()[1], 5.0) , res)
	
	return math.max(res, 0.5)
end

