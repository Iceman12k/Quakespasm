Mission: 
Qracks is the defaco engine for compeittive DM & CTF Net Quake. This project aims to add features from Qrack and more modern engines to the Quakespawm Spiked Code Base

Credits:
JPG, r00k, Spike for QSS, Ozkan Sezer & Eric Wasylishen (Quakespasm), John Fitzgibbons (FitzQuake), Baker (MarkV), MH, Joe, & many more


[ALL QSS-M Commands & Variables (Google Sheets)](https://docs.google.com/spreadsheets/d/1ubOuromaXpZonfL-eJ-KA7q-xSRiBBuSvxahzF-uFOY/edit?usp=sharing)


## new cvars

### `cl_autodemo`

Automatically records demos.   

Values: 0 - no auto recording, 1 - record demos everytime a new map is spawned, even if not connected to a server, 2 - only record when a CRMOD / CRCTF official match starts.
(Default: 0)

### `cl_damagehue`

Shifts viewmodel orange tint on damage taken to let the player know they have recieved damage. 

Values: 0 - no shift, 1 - shift on. 
(Default: 1)

### `cl_demospeed`

Increase the playback speed of a playing demo.  

Values: 1 - normal speed, 20 - max speed. 
(Default: 1)

### `cl_say`

All non-commands typed into console translate to talk (say)

Values: 0 - normal, 1 - toggle on. 
(Default: 0)

### `cl_truelightning`

Toggle lightning gun beam appearing to be straight ahead at all times.

Values: 1 - perfectly straight, 0 - disabled. Example: cl_truelightning .5 will display adjust more straight, but not entirely. 
(Default: 0)

### `gl_lightning_alpha`

Adjust transparency of lighting bolt to player does not get blinded in fights.

Values: 1 - full alpha, default, 0 - full transparnecy. Example: gl_lightning_alpha .5 will display half transparency. 
(Default: 1)

### `scr_match_hud`

Displays CRMOD / CRCTF match timer, scores, flagstatus in upper right hand corner of screen. This is the equivalent to Qrack's scr_printstats. In practice mode no dispaly, FFA clock only. 

Values: 0 - no display, 1 - display on. (Default: 1)

### `scr_ping`

Displays ping and packetloss (dropped datagrams) in lower left corner when connected to a server. PL will only show when a drop packet(s) occurs. Updated every 5 seconds. 

Values: 1 - show, 0 - no show. 
(Default: 1)

### `scr_showspeed`

Displays drawing of your current speed near sbar (MarkV)

Values: 1 - show, 0 - no show. 
(Default: 0)

## changed cvars

### `crosshair`

Sets types of crosshairs. 

Values: 0 - no crosshair, 1 - sqr, 2 - open +, 3 - basic +, 4 - bold +, 5 - outline sqr, 6 - outline bold +
(Default: 1)

### `scr_clock`

Displays realtime clock in lower right corner.

Values: 1 - leveltime, 2 - for 12 hr realtime clock, 3 - for 24 hr realtime clock.
(Default: 0)

### `scr_crosshaircolor`

Allows you to change the crosshair color from white to yellow, red, black, or blue. 

Values: 0 - white, 1 - yellow, 2 - red, 3 - blue, 4 - black
(Default: 0)

## new commands

### `identify`

Identifies a player by their IP Address. Type status and then identify their number.  

### `lastid`

Displays the last ghost code assigned by CRMOD / CRCTF, even if disconnected. 

## modified commands

### `record`

Typing record with no arguments will record a demo to id1/demos with the map name and date as the file name

## other added features

* pq location file support (r00k)
* pq timer support (r00k)
* pq smooth chasecam
* pq confilter+ (no pickup messages printed at all like ezquake)
* pq iplog
* pq doubleeyes default (mh version)
* name appended with AFK when application loses focus and team notify during match via message
* name appended with ... when typing in console with ezsay or in messagemode
* ctf/dm auto config loading -- searches for ctf.cfg or dm.cfg and will load either if connected to server type
* end of match auto config loading -- searches for end.cfg and will load at match end (say gg, stats, color, ready status, etc)
* deadbodyfilter default
* automaticaly report to team next quad time when expire sound begins, auto report powerup grabs in CRMOD
* show all connected clients sorted by score in lower left in observer/eyecam mode
* [custom particle set](https://github.com/timbergeron/qss-particles)

## other changes features/behaviors

* lightflicker from flags and quad glows OFF
* padding around scr_fps and scr_clock
* removed muzzleflash, rocketlight, rocket explosion light
* scoreboard pings white text; font scale to console font; qrack +showscores frame. Remove Ready during match
* default sbar in the middle
* center print messages adjust to size of console scale
* eliminated colored lit lighting on weapon/player models that obscured view
* alpha to weapon model for eyes (mhquake - death match only)
* screenshot/demos folders
* upped minimum light value models
* change proquake login message from pq 3.40 to 3.50
* added LOC %z to only report RL and LG (bruce); other weapons not important; changed no weapons-> nothing
* parsed match ends in for every minute, useless notifications; keep conole clear for usefull information
* removed all background ambient sounds for multiplayer
* +showscores enabled in demo playback
* moved observer center print to sbar area so as to not block view
* r_shadows based on ambient light (r00k)
* fixed external vis files loading when connected to servers
* read RGB values from worldspawn to customize the colour of cshift contents water, slime, lava (infiniti)
* removed useless CTF centerprint flag messages
* make CAPSLOCK bindable and disable capslock in messagemode
* sound back to DOSquake levels (seadondo) from proquake
* remove IPX from menu
* sbar face damagehue
* screenshots play a sound

## TODO / MAYBE

* enemyskin, teamskin, baseskin system from ezquake/FTE
* reverse clock timer option like ezquake
* fake lag
* map config
* per map models (different flags for different maps)
* web download
* disable windows key
* notready report like qw
* ezsay command exclusion list
```
timbergeron@gmail.com | discord.quakeone.com (woods#3451)
```
