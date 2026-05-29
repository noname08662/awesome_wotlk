-- File: UI.lua
-- Contains all functions for creating and managing UI elements.

local _, ACVar = ...
local L = ACVar.L or {}
local CONSTANTS = ACVar.CONSTANTS
local CVARS = ACVar.CVARS

local _G = _G
local ipairs = ipairs
local abs = math.abs
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
    local button = CreateFrame("Button", name, parent, template or "UIPanelButtonTemplate")
    button:SetWidth(width or CONSTANTS.FRAME.BUTTON_WIDTH)
    button:SetHeight(height or CONSTANTS.FRAME.BUTTON_HEIGHT)
    button:SetText(text)
	local fontString = button:GetFontString()
    if fontString and fontString:GetWidth() > ((width or CONSTANTS.FRAME.BUTTON_WIDTH) - 16) then
		fontString:ClearAllPoints()
		fontString:SetPoint("LEFT", button, "LEFT", 8, 0)
		fontString:SetPoint("RIGHT", button, "RIGHT", -8, 0)
    end
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

    local titleText = frame:CreateFontString(nil, "OVERLAY", "GameFontNormalLarge")
    titleText:SetPoint("TOP", 0, -16)
    titleText:SetText(title)

    local messageText = frame:CreateFontString(nil, "OVERLAY", "GameFontNormal")
    messageText:SetPoint("TOP", titleText, "BOTTOM", 0, -10)
    messageText:SetWidth(frame:GetWidth() - 40)
    messageText:SetJustifyH("CENTER")
    messageText:SetText(message)

    return frame
end

function ACVar:UpdateResetButtonState(cvarDef, currentValue)
    local resetButton = _G[getFrameName(cvarDef.name, "ResetButton")]
    if resetButton then
        if self.FormatNumber(currentValue) ~= self.FormatNumber(cvarDef.default) then
            resetButton:Enable()
        else
            resetButton:Disable()
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
        self:UpdateResetButtonState(cvarDef, currentValue)
    elseif cvarDef.type == "mode" then
        for i, mode in ipairs(cvarDef.modes) do
            _G[getFrameName(cvarName, "Radio"..i)]:SetChecked(currentValue == mode.value)
        end
    elseif cvarDef.type == "dropdown" then
        local dropdown = _G[getFrameName(cvarName, "Dropdown")]
        UIDropDownMenu_SetSelectedValue(dropdown, currentValue)
        local label = cvarDef.options[currentValue] or tostring(currentValue)
        UIDropDownMenu_SetText(dropdown, label)
        self:UpdateResetButtonState(cvarDef, currentValue)
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
	if self.DefaultConfirmationPopup then return end

    local frame = createPopupFrame("AwesomeCVarDefaultConfirmationPopup", L.RESET_POPUP_TITLE, L.RESET_POPUP_TEXT)
    self.DefaultConfirmationPopup = frame

    local okayButton = createButton(frame, "AwesomeCVar_ConfirmResetButton", _G.OKAY)
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

    local cancelButton = createButton(frame, "AwesomeCVar_CancelResetButton", _G.CANCEL)
    cancelButton:SetPoint("BOTTOM", frame, "BOTTOM", 60, 20)
    cancelButton:SetScript("OnClick", function()
        frame:Hide()
        PlaySound("igMainMenuOptionFaerTab")
    end)

    frame:SetScript("OnShow", function() PlaySound("igMainMenuOpen") end)

	tinsert(UISpecialFrames, "AwesomeCVarDefaultConfirmationPopup")
end

