-- File: UI.lua
-- Contains all functions for creating and managing UI elements.

local addonName, ACVar = ...
local L = ACVar.L or {}
local CONSTANTS = ACVar.CONSTANTS
local CVARS = ACVar.CVARS

local _G = _G
local ipairs = ipairs
local max = math.max
local pairs = pairs
local tinsert = table.insert
local tostring = tostring
local unpack = unpack

local CreateFrame = CreateFrame
local HideUIPanel = HideUIPanel
local PlaySound = PlaySound
local ReloadUI = ReloadUI
local UIParent = UIParent

local function getFrameName(prefix, suffix)
    return CONSTANTS.ADDON_NAME.."_"..prefix..(suffix or "")
end

local function createButton(parent, name, text, width, height, template)
    template = template or "UIPanelButtonTemplate"
    local button = CreateFrame("Button", name, parent, template)
    button:SetWidth(width or CONSTANTS.FRAME.BUTTON_WIDTH)
    button:SetHeight(height or CONSTANTS.FRAME.BUTTON_HEIGHT)
    button:SetText(text)
    return button
end

local function createPopupFrame(name, title, message, width, height)
    local frame = CreateFrame("Frame", name, UIParent, "DialogBoxFrame")
    frame:SetPoint("CENTER")
    frame:SetWidth(width or CONSTANTS.FRAME.POPUP_WIDTH)
    frame:SetHeight(height or CONSTANTS.FRAME.POPUP_HEIGHT)
    frame:SetFrameStrata("DIALOG")
    frame:Hide()
    frame:SetBackdropColor(0, 0, 0, 0.8)

    if _G[name.."Button"] then
        _G[name.."Button"]:Hide()
    end

    local titleText = frame:CreateFontString(nil, "OVERLAY", "GameFontNormal")
    titleText:SetPoint("TOP", 0, -16)
    titleText:SetText(title)

    local messageText = frame:CreateFontString(nil, "OVERLAY", "GameFontNormal")
    messageText:SetPoint("TOP", titleText, "BOTTOM", 0, -10)
    messageText:SetWidth(frame:GetWidth() - 40)
    messageText:SetJustifyH("CENTER")
    messageText:SetText(message)

    return frame
end

-- ### UI Update and Control Creation Functions ###
local function createToggleControl(control, cvarDef, text)
    local checkbox = CreateFrame("CheckButton", getFrameName(cvarDef.name, "Checkbox"), control, "UICheckButtonTemplate")
    checkbox:SetPoint("LEFT", text, "RIGHT", 10, 0)
    checkbox.cvarDef = cvarDef
    checkbox:SetScript("OnClick", function(self)
        local checked = self:GetChecked()
        local newVal = checked and self.cvarDef.max or self.cvarDef.min
        ACVar:SetCVarValue(self.cvarDef.name, newVal, self.cvarDef)
        ACVar:PrintCVarChange(self.cvarDef.name, newVal)
        PlaySound(checked and "igMainMenuOptionCheckBoxOn" or "igMainMenuOptionCheckBoxOff")
    end)
    return 25
end

