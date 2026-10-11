local addonName, ACVar = ...
local L = ACVar.L
local CONSTANTS = ACVar.CONSTANTS

local _G = _G
local tonumber = tonumber
local tostring = tostring
local lower = string.lower
local match = string.match
local format = string.format
local trim = string.trim
local math_cos = math.cos
local math_sin = math.sin
local math_rad = math.rad
local math_deg = math.deg
local math_atan2 = math.atan2
local math_max = math.max

local GetCVar = GetCVar
local SetCVar = SetCVar
local CreateFrame = CreateFrame
local SlashCmdList = SlashCmdList
local GetCursorPosition = GetCursorPosition
local InterfaceOptions_AddCategory = InterfaceOptions_AddCategory

-- Formats only when arguments are given, so a plain message may contain a literal "%".
local function formatMessage(template, ...)
    local text = select("#", ...) > 0 and format(template, ...) or template
    return CONSTANTS.COLORS.SUCCESS..L.ADDON_NAME..":"..CONSTANTS.COLORS.RESET.." "..text
end

function ACVar:PrintMessage(message, ...)
    _G.DEFAULT_CHAT_FRAME:AddMessage(formatMessage(message, ...))
end

function ACVar:PrintCVarChange(cvarName, value, label)
    if not self.DB.showChatMessages then return end
    ACVar:PrintMessage(
        L.MSG_SET_VALUE,
        CONSTANTS.COLORS.HIGHLIGHT..cvarName..CONSTANTS.COLORS.RESET,
        CONSTANTS.COLORS.VALUE..tostring(value)..(label and format(" (%s)", label) or "")..CONSTANTS.COLORS.RESET
    )
end

function ACVar:GetCVarValue(cvarName)
    local value = GetCVar(cvarName)
    return tonumber(value) or value
end

function ACVar:SetCVarValue(cvarName, value)
    if self:GetCVarValue(cvarName) then
        SetCVar(cvarName, value)
        if self.Frame then
            self:UpdateDependencies()
        end
    end
end

function ACVar.FormatNumber(value)
    return tonumber(format("%.2f", value or 0)) or 0
end

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
        ACVar:PrintMessage("%s", CONSTANTS.COLORS.ERROR..L.MSG_UNKNOWN_COMMAND..CONSTANTS.COLORS.RESET)
    end
end

function ACVar:UpdateMinimapButton()
    if not self.MinimapButton then return end
    if self.DB.minimap.hide then
        self.MinimapButton:Hide()
    else
        self.MinimapButton:Show()
    end
end

function ACVar:CreateMinimapButton()
    local button = CreateFrame("Button", "AwesomeCVarMinimapButton", Minimap)
    self.MinimapButton = button
    button:SetFrameStrata("MEDIUM")
    button:SetSize(31, 31)
    button:SetFrameLevel(8)
    button:RegisterForClicks("AnyUp")
    button:RegisterForDrag("LeftButton")
    button:SetHighlightTexture("Interface\\Minimap\\UI-Minimap-ZoomButton-Highlight")

    local overlay = button:CreateTexture(nil, "OVERLAY")
    overlay:SetWidth(53)
    overlay:SetHeight(53)
    overlay:SetTexture("Interface\\Minimap\\MiniMap-TrackingBorder")
    overlay:SetPoint("TOPLEFT")

    local icon = button:CreateTexture(nil, "BACKGROUND")
    icon:SetWidth(20)
    icon:SetHeight(20)
    icon:SetTexture("Interface\\Icons\\Trade_Engineering")
    icon:SetTexCoord(0.05, 0.95, 0.05, 0.95)
    icon:SetPoint("TOPLEFT", 7, -5)

    local function updatePosition()
        local angle = math_rad(self.DB.minimap.minimapPos or 220)
        local x = math_cos(angle) * 80
        local y = math_sin(angle) * 80
        button:SetPoint("CENTER", Minimap, "CENTER", x, y)
    end

    button:SetScript("OnDragStart", function(self)
        self:LockHighlight()
        self:SetScript("OnUpdate", function()
            local mx, my = Minimap:GetCenter()
            local px, py = GetCursorPosition()
            local scale = Minimap:GetEffectiveScale()
            px, py = px / scale, py / scale
            local angle = math_deg(math_atan2(py - my, px - mx))
            ACVar.DB.minimap.minimapPos = angle
            updatePosition()
        end)
    end)

    button:SetScript("OnDragStop", function(self)
        self:UnlockHighlight()
        self:SetScript("OnUpdate", nil)
    end)

    button:SetScript("OnClick", function()
        ACVar:ToggleFrame()
    end)

    button:SetScript("OnEnter", function(self)
        GameTooltip:SetOwner(self, "ANCHOR_LEFT")
        GameTooltip:AddLine(L.ADDON_NAME)
        GameTooltip:AddLine(L.MINIMAP_TOOLTIP, 1, 1, 1)
        GameTooltip:Show()
    end)

    button:SetScript("OnLeave", function()
        GameTooltip:Hide()
    end)

    updatePosition()
    self:UpdateMinimapButton()
end

function ACVar:CreateBlizzOptions()
    local panel = CreateFrame("Frame", "AwesomeCVarBlizzPanel", UIParent)
    panel.name = L.ADDON_NAME_SHORT

    local title = panel:CreateFontString(nil, "ARTWORK", "GameFontNormalLarge")
    title:SetPoint("TOPLEFT", 16, -16)
    title:SetText(panel.name)

    local openBtn = CreateFrame("Button", "AwesomeCVarBlizzOpenBtn", panel, "UIPanelButtonTemplate")
    openBtn:SetWidth(150)
    openBtn:SetHeight(30)
    openBtn:SetPoint("TOPLEFT", title, "BOTTOMLEFT", 0, -20)
    openBtn:SetText(L.OPEN_ADDON)
    openBtn:SetScript("OnClick", function()
        if InterfaceOptionsFrame and InterfaceOptionsFrame:IsShown() then
            HideUIPanel(InterfaceOptionsFrame)
        end
        ACVar:ShowFrame()
    end)

    InterfaceOptions_AddCategory(panel)
