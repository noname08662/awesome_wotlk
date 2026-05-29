local _, ACVar = ...
if not ACVar.Skins then
	ACVar.Skins = {}
end

ACVar.Skins.ElvUI = function()
	if not ElvUI then return end

	local E, L, V, P, G = unpack(ElvUI)
	local S = E:GetModule("Skins")
	local _G = _G
	local match = string.match

	local function SkinCVarControl(cvarDef)
		local name = cvarDef.name
		local PREFIX = "AwesomeCVar_"

		local control = _G[PREFIX..name.."Control"]
		if control and not control.__elvSkinned then
			control:StripTextures()
			control:SetTemplate("Transparent")
			control.__elvSkinned = true
		end

		if cvarDef.type == "slider" then
			local slider = _G[PREFIX..name.."Slider"]
			if slider and not slider.__elvSkinned then
				S:HandleSliderFrame(slider)
				slider.__elvSkinned = true
			end

			local resetBtn = _G[PREFIX..name.."ResetButton"]
			if resetBtn and not resetBtn.__elvSkinned then
				S:HandleButton(resetBtn)
				resetBtn.__elvSkinned = true
			end

		elseif cvarDef.type == "toggle" then
			local checkbox = _G[PREFIX..name.."Checkbox"]
			if checkbox and not checkbox.__elvSkinned then
				S:HandleCheckBox(checkbox, true)
				checkbox.__elvSkinned = true
			end

		elseif cvarDef.type == "dropdown" then
			local dropdown = _G[PREFIX..name.."Dropdown"]
			if dropdown and not dropdown.__elvSkinned then
				S:HandleDropDownBox(dropdown)
				dropdown:SetTemplate("Transparent")
				dropdown.backdrop:Hide()
				local btn = _G[dropdown:GetName().."Button"]
				btn:ClearAllPoints()
				btn:Point("RIGHT", dropdown, "RIGHT", -8, 0)
				btn:Size(20, 20)
				local _, p = dropdown:GetPoint()
				dropdown:ClearAllPoints()
				dropdown:SetPoint("TOPLEFT", p)
				dropdown:SetPoint("TOPRIGHT", p)
				dropdown.xOffset = 1
				dropdown.__elvSkinned = true
			end

			local resetBtn = _G[PREFIX..name.."ResetButton"]
			if resetBtn and not resetBtn.__elvSkinned then
				S:HandleButton(resetBtn)
				resetBtn.__elvSkinned = true
			end

		elseif cvarDef.type == "mode" then
			if cvarDef.modes then
				for i = 1, #cvarDef.modes do
					local radio = _G[PREFIX..name.."Radio"..i]
					if radio and not radio.__elvSkinned then
						S:HandleCheckBox(radio, true)
						radio.__elvSkinned = true
					end
				end
			end
		end
	end

	local function SkinMainFrame()
		if not ACVar.Frame or ACVar.Frame.__elvSkinned then return end

		ACVar.Frame:StripTextures()
		ACVar.Frame:SetTemplate("Transparent")

		local titleClose = _G["AwesomeCVarFrameClose"]
		if titleClose then
			S:HandleCloseButton(titleClose)
		end

		for _, tab in pairs(ACVar.TabsByName) do
			S:HandleTab(tab)
			local text = _G[tab:GetName().."Text"]
			text:ClearAllPoints()
			text:Point("CENTER", 0, 1)
		end

		for _, child in ipairs({ACVar.Frame:GetChildren()}) do
			local objType = child:GetObjectType()
			if objType == "ScrollFrame" then
				local cname = child:GetName() or ""
				child:StripTextures()
				local scrollBar = _G[cname.."ScrollBar"]
				if scrollBar then
					S:HandleScrollBar(scrollBar)
				end
				local category = match(cname, "^AwesomeCVarScrollFrame_(.+)$")
				if category then
					local subPanel = _G["AwesomeCVarFramePanel_"..category]
					if subPanel then
						subPanel:StripTextures()
						subPanel:SetTemplate("Transparent")
					end
				end
			end
		end

		local closeBtn = _G["AwesomeCVarCloseButton"]
		local okayBtn = _G["AwesomeCVarOkayButton"]
		local defaultsBtn = _G["AwesomeCVarDefaultsButton"]
		if closeBtn then S:HandleButton(closeBtn) end
		if okayBtn then S:HandleButton(okayBtn) end
		if defaultsBtn then S:HandleButton(defaultsBtn) end

		local reloadPopup = _G["AwesomeCVarReloadPopup"]
		if reloadPopup then
			reloadPopup:StripTextures()
			reloadPopup:SetTemplate("Transparent")
			local acceptBtn = _G["AwesomeCVarAcceptButton"]
			if acceptBtn then S:HandleButton(acceptBtn) end
		end

		ACVar.Frame.__elvSkinned = true
	end

	hooksecurefunc(ACVar, "UpdateUIForCVar", function(self, cvarDef)
		SkinCVarControl(cvarDef)
	end)

	hooksecurefunc(ACVar, "AddGameMenuButton", function()
		local btn = _G["GameMenuButtonAwesomeCVar"]
		if btn then S:HandleButton(btn) end
	end)

	hooksecurefunc(ACVar, "CreateDefaultConfirmationPopup", function()
		if ACVar.DefaultConfirmationPopup and not ACVar.DefaultConfirmationPopup.__elvSkinned then
			ACVar.DefaultConfirmationPopup:StripTextures()
			ACVar.DefaultConfirmationPopup:SetTemplate("Transparent")
			local okayPopupBtn = _G["AwesomeCVar_ConfirmResetButton"]
			local cancelBtn = _G["AwesomeCVar_CancelResetButton"]
			if cancelBtn then S:HandleButton(cancelBtn) end
			if okayPopupBtn then S:HandleButton(okayPopupBtn) end
			ACVar.DefaultConfirmationPopup.__elvSkinned = true
		end
	end)

	hooksecurefunc(ACVar, "CreateMainFrame", SkinMainFrame)

	if ACVar.Frame then
		SkinMainFrame()
	end

	if ACVar.GameMenuButton then
		S:HandleButton(ACVar.GameMenuButton)
	end

	if ACVar.DefaultConfirmationPopup then
		ACVar.DefaultConfirmationPopup:StripTextures()
		ACVar.DefaultConfirmationPopup:SetTemplate("Transparent")
		local okayPopupBtn = _G["AwesomeCVar_ConfirmResetButton"]
		local cancelBtn = _G["AwesomeCVar_CancelResetButton"]
		if cancelBtn then S:HandleButton(cancelBtn) end
		if okayPopupBtn then S:HandleButton(okayPopupBtn) end
		ACVar.DefaultConfirmationPopup.__elvSkinned = true
	end
end