local function createSliderControl(control, cvarDef, text, descText)
    local slider = CreateFrame("Slider", getFrameName(cvarDef.name, "Slider"), control, "OptionsSliderTemplate")
    _G[slider:GetName().."Low"]:SetText(cvarDef.min)
    _G[slider:GetName().."High"]:SetText(cvarDef.max)
    slider:SetMinMaxValues(cvarDef.min, cvarDef.max)
    slider:SetValueStep(cvarDef.step or 1)

    if descText then
        slider:SetPoint("TOPLEFT", descText, "BOTTOMLEFT", 0, -10)
    else
        slider:SetPoint("TOPLEFT", text, "BOTTOMLEFT", 0, -10)
    end
    slider:SetPoint("RIGHT", control, "RIGHT", -100, 0)

    local valueText = control:CreateFontString(getFrameName(cvarDef.name, "SliderValue"), "ARTWORK", "GameFontNormal")
    valueText:SetPoint("TOP", slider, "BOTTOM", 0, 0)

    slider.cvarDef = cvarDef
    slider.valueText = valueText

    local resetButton = createButton(control, getFrameName(cvarDef.name, "ResetButton"), string.format(L.RESET_TO, cvarDef.default), 100, 20)
    resetButton:SetPoint("LEFT", slider, "RIGHT", 12, -4)
    resetButton:Hide()
    resetButton.cvarDef = cvarDef
    resetButton.slider = slider
    resetButton:SetScript("OnClick", function(self)
        ACVar:SetCVarValue(self.cvarDef.name, self.cvarDef.default, self.cvarDef)
        ACVar:PrintCVarChange(self.cvarDef.name, self.cvarDef.default)
        self.slider:SetValue(self.cvarDef.default)
        self:Hide()

        PlaySound("igMainMenuOptionFaerTab")
    end)

    slider:SetScript("OnValueChanged", function(self, val)
        val = ACVar.FormatNumber(val)
        self.valueText:SetText(tostring(val))
        ACVar:SetCVarValue(self.cvarDef.name, val, self.cvarDef)
        self.pendingValue = val
        ACVar:UpdateResetButtonVisibility(self.cvarDef, val)
    end)

    slider:SetScript("OnMouseUp", function(self)
        if self.pendingValue then
            ACVar:PrintCVarChange(self.cvarDef.name, self.pendingValue)
        end
    end)
    return 40
end

local function createDropdownControl(control, cvarDef, text, descText)
    local dropdown = CreateFrame("Frame", getFrameName(cvarDef.name, "Dropdown"), control, "UIDropDownMenuTemplate")

    if descText then
        dropdown:SetPoint("TOPLEFT", descText, "BOTTOMLEFT", 0, -10)
    else
        dropdown:SetPoint("TOPLEFT", text, "BOTTOMLEFT", 0, -10)
    end
    dropdown:SetPoint("RIGHT", control, "RIGHT", -100, 0)

    UIDropDownMenu_SetWidth(dropdown, 160)
    dropdown.cvarDef = cvarDef

    local function getCurrentValue()
        return (ACVar and ACVar.GetCVarValue and ACVar:GetCVarValue(cvarDef.name)) or cvarDef.default
    end

    local function setClosedLabel(val)
        local label = cvarDef.options[val] or tostring(val)
        UIDropDownMenu_SetText(dropdown, label)
        UIDropDownMenu_SetSelectedValue(dropdown, val)
    end

    local function getMinMaxIndex(t)
        local minK, maxK
        for k in pairs(t or {}) do
            if type(k) == "number" then
                if not minK or k < minK then minK = k end
                if not maxK or k > maxK then maxK = k end
            end
        end
        return minK or 0, maxK or -1
    end

    UIDropDownMenu_Initialize(dropdown, function(self, level)
        local cur = getCurrentValue()
        local minK, maxK = getMinMaxIndex(cvarDef.options)

        for i = minK, maxK do
            local label = (cvarDef.options or {})[i]
            if label ~= nil then
                local info = UIDropDownMenu_CreateInfo()
                info.text = label
                info.value = i
                info.checked = (i == cur)
                info.func = function()
                    UIDropDownMenu_SetSelectedValue(dropdown, i)
                    UIDropDownMenu_SetText(dropdown, label)

                    ACVar:SetCVarValue(cvarDef.name, i, cvarDef)
                    dropdown.pendingValue = i
                    ACVar:UpdateResetButtonVisibility(cvarDef, i)
                    ACVar:PrintCVarChange(cvarDef.name, i)
                    PlaySound("igMainMenuOptionCheckBoxOn")
                    CloseDropDownMenus()
                end
                UIDropDownMenu_AddButton(info, level)
            end
        end
    end)

    setClosedLabel(getCurrentValue())

    local resetButton = createButton(
        control,
        getFrameName(cvarDef.name, "ResetButton"),
        string.format(L.RESET_TO, tostring(cvarDef.options[cvarDef.default] or cvarDef.default)),
        100, 20
    )
    resetButton:SetPoint("LEFT", dropdown, "RIGHT", 12, -4)
    resetButton:Hide()
    resetButton.cvarDef = cvarDef
    resetButton.dropdown = dropdown

    resetButton:SetScript("OnClick", function(self)
        ACVar:SetCVarValue(self.cvarDef.name, self.cvarDef.default, self.cvarDef)
        ACVar:PrintCVarChange(self.cvarDef.name, self.cvarDef.default)
        setClosedLabel(self.cvarDef.default)
        PlaySound("igMainMenuOptionCheckBoxOn")
        self:Hide()
    end)

    dropdown.resetButton = resetButton
    ACVar:UpdateResetButtonVisibility(cvarDef, getCurrentValue())

    return 40
