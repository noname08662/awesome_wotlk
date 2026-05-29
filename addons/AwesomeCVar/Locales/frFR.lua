-- File: frFR.lua
-- Language: French
local _, AwesomeCVar = ...

if not AwesomeCVar.L then
    AwesomeCVar.L = {}
end

local L = AwesomeCVar.L

if GetLocale() == "frFR" then
    -- Général
    L.ADDON_NAME = "AwesomeCVar"
    L.ADDON_NAME_SHORT = "Awesome CVar"
    L.MAIN_FRAME_TITLE = "Gestionnaire Awesome CVar"
    L.RESET_TO = "Réinitialiser à %s"

    -- Fenêtres surgissantes (Popups)
    L.RELOAD_POPUP_TITLE = "Rechargement de l'IU requis"
    L.RELOAD_POPUP_TEXT = "Une ou plusieurs modifications nécessitent un rechargement de l'interface (ReloadUI) pour prendre effet."
    L.RESET_POPUP_TITLE = "Confirmer la réinitialisation"
    L.RESET_POPUP_TEXT = "Êtes-vous sûr de vouloir réinitialiser toutes les valeurs par défaut ?"

    -- Messages de chat
    L.MSG_LOADED = "Awesome CVar chargé ! Tapez /awesome pour ouvrir le gestionnaire."
    L.MSG_FRAME_RESET = "La position de la fenêtre a été réinitialisée au centre."
    L.MSG_SET_VALUE = "%s réglé sur %s."
    L.MSG_FRAME_CREATE_ERROR = "Impossible de créer la fenêtre d'AwesomeCVar !"
    L.MSG_UNKNOWN_COMMAND = "Commande inconnue. Tapez /awesome help pour voir les commandes disponibles."
    L.MSG_HELP_HEADER = "Commandes Awesome CVar :"
    L.MSG_HELP_TOGGLE = "/awesome - Afficher/Masquer le gestionnaire"
    L.MSG_HELP_SHOW = "/awesome show - Afficher le gestionnaire"
    L.MSG_HELP_HIDE = "/awesome hide - Masquer le gestionnaire"
    L.MSG_HELP_RESET = "/awesome reset - Centrer la fenêtre"
    L.MSG_HELP_HELP = "/awesome help - Afficher ce message d'aide"

    -- Catégories CVar
    L.CATEGORY_CAMERA = "Caméra"
    L.CATEGORY_NAMEPLATES = "Barres d'info"
    L.CATEGORY_TEXT_TO_SPEECH = "Synthèse vocale"
    L.CATEGORY_INTERACTION = "Interaction"
    L.CATEGORY_OTHER = "Autre"

    -- Étiquettes et descriptions CVar
    L.CVAR_LABEL_INFO = "Notes"
    L.CVAR_LABEL_TTS_VOICE = "Voix de synthèse"
    L.CVAR_LABEL_TTS_VOLUME = "Volume synthèse"
    L.CVAR_LABEL_TTS_SPEED = "Vitesse synthèse"
    L.CVAR_LABEL_CAMERA_FOV = "Champ de vision (FoV)"
    L.CVAR_LABEL_STACKING_MODE = "Mode d'empilement"
    L.CVAR_LABEL_MOUSEOVER = "Mode de survol (Mouseover)"
    L.CVAR_LABEL_OCCLUSION_ALPHA = "Opacité d'occlusion"
	L.CVAR_LABEL_OCCLUSION_MODE = "Mode d'occlusion des barres de info"
    L.CVAR_LABEL_NONTARGET_ALPHA = "Opacité (hors cible)"
    L.CVAR_LABEL_ALPHA_SPEED = "Vitesse de transition d'opacité"
    L.CVAR_LABEL_CLAMP_TOP = "Fixer au bord supérieur"
    L.CVAR_LABEL_NAMEPLATE_DISTANCE = "Distance d'affichage"
    L.CVAR_LABEL_MAX_RAISE_DISTANCE = "Espacement vertical max (empilement)"
    L.CVAR_LABEL_MAX_PULL_DISTANCE = "Espacement horizontal max (empilement)"
    L.CVAR_LABEL_X_SPACE = "Espacement X"
    L.CVAR_LABEL_Y_SPACE = "Espacement Y"
    L.CVAR_LABEL_UPPER_BORDER = "Décalage bord supérieur"
    L.CVAR_LABEL_PLACEMENT = "Décalage de placement"
    L.CVAR_LABEL_SPEED_RAISE = "Vitesse de montée (empilement)"
    L.CVAR_LABEL_SPEED_LOWER = "Vitesse de descente (empilement)"
    L.CVAR_LABEL_SPEED_PULL = "Vitesse d'attraction horizontale"
	L.CVAR_LABEL_INERTIA = "Inertie d'empilement des plaques"
	L.CVAR_LABEL_HYST_DECAY = "Vitesse de séparation des paires"
	L.CVAR_LABEL_HITBOX_ANCHOR = "Ancre de la Hitbox de Plaque"
    L.CVAR_LABEL_HITBOX_HEIGHT_ENEMY = "Hauteur hitbox (Ennemi)"
    L.CVAR_LABEL_HITBOX_WIDTH_ENEMY = "Largeur hitbox (Ennemi)"
    L.CVAR_LABEL_HITBOX_HEIGHT_FRIENDLY = "Hauteur hitbox (Allié)"
    L.CVAR_LABEL_HITBOX_WIDTH_FRIENDLY = "Largeur hitbox (Allié)"
    L.CVAR_LABEL_INTERACTION_MODE = "Mode d'interaction"
    L.CVAR_LABEL_INTERACTION_ANGLE = "Angle d'interaction (deg)"
    L.CVAR_LABEL_STANCE_PATCH = "Correctif Changement de forme/Posture"
    L.CVAR_LABEL_SHOW_PLAYER = "Affichage du modèle du joueur"
    L.CVAR_LABEL_MSDF_MODE = "Mode de rendu des polices (Requiert Redémarrage)"
	L.CVAR_LABEL_OBJ_HIGHLIGHT = "Mise en valeur des objets"
    L.CVAR_LABEL_CAMERA_INDIRECT_VISIBILITY = "Visibilité indirecte de la caméra"
    L.CVAR_LABEL_CAMERA_INDIRECT_ALPHA = "Opacité indirecte de la caméra"
    L.CVAR_LABEL_CAMERA_DISTANCE_MAX = "Distance de caméra max"

    L.DESC_INFO = "Toutes les barres d'info incluent des méthodes additionnelles :\n- SetStackingEnabled(bool)\n- GetStackingEnabled()\n\nExemple d'utilisation :\nif UnitExists('target') then\n C_NamePlate.GetNamePlateForUnit('target'):SetStackingEnabled(false)\nend\n\nNote : Il est trop tôt pour appeler\nces méthodes lors de NAME_PLATE_CREATED,\nutilisez NAME_PLATE_UNIT_ADDED à la place.\n"
	L.DESC_STACKING_MODE = "Le mode 'Intelligent' permet aux plaques de contourner l'empilement s'il y a suffisamment d'espace en dessous, offrant une disposition plus compacte au prix de réarrangements plus fréquents."
    L.DESC_MOUSEOVER = "Les options 'Surélever' augmentent le niveau du cadre de la plaque survolée, la faisant apparaître au-dessus des autres.\nLa cible actuelle reste toujours au niveau le plus élevé."
	L.DESC_INERTIA = "Contrôle le poids physique du mouvement des barres lors de l'empilement.\nDes valeurs élevées augmentent la réactivité ; des valeurs faibles produisent un mouvement plus lourd et amorti."
	L.DESC_HYST_DECAY = "Contrôle la vitesse à laquelle les paires d'empilement se dissolvent lorsque les plaques ne se chevauchent plus.\nDes valeurs élevées accélèrent la séparation ; des valeurs faibles maintiennent les paires liées plus longtemps."
	L.DESC_PLACEMENT = "Un ratio de décalage vertical qui déplace les barres d'info depuis leur point d'ancrage par défaut."
	L.DESC_HITBOX_ANCHOR = "Définit le point d'origine vertical de la zone cliquable de la barre.\nAjustez cette valeur selon la façon dont votre addon ancre ses cadres de barres (ex. : ancrage par le haut ou le bas)."
    L.DESC_ALPHA_BLEND = "Contrôle la vitesse à laquelle l'opacité des barres s'ajuste (1 = Instantané)."
    L.DESC_STANCE_PATCH = "Permet de changer de posture/forme et de lancer un sort en un seul clic via les macros."
    L.DESC_OCCLUSION_ALPHA = "Contrôle l'opacité des barres lorsqu'elles sont masquées par le décor."
	L.DESC_OCCLUSION_MODE = "Contrôle quand les barres d'info sont masquées ou estompées lorsqu'elles sont bloquées par le terrain ou les murs."
    L.DESC_CAMERA_INDIRECT_VISIBILITY = "Permet à la caméra de traverser certains objets au lieu d'être bloquée."
    L.DESC_CAMERA_INDIRECT_ALPHA = "Définit la transparence des objets se trouvant entre la caméra et le personnage."
    L.DESC_CAMERA_DISTANCE_MAX = "Définit la distance maximale de dézoom de la caméra."
    L.DESC_MSDF = "Active le rendu vectoriel des polices, améliorant drastiquement la qualité."
	L.DESC_OBJ_HIGHLIGHT = "Force l'apparition de scintillements brillants sur les ressources (herbes/minerai) et les objets interactifs comme les caisses ou les panneaux de primes."

	-- Options Mode CVar
	L.MODE_DISABLED = "Désactivé"
	L.MODE_ENABLED = "Activé"

    L.MODE_STACKING_DISABLED = "Chevauchement"
    L.MODE_STACKING_ALL = "Empilées (Toutes)"
    L.MODE_STACKING_ENEMY = "Empilées (Ennemis)"
    L.MODE_STACKING_FRIENDLY = "Empilées (Amis)"
    L.MODE_STACKING_SMART_ALL = "Empilage intelligent (Toutes)"
    L.MODE_STACKING_SMART_ENEMY = "Empilage intelligent (Ennemis)"
    L.MODE_STACKING_SMART_FRIENDLY = "Empilage intelligent (Amis)"

    L.MODE_MOUSE_DISABLED = "Par défaut"
    L.MODE_MOUSE_CLICKTHROUGH_ENEMY = "Cliquer à travers (Ennemis)"
    L.MODE_MOUSE_CLICKTHROUGH_ENEMY_RAISE_FRIENDLY = "Clic Ennemi + Priorité Amis"
    L.MODE_MOUSE_CLICKTHROUGH_ENEMY_RAISE_FRIENDLY_COMBAT = "Clic Ennemi + Priorité Amis (Combat)"
    L.MODE_MOUSE_CLICKTHROUGH_FRIENDLY = "Cliquer à travers (Amis)"
    L.MODE_MOUSE_CLICKTHROUGH_FRIENDLY_RAISE_ENEMY = "Clic Ami + Priorité Ennemis"
    L.MODE_MOUSE_CLICKTHROUGH_FRIENDLY_RAISE_ENEMY_COMBAT = "Clic Ami + Priorité Ennemis (Combat)"
    L.MODE_MOUSE_RAISE_OCCLUDED = "Priorité masquées (Toujours)"
    L.MODE_MOUSE_RAISE_OCCLUDED_COMBAT = "Priorité masquées (Combat)"

    L.MODE_CLAMP_ALL = "Fixer tout"
    L.MODE_CLAMP_BOSSES = "Fixer boss uniquement"

	L.MODE_OCCLUSION_ALWAYS = "Toujours masquer"
	L.MODE_OCCLUSION_NOCOMBAT = "Uniquement hors combat"

    L.MODE_HITBOX_TOP = "Haut"
    L.MODE_HITBOX_CENTER = "Centre"
    L.MODE_HITBOX_BOTTOM = "Bas"

    L.MODE_LABEL_PLAYER_RADIUS = "Rayon joueur 20yd"
    L.MODE_LABEL_CONE_ANGLE = "Angle du cône (deg) à moins de 20yd"

	L.MODE_MSDF_ENABLED_UNSAFE = "Activé (polices non sécurisées) — En raison de la méthode de calcul des champs de distance, certaines polices ayant des contours qui s'intersectent (par ex. 'diediedie') peuvent s'afficher incorrectement."

	L.MODE_HIGHLIGHTS_TRACKED = "Suivis uniquement"
end