end

function ACVar:UpdateGameMenuButton()
    if not self.GameMenuButton then return end
    if self.DB.showGameMenuButton then
        self.GameMenuButton:Show()
    else
        self.GameMenuButton:Hide()
    end
end

function ACVar:OnLoad()
    if self.isLoaded then return end
    self:CreateMainFrame()
    self:CreateDefaultConfirmationPopup()
    self:AddGameMenuButton()
    self:CreateMinimapButton()
    self:CreateBlizzOptions()
    if self.DB.showChatMessages then
        self:PrintMessage(L.MSG_LOADED)
    end
    if self.GetVersionStatus() ~= "ok" then
        self:PrintMessage("%s", self.GetVersionStatusText())
    end
    self.isLoaded = true
end

-- Version broadcast to group and guild; a newer version from anyone is reported once per session.
-- The payload is a leading integer and the rest is ignored, so fields can be appended later.
do
    local PREFIX = "AwesomeWotlk"
    local BROADCAST_DELAY = 10

    -- The newer of the two, so a half-updated install still relays the newest release it knows of.
    local function knownVersion()
        return math_max(ACVar.MOD_VERSION, tonumber(_G.AwesomeWotlk) or 0)
    end

    local function groupChannel()
        if GetNumRaidMembers() > 1 then
            local _, instanceType = IsInInstance()
            return instanceType == "pvp" and "BATTLEGROUND" or "RAID"
        elseif GetNumPartyMembers() > 0 then
            return "PARTY"
        end
    end

    local function groupSize()
        local raid = GetNumRaidMembers()
        return raid > 0 and raid or GetNumPartyMembers() + 1
    end

    local frame = CreateFrame("Frame")
    frame:Hide()
    local waited, sendGroup, sendGuild = 0, false, false
    local lastGroupSize, notified = 0, false

    -- Requests within BROADCAST_DELAY collapse into one send.
    local function schedule(group, guild)
        sendGroup = sendGroup or group
        sendGuild = sendGuild or guild
        if not frame:IsShown() then
            waited = 0
            frame:Show()
        end
    end

    local function send(group, guild)
        local message = tostring(knownVersion())
        local channel = group and groupChannel()
        if channel then SendAddonMessage(PREFIX, message, channel) end
        if guild and IsInGuild() then SendAddonMessage(PREFIX, message, "GUILD") end
    end

    local function receive(message)
        if notified or (ACVar.DB and not ACVar.DB.showVersionAlerts) then return end
        local digits = message and match(message, "^(%d+)")
        local version = digits and #digits <= 6 and tonumber(digits)
        if version and version > knownVersion() then
            notified = true
            ACVar:PrintMessage(L.VERSION_NEWER_SEEN, version, ACVar.URLS.RELEASES)
        end
    end

    frame:SetScript("OnUpdate", function(self, elapsed)
        waited = waited + elapsed
        if waited < BROADCAST_DELAY then return end
        self:Hide()
        send(sendGroup, sendGuild)
        sendGroup, sendGuild = false, false
    end)

    frame:RegisterEvent("CHAT_MSG_ADDON")
    frame:RegisterEvent("PARTY_MEMBERS_CHANGED")
    frame:RegisterEvent("RAID_ROSTER_UPDATE")
    frame:RegisterEvent("PLAYER_ENTERING_WORLD")
    frame:SetScript("OnEvent", function(self, event, prefix, message, _, sender)
        if event == "CHAT_MSG_ADDON" then
            if prefix == PREFIX and sender ~= UnitName("player") then receive(message) end
        elseif event == "PLAYER_ENTERING_WORLD" then
            -- Fires on every loading screen; broadcast once per login or /reload.
            self:UnregisterEvent(event)
            lastGroupSize = groupSize()
            schedule(true, true)
        else
            -- Only newcomers need our version.
            local size = groupSize()
            if size > 1 and size > lastGroupSize then schedule(true, false) end
            lastGroupSize = size
        end
    end)
end

local eventFrame = CreateFrame("Frame")
eventFrame:RegisterEvent("ADDON_LOADED")
eventFrame:RegisterEvent("PLAYER_ENTERING_WORLD")
eventFrame:SetScript("OnEvent", function(self, event, arg)
    if event == "ADDON_LOADED" then
        if arg == addonName then
            local db = _G.AwesomeCVarDB or {}
            _G.AwesomeCVarDB = db
            db.minimap = db.minimap or {}
            if db.minimap.hide == nil then db.minimap.hide = false end
            if db.minimap.minimapPos == nil then db.minimap.minimapPos = 220 end
            if db.showGameMenuButton == nil then db.showGameMenuButton = true end
            if db.showChatMessages == nil then db.showChatMessages = true end
            if db.showVersionAlerts == nil then db.showVersionAlerts = true end

            ACVar.DB = db
            ACVar:OnLoad()
        end
    elseif event == "PLAYER_ENTERING_WORLD" then
        for _, f in pairs(ACVar.Skins or {}) do
            f()
        end
        self:UnregisterEvent(event)
    end
end)

SLASH_AWESOME1 = "/awesome"
SLASH_AWESOME2 = "/awesomecvar"
SlashCmdList["AWESOME"] = processSlashCommand