end

local function createModeControl(control, cvarDef, text, descText)
    local currentOffsetY = (descText and descText:GetHeight() + 5) or 0
    for j, mode in ipairs(cvarDef.modes) do
        currentOffsetY = currentOffsetY + 20
        local radio = CreateFrame("CheckButton", getFrameName(cvarDef.name, "Radio"..j), control, "UIRadioButtonTemplate")
        radio:SetPoint("TOPLEFT", text, "TOPLEFT", 0, -currentOffsetY)

        local label = radio:CreateFontString(nil, "OVERLAY", "GameFontNormal")
        label:SetPoint("LEFT", radio, "RIGHT", 5, 0)
        label:SetText(mode.label)

        radio.cvarDef = cvarDef
        radio.modeValue = mode.value
        radio:SetScript("OnClick", function(self)
            for k = 1, #self.cvarDef.modes do
                _G[getFrameName(self.cvarDef.name, "Radio"..k)]:SetChecked(false)
            end
            self:SetChecked(true)
            ACVar:SetCVarValue(self.cvarDef.name, self.modeValue, self.cvarDef)
            ACVar:PrintCVarChange(self.cvarDef.name, self.modeValue)
            PlaySound("igMainMenuOptionCheckBoxOn")
        end)
    end
    return 25 * #cvarDef.modes
end

function ACVar:UpdateResetButtonVisibility(cvarDef, currentValue)
    local resetButton = _G[getFrameName(cvarDef.name, "ResetButton")]
    if resetButton then
        if self.FormatNumber(currentValue) ~= self.FormatNumber(cvarDef.default) then
            resetButton:Show()
        else
            resetButton:Hide()
        end
    end
end

function ACVar:UpdateUIForCVar(cvarDef)
    local cvarName = cvarDef.name
    local currentValue = self:GetCVarValue(cvarName)
    if cvarDef.type == "toggle" then
        _G[getFrameName(cvarName, "Checkbox")]:SetChecked(currentValue == cvarDef.max)
    elseif cvarDef.type == "slider" then
        local slider = _G[getFrameName(cvarName, "Slider")]
        slider:SetValue(currentValue or 0)
        _G[getFrameName(cvarName, "SliderValue")]:SetText(tostring(currentValue))
        self:UpdateResetButtonVisibility(cvarDef, currentValue)
    elseif cvarDef.type == "mode" then
        for i, mode in ipairs(cvarDef.modes) do
            _G[getFrameName(cvarName, "Radio"..i)]:SetChecked(currentValue == mode.value)
        end
    elseif cvarDef.type == "dropdown" then
        local dropdown = _G[getFrameName(cvarName, "Dropdown")]
        UIDropDownMenu_SetSelectedValue(dropdown, currentValue)
        local label = cvarDef.options[currentValue] or tostring(currentValue)
        UIDropDownMenu_SetText(dropdown, label)
        self:UpdateResetButtonVisibility(cvarDef, currentValue)
    end
end

function ACVar:UpdateAllUI()
    for _, cvarList in pairs(CVARS) do
        for _, cvarDef in ipairs(cvarList) do
            self:UpdateUIForCVar(cvarDef)
        end
    end
end

function ACVar:ToggleFrame(tabName)
    if self.Frame then
        if self.Frame:IsShown() then
            self:HideFrame()
            PlaySound("igMainMenuOptionFaerTab")
        else
            self:ShowFrame(tabName)
            PlaySound("igMainMenuContinue")
        end
    end
end
-- Open API function usage: AwesomeCVar:ToggleFrame("Nameplates")
_G["AwesomeCVar"].ToggleFrame = function(self, tabName) ACVar:ToggleFrame(tabName) end

