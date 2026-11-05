
-- PN Minigame Arcade: recognition only, no score or reward. Server: db/import/arcade_achievement_db.yml
-- Project contributions: (C) 2026 PN Development Team, GPL-3.0-or-later.
local function pn_arcade(id, title, text)
	achievement_tbl[id] = {
		UI_Type = 0,
		group = "GOAL_ACHIEVE",
		major = 6,
		minor = 0,
		title = title,
		content = { summary = text, details = text },
		resource = { [1] = { text = text } },
		reward = {},
		score = 0
	}
end
pn_arcade(300001, "Arcade First Coin", "Record a score in the Minigame Arcade (@arcade)")
pn_arcade(300002, "Arcade All-Rounder", "Stamp all four arcade games in one week")
pn_arcade(300003, "Arcade Regular", "Earn 5 arcade stamps")
pn_arcade(300004, "Arcade Veteran", "Earn 20 arcade stamps")
pn_arcade(300005, "Arcade Legend", "Earn 50 arcade stamps")
pn_arcade(300011, "Echo Steps Bronze", "Record a score of 3 in Echo Steps")
pn_arcade(300012, "Echo Steps Silver", "Record a score of 5 in Echo Steps")
pn_arcade(300013, "Echo Steps Gold", "Record a perfect 8 in Echo Steps")
pn_arcade(300021, "Dice Pads Bronze", "Record a score of 4 in Dice Pads")
pn_arcade(300022, "Dice Pads Silver", "Record a score of 8 in Dice Pads")
pn_arcade(300023, "Dice Pads Gold", "Record a perfect 12 in Dice Pads")
pn_arcade(300031, "Floor is Lava Bronze", "Record a score of 5 in Floor is Lava")
pn_arcade(300032, "Floor is Lava Silver", "Record a score of 10 in Floor is Lava")
pn_arcade(300033, "Floor is Lava Gold", "Record a perfect 15 in Floor is Lava")
pn_arcade(300041, "Poring Search Bronze", "Record a score of 5 in Poring Search")
pn_arcade(300042, "Poring Search Silver", "Record a score of 10 in Poring Search")
pn_arcade(300043, "Poring Search Gold", "Record a perfect 15 in Poring Search")

