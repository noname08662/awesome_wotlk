local combatFrame = CreateFrame("Frame")

combatFrame:RegisterEvent("PLAYER_REGEN_DISABLED")
combatFrame:RegisterEvent("CHAT_MSG_WHISPER")
combatFrame:SetScript("OnEvent", function(self, event)
    if event == "PLAYER_REGEN_DISABLED" or event == "CHAT_MSG_WHISPER" then
        if not IsWindowFocused() then
            FlashWindow()
        end
    end
end)