function ACVar:ShowFrame(tabName)
    if self.Frame then
        self.Frame:Show()
        self:UpdateAllUI()
        if self._SelectTab and self.TabsByName and self.TabsByName[tabName] then
            self._SelectTab(self.TabsByName[tabName])
        end
        PlaySound("igMainMenuContinue")
    end
end

function ACVar:HideFrame()
    if self.Frame then
        self.Frame:Hide()
        PlaySound("igMainMenuOptionFaerTab")
    end
end

function ACVar:ResetFramePosition()
    if self.Frame then
        self.Frame:ClearAllPoints()
        self.Frame:SetPoint("CENTER")
        self:PrintMessage(L.MSG_FRAME_RESET)
    end
end

-- ### Main Frame Creation Functions ###
function ACVar:CreateReloadPopup()
    local frame = createPopupFrame("AwesomeCVarReloadPopup", L.RELOAD_POPUP_TITLE, L.RELOAD_POPUP_TEXT)
    self.ReloadPopupFrame = frame

    local acceptButton = createButton(frame, "AwesomeCVarAcceptButton", _G.ACCEPT)
    acceptButton:SetPoint("BOTTOM", frame, "BOTTOM", -60, 20)
    acceptButton:SetScript("OnClick", function()
        ReloadUI()
        PlaySound("igMainMenuClose")
    end)

    local cancelButton = createButton(frame, "AwesomeCVarCancelButton", _G.CANCEL)
    cancelButton:SetPoint("BOTTOM", frame, "BOTTOM", 60, 20)
    cancelButton:SetScript("OnClick", function()
        self.reloadIsPending = false -- User chose not to reload now
        frame:Hide()
        PlaySound("igMainMenuOptionFaerTab")
    end)

    frame:SetScript("OnShow", function() PlaySound("igMainMenuOpen") end)
end

function ACVar:CreateDefaultConfirmationPopup()
    local frame = createPopupFrame("AwesomeCVarDefaultConfirmationPopup", L.RESET_POPUP_TITLE, L.RESET_POPUP_TEXT)
    self.DefaultConfirmationPopup = frame

    local okayButton = createButton(frame, "AwesomeCVarOkayButton", _G.OKAY)
    okayButton:SetPoint("BOTTOM", frame, "BOTTOM", -60, 20)
    okayButton:SetScript("OnClick", function()
        for _, cvarList in pairs(CVARS) do
            for _, cvarDef in ipairs(cvarList) do
                self:SetCVarValue(cvarDef.name, cvarDef.default, cvarDef)
                self:PrintCVarChange(cvarDef.name, cvarDef.default)
            end
        end
        self:ResetFramePosition()
        self:UpdateAllUI()
        frame:Hide()
        PlaySound("igMainMenuClose")
    end)

    local cancelButton = createButton(frame, "AwesomeCVarCancelButton", _G.CANCEL)
    cancelButton:SetPoint("BOTTOM", frame, "BOTTOM", 60, 20)
    cancelButton:SetScript("OnClick", function()
        frame:Hide()
        PlaySound("igMainMenuOptionFaerTab")
    end)

    frame:SetScript("OnShow", function() PlaySound("igMainMenuOpen") end)
end

local gameMenuFrameIsShown = false
function ACVar:AddGameMenuButton()
    local button = CreateFrame("Button", "GameMenuButtonAwesomeCVar", _G.GameMenuFrame, "GameMenuButtonTemplate")
    if button then
        button:SetText(L.ADDON_NAME_SHORT)

        local _, relTo, _, _, offY = _G.GameMenuButtonMacros:GetPoint()
        if relTo ~= button then
            button:ClearAllPoints()
            button:SetPoint('TOPLEFT', relTo, 'BOTTOMLEFT', 0, -1)

            _G.GameMenuButtonMacros:ClearAllPoints()
            _G.GameMenuButtonMacros:SetPoint('TOPLEFT', button, 'BOTTOMLEFT', 0, offY)
        end

        button:SetScript("OnClick", function()
            self:ToggleFrame()
            HideUIPanel(_G.GameMenuFrame)
        end)
    end

    if not gameMenuFrameIsShown then
        _G.GameMenuFrame:SetHeight(_G.GameMenuFrame:GetHeight() + _G.GameMenuButtonMacros:GetHeight() - 4)
        gameMenuFrameIsShown = true
    end
