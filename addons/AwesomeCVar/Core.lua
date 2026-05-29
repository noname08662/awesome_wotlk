-- File: Core.lua
-- Main addon logic, event handling, and slash command processing.

local addonName, ACVar = ...
local L = ACVar.L or {}
local CONSTANTS = ACVar.CONSTANTS

local _G = _G
local tonumber = tonumber
local tostring = tostring
local lower = string.lower
local format = string.format
local trim = string.trim

local GetCVar = GetCVar
local SetCVar = SetCVar
local CreateFrame = CreateFrame
local SlashCmdList = SlashCmdList

ACVar.reloadIsPending = false

local function formatMessage(template, ...)
    return CONSTANTS.COLORS.SUCCESS..L.ADDON_NAME..":"..CONSTANTS.COLORS.RESET.." "..string.format(template, ...)
end

function ACVar:PrintMessage(message, ...)
    _G.DEFAULT_CHAT_FRAME:AddMessage(formatMessage(message, ...))
end

function ACVar:PrintCVarChange(cvarName, value, label)
    local baseMessage = format(
        L.MSG_SET_VALUE,
        CONSTANTS.COLORS.HIGHLIGHT..cvarName..CONSTANTS.COLORS.RESET,
        CONSTANTS.COLORS.VALUE..tostring(value)..CONSTANTS.COLORS.RESET
    )
	ACVar:PrintMessage(
		label and format("%s %s(%s)%s", baseMessage, CONSTANTS.COLORS.VALUE, label, CONSTANTS.COLORS.RESET)
		or baseMessage
	)
end

function ACVar:GetCVarValue(cvarName)
    local value = GetCVar(cvarName)
    return tonumber(value) or value
end

function ACVar:SetCVarValue(cvarName, value, cvarDef)
    if self:GetCVarValue(cvarName) then
        SetCVar(cvarName, value)
        if cvarDef and cvarDef.reloadRequired then
            self.reloadIsPending = true
        end
    end
end

function ACVar.FormatNumber(value)
    return tonumber(format("%.2f", value or 0)) or 0
end

-- ### Slash Command Handler ###
local function processSlashCommand(msg)
    msg = lower(trim(msg))

    if msg == "" or msg == "toggle" then
        ACVar:ToggleFrame()
    elseif msg == "show" then
        ACVar:ShowFrame()
    elseif msg == "hide" then
        ACVar:HideFrame()
    elseif msg == "reset" or msg == "resetposition" then
        ACVar:ResetFramePosition()
    elseif msg == "help" then
        ACVar:PrintMessage(L.MSG_HELP_HEADER)
        ACVar:PrintMessage(L.MSG_HELP_TOGGLE)
        ACVar:PrintMessage(L.MSG_HELP_SHOW)
        ACVar:PrintMessage(L.MSG_HELP_HIDE)
        ACVar:PrintMessage(L.MSG_HELP_RESET)
        ACVar:PrintMessage(L.MSG_HELP_HELP)
    else
        _G.DEFAULT_CHAT_FRAME:AddMessage(CONSTANTS.COLORS.ERROR..L.MSG_UNKNOWN_COMMAND)
    end
end

-- ### Initialization and Event Handling ###
function ACVar:OnLoad()
	if self.isLoaded then return end
    self:CreateMainFrame()
    self:CreateReloadPopup()
    self:CreateDefaultConfirmationPopup()
    self:AddGameMenuButton()
    self:PrintMessage(L.MSG_LOADED)
	self.isLoaded = true
end

local eventFrame = CreateFrame("Frame")
eventFrame:RegisterEvent("ADDON_LOADED")
eventFrame:RegisterEvent("PLAYER_ENTERING_WORLD")
eventFrame:SetScript("OnEvent", function(self, event, arg)
    if event == "ADDON_LOADED" then
		if arg == addonName then
			ACVar:OnLoad()
		end
	elseif event == "PLAYER_ENTERING_WORLD" then
		for _, f in pairs(ACVar.Skins) do
			f()
		end
		self:UnregisterEvent(event)
    end
end)

-- Register Slash Commands
SLASH_AWESOME1 = "/awesome"
SLASH_AWESOME2 = "/awesomecvar"
SlashCmdList["AWESOME"] = processSlashCommand
