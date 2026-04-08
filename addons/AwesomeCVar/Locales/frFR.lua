-- File: frFR.lua
-- Language: French
local addonName, AwesomeCVar = ...

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
    L.CVAR_LABEL_HITBOX_HEIGHT_ENEMY = "Hauteur hitbox (Ennemi)"
    L.CVAR_LABEL_HITBOX_WIDTH_ENEMY = "Largeur hitbox (Ennemi)"
    L.CVAR_LABEL_HITBOX_HEIGHT_FRIENDLY = "Hauteur hitbox (Allié)"
    L.CVAR_LABEL_HITBOX_WIDTH_FRIENDLY = "Largeur hitbox (Allié)"
    L.CVAR_LABEL_INTERACTION_MODE = "Mode d'interaction"
    L.CVAR_LABEL_INTERACTION_ANGLE = "Angle d'interaction (deg)"
    L.CVAR_STANCE_PATCH = "Correctif Changement de forme/Posture"
    L.CVAR_SHOW_PLAYER = "Affichage du modèle du joueur"
    L.CVAR_MSDF_MODE = "Mode de rendu des polices (Requiert Redémarrage)"
    L.CVAR_LABEL_CAMERA_INDIRECT_VISIBILITY = "Visibilité indirecte de la caméra"
    L.CVAR_LABEL_CAMERA_INDIRECT_ALPHA = "Opacité indirecte de la caméra"
    L.CVAR_LABEL_CAMERA_DISTANCE_MAX = "Distance de caméra max"

    L.DESC_INFO = "\nToutes les barres d'info incluent des méthodes additionnelles :\n- SetStackingEnabled(bool)\n- GetStackingEnabled()\n\nExemple d'utilisation :\nif UnitExists('target') then\n C_NamePlate.GetNamePlateForUnit('target'):SetStackingEnabled(false)\nend\n\nNote : Il est trop tôt pour appeler\nces méthodes lors de NAME_PLATE_CREATED,\nutilisez NAME_PLATE_UNIT_ADDED à la place.\n"
    L.DESC_STACKING_MODE = "0 = Désactivé ; 1 = Tout activer ; 2 = Ennemis uniquement ; 3 = Alliés uniquement."
    L.DESC_MOUSEOVER = "0 = Désactivé ;\n1 = Clic à travers les ennemis ;\n2 = Clic à travers les ennemis, toujours mettre en avant les barres alliées masquées ;\n3 = Clic à travers les ennemis, mettre en avant les alliés masqués en combat ;\n4 = Clic à travers les alliés ;\n5 = Clic à travers les alliés, toujours mettre en avant les ennemis masqués ;\n6 = Clic à travers les alliés, mettre en avant les ennemis masqués en combat ;\n7 = Toujours mettre en avant le niveau de la barre masquée sous la souris ;\n8 = Mettre en avant la barre masquée sous la souris en combat."
    L.DESC_ALPHA_BLEND = "Contrôle la vitesse à laquelle l'opacité des barres s'ajuste (1 = Instantané)."
    L.DESC_CLAMP_TOP = "0 = Désactivé ; 1 = Tout fixer ; 2 = Boss uniquement."
    L.DESC_STANCE_PATCH = "Permet de changer de posture/forme et de lancer un sort en un seul clic via les macros."
    L.DESC_OCCLUSION_ALPHA = "Contrôle l'opacité des barres lorsqu'elles sont masquées par le décor."
    L.DESC_CAMERA_INDIRECT_VISIBILITY = "Permet à la caméra de traverser certains objets au lieu d'être bloquée."
    L.DESC_CAMERA_INDIRECT_ALPHA = "Définit la transparence des objets se trouvant entre la caméra et le personnage."
    L.DESC_CAMERA_DISTANCE_MAX = "Définit la distance maximale de dézoom de la caméra."
    L.DESC_MSDF = "Active le rendu vectoriel des polices, améliorant drastiquement la qualité.\n0 = Désactivé ; 1 = Activé ;\n2 = Activé (polices instables) — à cause du calcul des champs de distance,\ncertaines polices aux contours croisés peuvent bugger."

    -- Options Mode CVar
    L.MODE_LABEL_PLAYER_RADIUS = "Rayon joueur 20yd"
    L.MODE_LABEL_CONE_ANGLE = "Angle du cône (deg) à moins de 20yd"
end