end

function ACVar:CreateMainFrame()
    local frame = CreateFrame("Frame", "AwesomeCVarFrame", UIParent, "UIPanelDialogTemplate")
    self.Frame = frame
    frame:SetWidth(CONSTANTS.FRAME.MAIN_WIDTH)
    frame:SetHeight(CONSTANTS.FRAME.MAIN_HEIGHT)
    frame:SetPoint("CENTER")
    frame:SetMovable(true)
    frame:EnableMouse(true)
    frame:RegisterForDrag("LeftButton")
    frame:SetScript("OnDragStart", frame.StartMoving)
    frame:SetScript("OnDragStop", frame.StopMovingOrSizing)
    frame:Hide()

    tinsert(_G.UISpecialFrames, "AwesomeCVarFrame")

    frame:SetScript("OnHide", function()
        if self.reloadIsPending then
            self.ReloadPopupFrame:Show()
        end
    end)

    -- Title
    local titleFontString = frame:CreateFontString(nil, "OVERLAY", "GameFontNormalMed3")
    titleFontString:SetPoint("TOP", 0, -8)
    titleFontString:SetText(L.MAIN_FRAME_TITLE)

    -- Tabs
    local tabStrip = CreateFrame("Frame", nil, frame)
    tabStrip:SetPoint("TOPLEFT", 20, -30)
    tabStrip:SetSize(frame:GetWidth() - 32, CONSTANTS.FRAME.TAB_HEIGHT)
    local tabs, panels = {}, {}
    local currentPanel, currentTab = nil, nil
    local prevTab = nil

    local function updateTabState(tab, isSelected)
        local fontString = tab:GetFontString()
        if isSelected then
            fontString:SetTextColor(1, 1, 1, 1) -- White
        else
            fontString:SetTextColor(1, 0.8196, 0) -- Yellow
        end
    end

    -- Helper function to manage tab selection state
    local function selectTab(tab)
        if currentTab then
            updateTabState(currentTab, false)
        end
        updateTabState(tab, true)
        currentTab = tab

        if currentPanel then currentPanel:Hide() end
        panels[tab.categoryName]:Show()
        currentPanel = panels[tab.categoryName]

        ACVar:UpdateAllUI()
    end
    self._SelectTab = selectTab

    for categoryName in pairs(CVARS) do
        -- Create a button with a proper tab template
        local tab = CreateFrame("CheckButton", "AwesomeCVarFrame_"..categoryName.."Tab", tabStrip, "OptionsFrameTabButtonTemplate")

        -- Set the text on the tab's font string
        tab:SetText(categoryName)

        tab.categoryName = categoryName

        -- Tab buttons automatically size to their text, but you can set a min width
        tab:SetHeight(CONSTANTS.FRAME.TAB_HEIGHT)
        tab:SetWidth(tab:GetTextWidth() + 30) -- Add some padding

        if prevTab then
            tab:SetPoint("LEFT", prevTab, "RIGHT", -16, 0) -- Overlap tabs slightly
        else
            tab:SetPoint("TOPLEFT", 0, 0)
        end
        prevTab = tab

        local panel = CreateFrame("ScrollFrame", "AwesomeCVarScrollFrame_"..categoryName, frame, "UIPanelScrollFrameTemplate")
        panel:SetPoint("TOPLEFT", 16, -61)
        panel:SetPoint("BOTTOMRIGHT", frame, "BOTTOMRIGHT", -36, 64)
        panel:Hide()

        local subPanel = CreateFrame("Frame", "AwesomeCVarFramePanel_"..categoryName, panel)
        subPanel:SetPoint("TOPLEFT", panel, "TOPLEFT", 0, 6)
        subPanel:SetSize(panel:GetWidth(), panel:GetHeight() + 12)
        subPanel:SetBackdrop({
            bgFile = "Interface/Tooltips/UI-Tooltip-Background",
            edgeFile = "Interface/Tooltips/UI-Tooltip-Border",
            tile = true, tileSize = 12, edgeSize = 12,
            insets = { left = 0, right = 0, top = 0, bottom = 0 }
        })
        subPanel:SetBackdropColor(0.3, 0.3, 0.3, 0.8)
        subPanel:SetBackdropBorderColor(0.5, 0.5, 0.5, 1)
        subPanel:SetFrameLevel(panel:GetFrameLevel())

        local content = CreateFrame("Frame", "AwesomeCVarFrameContent_"..categoryName, subPanel)
        content:SetSize(panel:GetSize())
        panel:SetScrollChild(content)
        panels[categoryName] = panel
        tinsert(tabs, tab)

        tab:SetScript("OnClick", function(self)
            selectTab(self)
            PlaySound("igCharacterInfoTab")
        end)

        -- Populate panel content
        local cvarList = CVARS[categoryName]
        local lastControl, totalContentHeight = nil, 0
        for i, cvarDef in ipairs(cvarList) do
            local control = CreateFrame("Frame", getFrameName(cvarDef.name, "Control"), content)
            control:SetWidth(content:GetWidth() - 45)
            if lastControl then
                control:SetPoint("TOPLEFT", lastControl, "BOTTOMLEFT", 0, -15)
            else
                control:SetPoint("TOPLEFT", 20, -20)
            end

            local text = control:CreateFontString(nil, "ARTWORK", "GameFontNormal")
            text:SetPoint("TOPLEFT", 0, 0)
            text:SetText(cvarDef.label..":")
			text:SetJustifyH("CENTER")
            local controlHeight = text:GetHeight()
            local descText
            if cvarDef.desc then
                descText = control:CreateFontString(nil, "ARTWORK", "GameFontHighlightSmall")
                descText:SetPoint("TOPLEFT", text, "BOTTOMLEFT", 0, -2)
                descText:SetText(cvarDef.desc)
                descText:SetTextColor(unpack(CONSTANTS.COLORS.DESC_TEXT))
				descText:SetJustifyH("CENTER")
                controlHeight = controlHeight + descText:GetHeight() + 2
            end

            local additionalHeight = 0
            if cvarDef.type == "toggle" then
                additionalHeight = createToggleControl(control, cvarDef, text)
                controlHeight = max(controlHeight, additionalHeight)
            elseif cvarDef.type == "slider" then
                additionalHeight = createSliderControl(control, cvarDef, text, descText)
                controlHeight = controlHeight + additionalHeight + 5
            elseif cvarDef.type == "mode" then
                additionalHeight = createModeControl(control, cvarDef, text, descText)
                controlHeight = controlHeight + additionalHeight
            elseif cvarDef.type == "dropdown" then
                additionalHeight = createDropdownControl(control, cvarDef, text, descText)
                controlHeight = controlHeight + additionalHeight + 5
			elseif cvarDef.type ~= "description" then
                text:SetText(text:GetText().." (Unsupported type: "..tostring(cvarDef.type)..")")
            end
            control:SetHeight(controlHeight)
            lastControl = control
            totalContentHeight = totalContentHeight + control:GetHeight() + 15
        end
        content:SetHeight(totalContentHeight + 10)
    end

    self.TabsByName = {}
    for _, tab in ipairs(tabs) do
        self.TabsByName[tab.categoryName] = tab
    end
    self.PanelsByName = panels

    if #tabs > 0 then
        selectTab(tabs[1]) -- Use the new helper function to select the first tab
    end

    -- Bottom Buttons
    local closeButton = createButton(frame, "AwesomeCVarCloseButton", _G.CLOSE)
    closeButton:SetPoint("BOTTOMRIGHT", -16, 16)
    closeButton:SetScript("OnClick", function()
        ACVar:HideFrame()
    end)

    local okayButton = createButton(frame, "AwesomeCVarOkayButton", _G.OKAY)
    okayButton:SetPoint("RIGHT", closeButton, "LEFT", -10, 0)
    okayButton:SetScript("OnClick", function()
        ACVar:HideFrame()
    end)

    local defaultsButton = createButton(frame, "AwesomeCVarDefaultsButton", _G.DEFAULTS)
    defaultsButton:SetPoint("BOTTOMLEFT", 16, 16)
    defaultsButton:SetScript("OnClick", function()
        self.DefaultConfirmationPopup:Show()
    end)
end