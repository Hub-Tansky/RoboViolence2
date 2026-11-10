# Robo Violence 2 game

The language of the game server and the players in it.

## Language

### Players

**Player**:
Anyone in a player slot (0-31) of a game server, human or bot.
_Avoid_: Client, connection

**Human player**:
A player behind a network connection: a person running the game client.
_Avoid_: Real player, user

**Bot**:
A player the server runs itself, with no connection; its name ends in ` [bot]`.
_Avoid_: AI, NPC, fake player

**Target population**:
How many players on blue and red the server aims for (`sv_bots`); bots fill what humans on a team leave free.
_Avoid_: Bot count, minimum players

**Bot roster**:
The list of bot names (`content/bots/names.txt`); a new bot takes a random name no bot has. A bot keeps its roster index, which picks its main weapon and skin.
_Avoid_: Bot list

### Bot behaviour

**Tactical state**:
What a bot is doing now: wander, attack, attack objective, capture, escort, recover or retreat.
_Avoid_: Mode, behaviour

**CTF role**:
A bot's job in its team's capture-the-flag plan: attacker, defender, escorter, recoverer, interceptor or carrier.
_Avoid_: Class, position

**Brain**:
A bot's memory between frames: state, role, goal, enemy memory, aim and stuck recovery.
_Avoid_: AI, controller
