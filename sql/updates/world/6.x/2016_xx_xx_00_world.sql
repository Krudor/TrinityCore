UPDATE `trinity_string` SET `content_default`='WorldSafeLoc ID: %u not found!' WHERE `entry`=288;

UPDATE `command` SET `name`='go wsloc', `help`='Syntax: .go wsloc #wslocId Teleport to WorldSafeLoc with the wslocId specified (WorldSafeLoc.dbc).' WHERE `permission`=379;