function ACVar:AddGameMenuButton()
    local button = ACVar.GameMenuButton or CreateFrame("Button", "GameMenuButtonAwesomeCVar", _G.GameMenuFrame, "GameMenuButtonTemplate")
    if button then
        button:SetText(L.ADDON_NAME_SHORT)
		button:ClearAllPoints()
		button:SetPoint("TOP", _G.GameMenuButtonContinue, "BOTTOM", 0, -1)
        button:SetScript("OnClick", function()
            self:ToggleFrame()
            HideUIPanel(_G.GameMenuFrame)
        end)
		button:SetScript("OnShow", function()
			if not self.MenuExtended then
				_G.GameMenuFrame:SetHeight(_G.GameMenuFrame:GetHeight() + 24)
				self.MenuExtended = true
			end
		end)
		button:SetScript("OnHide", function()
			if self.MenuExtended then
				_G.GameMenuFrame:SetHeight(_G.GameMenuFrame:GetHeight() - 24)
			end
			self.MenuExtended = false
		end)
		ACVar.GameMenuButton = button
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

	frame.numTabs = 0

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
    local currentPanel = nil
    local prevTab = nil

    local function selectTab(tab)
        if currentPanel then currentPanel:Hide() end
        panels[tab.categoryName]:Show()
        currentPanel = panels[tab.categoryName]
		PanelTemplates_SetTab(frame, tab:GetID())
        ACVar:UpdateAllUI()
    end
    self._SelectTab = selectTab

    for categoryName in pairs(CVARS) do
		frame.numTabs = frame.numTabs + 1

        local tab = CreateFrame("CheckButton", "AwesomeCVarFrameTab"..frame.numTabs, tabStrip, "OptionsFrameTabButtonTemplate")
        tab.categoryName = categoryName
		tab:SetID(frame.numTabs)

        tab:SetText(categoryName)
        tab:SetHeight(CONSTANTS.FRAME.TAB_HEIGHT)
        tab:SetWidth(tab:GetTextWidth() + 30)

        if prevTab then
            tab:SetPoint("LEFT", prevTab, "RIGHT", -16, 0)
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
        local lastControl = nil

        for _, cvarDef in ipairs(cvarList) do
            -- 1. Container frame
            local control = CreateFrame("Frame", getFrameName(cvarDef.name, "Control"), content)
            control:SetWidth(content:GetWidth() - 35)

            if lastControl then
                control:SetPoint("TOPLEFT", lastControl, "BOTTOMLEFT", 0, -15)
            else
                control:SetPoint("TOPLEFT", 10, -15)
            end

            control:SetBackdrop({
                bgFile = "Interface/Tooltips/UI-Tooltip-Background",
                edgeFile = "Interface/Tooltips/UI-Tooltip-Border",
                tile = true, tileSize = 12, edgeSize = 12,
                insets = { left = 3, right = 3, top = 3, bottom = 3 }
            })
            control:SetBackdropColor(0.15, 0.15, 0.15, 0.6)
            control:SetBackdropBorderColor(0.4, 0.4, 0.4, 0.8)

            local paddingLeft = 12
            local paddingRight = -12
            local paddingTop = -10
            local paddingBottom = 10

            -- 2. Label
            local text = control:CreateFontString(nil, "ARTWORK", "GameFontNormalLarge")
            text:SetPoint("TOPLEFT", control, "TOPLEFT", paddingLeft, paddingTop)
            text:SetWidth(control:GetWidth() + (paddingRight - paddingLeft))
            text:SetJustifyH("LEFT")
            text:SetText(cvarDef.label)

            -- 3. Desc
            local descText
            if cvarDef.desc then
                descText = control:CreateFontString(nil, "ARTWORK", "GameFontHighlightSmall")
                descText:SetPoint("TOPLEFT", text, "BOTTOMLEFT", 0, -4)
                descText:SetWidth(control:GetWidth() + (paddingRight - paddingLeft))
                descText:SetJustifyH("LEFT")
                descText:SetTextColor(unpack(CONSTANTS.COLORS.DESC_TEXT))
                descText:SetText(cvarDef.desc)
            end

            -- 4. Widget frame anchored below label/desc
            local widgetFrame = CreateFrame("Frame", getFrameName(cvarDef.name, "Widget"), control)
            widgetFrame:SetPoint("TOPLEFT", descText or text, "BOTTOMLEFT", 0, -8)
            widgetFrame:SetPoint("RIGHT", control, "RIGHT", paddingRight, 0)

            if cvarDef.type == "toggle" then
                widgetFrame:SetHeight(25)
                local checkbox = CreateFrame("CheckButton", getFrameName(cvarDef.name, "Checkbox"), widgetFrame, "UICheckButtonTemplate")
                checkbox:SetPoint("LEFT", widgetFrame, "LEFT", 0, 0)
                checkbox.cvarDef = cvarDef
                checkbox:SetScript("OnClick", function(self)
                    local checked = self:GetChecked()
                    local newVal = checked and self.cvarDef.max or self.cvarDef.min
                    ACVar:SetCVarValue(self.cvarDef.name, newVal, self.cvarDef)
                    ACVar:PrintCVarChange(self.cvarDef.name, newVal)
                    PlaySound(checked and "igMainMenuOptionCheckBoxOn" or "igMainMenuOptionCheckBoxOff")
                end)

            elseif cvarDef.type == "slider" then
                widgetFrame:SetHeight(40)
                local slider = CreateFrame("Slider", getFrameName(cvarDef.name, "Slider"), widgetFrame, "OptionsSliderTemplate")
                _G[slider:GetName().."Low"]:SetText(cvarDef.min)
                _G[slider:GetName().."High"]:SetText(cvarDef.max)
                slider:SetMinMaxValues(cvarDef.min, cvarDef.max)
                slider:SetValueStep(cvarDef.step or 1)
                slider:SetPoint("TOPLEFT", widgetFrame, "TOPLEFT", 0, -8)
                slider:SetPoint("RIGHT", widgetFrame)

                local valueText = widgetFrame:CreateFontString(getFrameName(cvarDef.name, "SliderValue"), "ARTWORK", "GameFontNormal")
                valueText:SetPoint("TOP", slider, "BOTTOM", 0, -2)
                slider.cvarDef = cvarDef
                slider.valueText = valueText

                local resetButton = createButton(widgetFrame, getFrameName(cvarDef.name, "ResetButton"),
					format(L.RESET_TO, cvarDef.default), 100, 20)
                resetButton:SetPoint("TOPRIGHT", control, paddingRight, paddingTop)
                resetButton:Disable()
                resetButton.cvarDef = cvarDef
                resetButton.slider = slider
                resetButton:SetScript("OnClick", function(self)
                    ACVar:SetCVarValue(self.cvarDef.name, self.cvarDef.default, self.cvarDef)
                    ACVar:PrintCVarChange(self.cvarDef.name, self.cvarDef.default)
                    self.slider:SetValue(self.cvarDef.default)
                    self:Disable()
                    PlaySound("igMainMenuOptionFaerTab")
                end)
                slider:SetScript("OnValueChanged", function(self, val)
                    val = ACVar.FormatNumber(val)
                    self.valueText:SetText(tostring(val))
                    ACVar:SetCVarValue(self.cvarDef.name, val, self.cvarDef)
                    self.pendingValue = val
                    ACVar:UpdateResetButtonState(self.cvarDef, val)
                end)
                slider:SetScript("OnMouseUp", function(self)
                    if self.pendingValue then
                        ACVar:PrintCVarChange(self.cvarDef.name, self.pendingValue)
                    end
                end)

            elseif cvarDef.type == "dropdown" then
                widgetFrame:SetHeight(40)
                local dropdown = CreateFrame("Frame", getFrameName(cvarDef.name, "Dropdown"), widgetFrame, "UIDropDownMenuTemplate")
                dropdown:SetPoint("TOPLEFT", widgetFrame, -16, 0)
                dropdown:SetPoint("TOPRIGHT", widgetFrame)
                dropdown.cvarDef = cvarDef
				dropdown.relativeTo = dropdown
				dropdown.point = "TOPLEFT"
				dropdown.relativePoint = "BOTTOMLEFT"
				dropdown.xOffset = 18
				dropdown.yOffset = 2

				UIDropDownMenu_SetWidth(dropdown, control:GetWidth() - 40)

                local function getCurrentValue()
                    return (ACVar and ACVar.GetCVarValue and ACVar:GetCVarValue(cvarDef.name)) or cvarDef.default
                end
                local function setClosedLabel(val)
                    UIDropDownMenu_SetText(dropdown, cvarDef.options[val] or tostring(val))
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
                                ACVar:UpdateResetButtonState(cvarDef, i)
                                ACVar:PrintCVarChange(cvarDef.name, i)
                                PlaySound("igMainMenuOptionCheckBoxOn")
                                CloseDropDownMenus()
                            end
                            UIDropDownMenu_AddButton(info, level)
                        end
                    end
					_G["DropDownList"..(level or 1)].maxWidth = _G[dropdown:GetName().."Middle"]:GetWidth() + paddingRight
                end)
                setClosedLabel(getCurrentValue())

                local resetButton = createButton(widgetFrame, getFrameName(cvarDef.name, "ResetButton"),
                    string.format(L.RESET_TO, tostring(cvarDef.options[cvarDef.default] or cvarDef.default)), 100, 20)
                resetButton:SetPoint("TOPRIGHT", control, paddingRight, paddingTop)
                resetButton:Disable()
                resetButton.cvarDef = cvarDef
                resetButton.dropdown = dropdown
                resetButton:SetScript("OnClick", function(self)
                    ACVar:SetCVarValue(self.cvarDef.name, self.cvarDef.default, self.cvarDef)
                    ACVar:PrintCVarChange(self.cvarDef.name, self.cvarDef.default)
                    setClosedLabel(self.cvarDef.default)
                    PlaySound("igMainMenuOptionCheckBoxOn")
                    self:Disable()
                end)
                dropdown.resetButton = resetButton
                ACVar:UpdateResetButtonState(cvarDef, getCurrentValue())

            elseif cvarDef.type == "mode" then
                widgetFrame:SetHeight(#cvarDef.modes * 20)
                local prevRadio
                for j, mode in ipairs(cvarDef.modes) do
                    local radio = CreateFrame("CheckButton", getFrameName(cvarDef.name, "Radio"..j), widgetFrame, "UIRadioButtonTemplate")
                    if prevRadio then
                        radio:SetPoint("TOPLEFT", prevRadio, "BOTTOMLEFT", 0, -4)
                    else
                        radio:SetPoint("TOPLEFT", widgetFrame, "TOPLEFT", 0, 0)
                    end
                    local label = radio:CreateFontString(nil, "OVERLAY", "GameFontNormal")
                    label:SetPoint("LEFT", radio, "RIGHT", 5, 0)
                    label:SetText(mode.label)

                    radio.cvarDef = cvarDef
                    radio.modeValue = mode.value
                    radio.modeLabel = mode.label
                    radio:SetScript("OnClick", function(self)
                        for k = 1, #self.cvarDef.modes do
                            _G[getFrameName(self.cvarDef.name, "Radio"..k)]:SetChecked(false)
                        end
                        self:SetChecked(true)
                        ACVar:SetCVarValue(self.cvarDef.name, self.modeValue, self.cvarDef)
                        ACVar:PrintCVarChange(self.cvarDef.name, self.modeValue, self.modeLabel)
                        PlaySound("igMainMenuOptionCheckBoxOn")
                    end)
                    prevRadio = radio
                end

            elseif cvarDef.type ~= "description" then
                widgetFrame:SetHeight(20)
                text:SetText(text:GetText().." (Unsupported type: "..tostring(cvarDef.type)..")")
            else
                widgetFrame:SetHeight(0)
            end

            control:SetScript("OnUpdate", function(self)
                self:SetScript("OnUpdate", nil)
                local descH = descText and (descText:GetStringHeight() + 4 + 8) or 0
                local widgetH = (cvarDef.type ~= "description") and (widgetFrame:GetHeight() + 8) or 0
                self:SetHeight(abs(paddingTop) + text:GetStringHeight() + descH + widgetH + paddingBottom)
            end)

            lastControl = control
        end

		-- total scroll content height
		content:SetScript("OnUpdate", function(self)
			self:SetScript("OnUpdate", nil)
			if lastControl and lastControl:GetBottom() then
				self:SetHeight(self:GetTop() - lastControl:GetBottom() + 20)
			end
		end)
    end

    self.TabsByName = {}
    for _, tab in ipairs(tabs) do
        self.TabsByName[tab.categoryName] = tab
    end
    self.PanelsByName = panels

    if #tabs > 0 then
        PanelTemplates_SetNumTabs(frame, frame.numTabs)
        selectTab(tabs[1])
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