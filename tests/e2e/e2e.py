#!/usr/bin/env python3
"""Phase T4: end-to-end scenarios: bv2dedicated with the admin API, plus bv2 autotest clients under Xvfb.

Usage: e2e.py <runtime dir> <scratch dir> <scenario>
Scenarios: join_leave, password, chat, kick, ban, master_list, browser_tab_list, ctf_capture, ghost, user_map, weapon_choice,
grenade, rocket, flamethrower, photon_rifle, nuke. Each has one observable: the admin API's
`GET /status` player count or score, the server's stdout, or a client's autotest result (ADR 0025). Linux only (Xvfb).
"""
import os
import re
import shutil
import socket
import subprocess
import sys
import threading
import time
import urllib.request

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
from autotest_client import client_command, start_xvfb  # noqa: E402

ADMIN_PASS = "e2e-admin-pass"
ADMIN_PORT = 18080
JOIN_PASSWORD = "e2e-secret"
API_INTERVAL = 1.2  # seconds between admin API probes: its budget refills one request per second (AdminApi.cpp)


class Run:
    def __init__(self, runtime, scratch, name):
        self.runtime = os.path.abspath(runtime)
        self.dir = os.path.join(scratch, name)
        shutil.rmtree(self.dir, ignore_errors=True)
        os.makedirs(self.dir)
        self.procs = []
        self.server_out = []
        self.logs = []
        xvfb, self.display = start_xvfb()
        self.procs.append(xvfb)

    def start(self, cmd, **kw):
        p = subprocess.Popen(cmd, **kw)
        self.procs.append(p)
        return p

    def stop(self):
        for p in reversed(self.procs):
            if p.poll() is None:
                p.terminate()
                try:
                    p.wait(10)
                except subprocess.TimeoutExpired:
                    p.kill()
        for log in self.logs:
            log.close()

    def master(self):
        home = os.path.join(self.dir, "master")
        os.makedirs(home)
        for db in ("master.db", "web.db"):
            shutil.copy(os.path.join(self.runtime, db), home)
        self.start([os.path.join(self.runtime, "bv2master")], cwd=home, stdout=subprocess.DEVNULL, stderr=subprocess.STDOUT)
        wait(lambda: port_open(10207), 10, "master listening on 10207")

    def server(self, launch=b"execute CTF\n", **env):
        env = {**os.environ, "BV2_MASTER_SERVERS": "none", "BV2_PREF_DIR": os.path.join(self.dir, "server"),  # ADR 0028
               "BV2_ADMIN_PASS": ADMIN_PASS, "BV2_ADMIN_API_PORT": str(ADMIN_PORT), **env}
        p = self.start([os.path.join(self.runtime, "bv2dedicated")], cwd=self.runtime, env=env, stdin=subprocess.PIPE,
                       stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        threading.Thread(target=self.collect, args=(p.stdout, self.server_out), daemon=True).start()
        p.stdin.write(launch)
        p.stdin.flush()
        wait(lambda: any(s in self.server_text() for s in ("Server Created on port 3333", "map not found")), 30,
             "server created")
        check("map not found" not in self.server_text(), "server found its map")
        wait(lambda: self.players() is not None, 10, "admin API answering", API_INTERVAL)  # bots count as players

    @staticmethod
    def collect(stream, lines):
        for line in stream:
            lines.append(line.decode(errors="replace"))

    def server_text(self):
        return "".join(self.server_out)

    def admin(self, method, path, body=None):
        req = urllib.request.Request(f"http://127.0.0.1:{ADMIN_PORT}{path}", data=body, method=method,
                                     headers={"Authorization": "Bearer " + ADMIN_PASS})
        with urllib.request.urlopen(req, timeout=5) as r:
            return r.read().decode()

    def status(self):
        """GET /status as {first word: the other words}, or {} while the API doesn't answer"""
        try:
            return {w[0]: w[1:] for w in (line.split() for line in self.admin("GET", "/status").splitlines()) if w}
        except OSError:
            return {}

    def players(self):
        """Player count from GET /status, or None while the API doesn't answer"""
        players = self.status().get("players")
        return int(players[0]) if players else None

    def score(self):
        """(blue, red) from GET /status `score blue N red M`, or None while the API doesn't answer"""
        score = self.status().get("score")
        return (int(score[1]), int(score[3])) if score else None

    def joined(self, name):
        """Waits for the server's join line: /status counts a slot from the TCP connect, before PLAYER_INFO sets its name"""
        wait(lambda: f"server> {name} joined the game" in self.server_text(), 40, f"{name} joined the game")

    def command(self, text):
        return self.admin("POST", "/command", text.encode())

    def client(self, name, steps, timeout=None, **env):
        """Starts a bv2 autotest client named `name`; returns its process (exit 0 = its script passed). `timeout`: the
        script's limit in seconds when it needs more than the default 60 (--timeout, ADR 0025)"""
        pref = os.path.join(self.dir, name)
        os.makedirs(pref)
        script = os.path.join(pref, "script.txt")
        with open(script, "w") as f:
            f.write("\n".join(steps) + "\n")
        cmd, client_env = client_command(os.path.join(self.runtime, "bv2"), pref, script, f'cl_playerName "{name}"\n')
        if timeout:
            cmd += ["--timeout", str(timeout)]
        log = open(os.path.join(pref, "client.log"), "w")
        self.logs.append(log)
        return self.start(cmd, cwd=self.runtime, env=dict(client_env, DISPLAY=self.display, **env), stdout=log,
                          stderr=subprocess.STDOUT)

    def finished(self, client, timeout=90):
        """The client's exit code once its script ended"""
        try:
            return client.wait(timeout)
        except subprocess.TimeoutExpired:
            return None


def port_open(port):
    with socket.socket() as s:
        return s.connect_ex(("127.0.0.1", port)) == 0


def wait(condition, seconds, what, interval=0.5):
    end = time.time() + seconds
    while time.time() < end:
        if condition():
            return
        time.sleep(interval)
    raise AssertionError(f"timed out after {seconds} s waiting for: {what}")


def check(ok, what):
    if not ok:
        raise AssertionError(what)


JOIN = ["expect-scene menu", "connect 127.0.0.1", "expect-scene game"]
DROPPED = ["expect-scene menu", "connect 127.0.0.1", "expect-console Server disconnected"]  # kicked or banned, maybe before the game scene


def join_leave(run):
    run.server()
    client = run.client("e2e-a", JOIN + ["wait 6"])
    wait(lambda: run.players() == 1, 40, "/status players 1 after the join", API_INTERVAL)
    check(run.finished(client) == 0, "client script passed")
    wait(lambda: run.players() == 0, 20, "/status players 0 after the client left", API_INTERVAL)


def password(run):
    run.server(BV2_SV_PASSWORD=JOIN_PASSWORD)
    wrong = run.client("e2e-wrong", ["expect-scene menu", "connect 127.0.0.1 3333 wrong", "expect-console Server disconnected"])
    check(run.finished(wrong) == 0, "wrong password: client sees the server disconnect it")
    wait(lambda: run.players() == 0, 10, "wrong password: the refused player's slot is freed", API_INTERVAL)
    right = run.client("e2e-right", ["expect-scene menu", f"connect 127.0.0.1 3333 {JOIN_PASSWORD}", "expect-scene game", "wait 6"])
    wait(lambda: run.players() == 1, 40, "right password: /status players 1", API_INTERVAL)
    check(run.finished(right) == 0, "right password: client script passed")


def chat(run):
    run.server()
    client = run.client("e2e-a", JOIN + ["wait 2", "sayall e2e-hello", "wait 3"])
    check(run.finished(client) == 0, "client script passed")
    check("e2e-hello" in run.server_text(), "server stdout has the chat message")


def kick(run):
    run.server()
    client = run.client("e2e-a", DROPPED)
    run.joined("e2e-a")
    run.command("kick e2e-a")
    wait(lambda: "kicked by server" in run.server_text(), 10, "server kicked e2e-a")
    wait(lambda: run.players() == 0, 10, "/status players 0 after POST /command kick", API_INTERVAL)
    check(run.finished(client) == 0, "kicked client sees the server disconnect it")


def ban(run):
    run.server()
    client = run.client("e2e-a", DROPPED)
    run.joined("e2e-a")
    run.command("ban e2e-a")
    wait(lambda: "banned by server" in run.server_text(), 10, "server banned e2e-a")
    wait(lambda: run.players() == 0, 10, "/status players 0 after POST /command ban", API_INTERVAL)
    check(run.finished(client) == 0, "banned client sees the server disconnect it")
    again = run.client("e2e-b", DROPPED)
    wait(lambda: "Disconnecting banned client" in run.server_text(), 40, "server refuses the banned address")
    check(run.finished(again) == 0, "refused client sees the server disconnect it")


def ctf_capture(run):
    """The server moves the bot: blue team, spawned on the red flag pod, then on its own (CTF-Placeholder pods: blue
    at 3,8, red at 13,8; tools/gen-placeholder-content.py). Bots can't steer, and the server only checks positions.
    The CTF script plays content/maps, so the placeholder map is set here."""
    run.server(launch=b"set sv_gameType 2\nset sv_port 3333\ndedicate CTF-Placeholder\n")
    client = run.client("e2e-a", JOIN + ["wait 40"])  # stays well past both 10 s waits below
    run.joined("e2e-a")
    slot = re.search(r"^\[(\d+)\] e2e-a ", run.command("playerlist"), re.M)
    check(slot, "playerlist names e2e-a")
    slot = slot.group(1)
    run.command("move 0 e2e-a")
    run.command(f"forceplayerspawn {slot} 13 8")
    wait(lambda: "took the red flag" in run.server_text(), 10, "e2e-a took the red flag")
    run.command(f"forceplayerspawn {slot} 3 8")
    wait(lambda: run.score() == (1, 0), 10, "/status score blue 1 red 0", API_INTERVAL)
    check(run.finished(client) == 0, "client script passed")


def ghost(run):
    """A TCP client that never answers the join handshake (a port scanner) loses its slot after the join timeout"""
    run.server()
    with socket.create_connection(("127.0.0.1", 3333)):
        wait(lambda: run.players() == 1, 10, "/status counts the silent connection", API_INTERVAL)
        wait(lambda: run.players() == 0, 25, "/status players 0 once the join timeout drops it", API_INTERVAL)
    check("no join handshake" in run.server_text(), "server logs why it dropped the client")


def user_map(run):
    """A map in the pref dir's maps/ (where the editor saves) hosts a game: the server used to look only in main/maps.
    The name has a space and mixed case, like the editor maps that first showed the bug"""
    maps = os.path.join(run.dir, "server", "maps")
    os.makedirs(maps)
    shutil.copy(os.path.join(run.runtime, "main", "maps", "CTF-Placeholder.bvm"), os.path.join(maps, "My Map 1.bvm"))
    run.server(launch=b"dedicate My Map 1\n")
    client = run.client("e2e-a", JOIN + ["wait 6"])
    wait(lambda: run.players() == 1, 40, "/status players 1 on the user map", API_INTERVAL)
    check(run.finished(client) == 0, "client script passed")


def master_list(run):
    run.master()
    masters = "127.0.0.1:10207"
    run.server(BV2_MASTER_SERVERS=masters)
    client = run.client("e2e-a", ["expect-scene menu", "wait 3", "refreshgames", "expect-console Games found : 1"],
                        BV2_MASTER_SERVERS=masters)
    check(run.finished(client) == 0, "client lists the server: Games found : 1")


def browser_tab_list(run):
    """Opening the Browse tab lists the games again (ROBO-217): before, only the menu's first opening did, so a server
    hosted later stayed missing until Refresh or a restart"""
    run.master()
    masters = "127.0.0.1:10207"
    client = run.client("e2e-a", ["expect-scene menu", "expect-console No games found", "wait 12", "menutab host",
                                  "menutab browser", "expect-console Games found : 1"], BV2_MASTER_SERVERS=masters)
    run.server(BV2_MASTER_SERVERS=masters)  # registers 5 s after it starts, during the client's wait
    check(run.finished(client) == 0, "the reopened Browse tab lists the server: Games found : 1")


def weapon_choice(run):
    """A client keeps its chosen gun (cl_primaryWeapon) when it joins again. Every other player's spawn used to overwrite
    it on the client, so with bots on the server a human joined with the last spawned bot's gun"""
    spawned = lambda: re.findall(r"Player e2e-a spawned; ID:\d+ WeaponID:(\d+)", run.server_text())
    joins = lambda: run.server_text().count("server> e2e-a joined the game")
    run.server(BV2_SV_BOTS="2")
    # a console `set`: the shipped bv2.example.cfg layer sets cl_primaryWeapon 3 over a --config file. The first visit
    # stays a spectator (never spawns), so only the bot's spawn can touch the choice
    client = run.client("e2e-a", ["expect-scene menu", "set cl_primaryWeapon 6", "connect 127.0.0.1", "expect-scene game",
                                  "wait 12", "disconnect", "expect-scene menu", "wait 2", "connect 127.0.0.1",
                                  "expect-scene game", "wait 20"])
    run.joined("e2e-a")
    bot = re.search(r"^\[(\d+)\] .*\[bot\]", run.command("playerlist"), re.M)
    check(bot, "playerlist names a bot")
    run.command(f"forceplayerspawn {bot.group(1)} 8 8 1")  # every client sees the bot spawn with gun 1
    wait(lambda: "Player disconnected : e2e-a" in run.server_text(), 30, "e2e-a left")
    check(not spawned(), "e2e-a did not spawn as a spectator")
    run.command("set sv_forceRespawn true")  # sent on join: the client spawns without a key press
    wait(lambda: joins() >= 2, 30, "e2e-a joined again")
    run.command("move 0 e2e-a")
    wait(lambda: spawned(), 20, "e2e-a spawned")
    check(spawned()[0] == "6", f"spawn after joining again with the chosen gun 6, got {spawned()[0]}")
    check(run.finished(client) == 0, "client script passed")

# Weapons and effects (ROBO-240): every server message the client gets in a fight must pass its packet check. A dropped
# one is logged once ("Dropped server message <type>", ClientRecv.cpp), as ROBO-198's grenade explosion was
GRENADE, BAZOOKA, PHOTON_RIFLE, FLAME_THROWER, NUCLEAR = 8, 5, 6, 7, 11  # game/src/GameVar.h
FIGHT_LAUNCH = (b"set sv_gameType 1\nset sv_port 3333\nset sv_showKills true\nset sv_forceRespawn true\nset sv_spawnImmunityTime 0\n"
                b"dedicate TDM-Placeholder\n")  # team deathmatch: the bot fights instead of running for a flag


def fight(run, death, setup=b""):
    """e2e-a on blue, facing one red bot on TDM-Placeholder (an open 16 x 16 arena); returns (e2e-a's slot, the bot's slot, client).
    sv_bots 2 with one human on blue leaves one bot, on red (BotRoster.cpp). The client passes once its console shows
    e2e-a killed by the weapon named `death` (`<killer> -----<weapon>-----> e2e-a`, Player.cpp)"""
    run.server(launch=setup + FIGHT_LAUNCH, BV2_SV_BOTS="2")
    # "wait 2" is not a wait for a condition: it keeps the client up for effect messages sent after the kill, for no_dropped_messages
    client = run.client("e2e-a", JOIN + [f"expect-console -----{death}-----> ", "wait 2"], timeout=100)
    run.joined("e2e-a")
    run.command("move 0 e2e-a")
    slots = {}

    def found():
        text = run.command("playerlist")
        human = re.search(r"^\[(\d+)\] e2e-a ", text, re.M)
        bots = re.findall(r"^\[(\d+)\] .*\[bot\]", text, re.M)
        if human and len(bots) == 1:
            slots.update(human=int(human.group(1)), bot=int(bots[0]))  # playerlist pads: [02]
        return bool(slots)
    wait(found, 20, "playerlist: e2e-a and one bot", API_INTERVAL)
    wait(lambda: "Player e2e-a spawned" in run.server_text(), 20, "e2e-a spawned (sv_forceRespawn)")
    return slots["human"], slots["bot"], client


def killed(run, victim, weapon):
    """The server's kill lines (sv_showKills) with this victim and weapon: `Player id:<victim> killed player id:<killer>`"""
    return re.findall(rf"Player id:{victim} killed player id:\d+ with weapon id:{weapon}\b", run.server_text())


def no_dropped_messages(run, client):
    check(run.finished(client) == 0, "client script passed")
    with open(os.path.join(run.dir, "e2e-a", "client.log"), errors="replace") as log:
        dropped = re.findall(r"Dropped server message \d+ \(size \d+\)", log.read())
    check(not dropped, f"client accepted every server message, dropped: {dropped}")


def bot_kills_with(run, weapon, death, gun=None, setup=b""):
    """The bot gets `gun` (default `weapon`) and stands 4 m from e2e-a; it shoots on sight (Bot.cpp). Waits for a kill
    with `weapon`, named `death` on the client"""
    human, bot, client = fight(run, death, setup)
    run.command(f"forceplayerspawn {human} 6 8")
    run.command(f"forceplayerspawn {bot} 10 8 {weapon if gun is None else gun}")
    wait(lambda: killed(run, human, weapon), 45, f"e2e-a killed with weapon {weapon}")
    no_dropped_messages(run, client)


def rocket(run):
    bot_kills_with(run, BAZOOKA, "Bazooka")


def photon_rifle(run):
    bot_kills_with(run, PHOTON_RIFLE, "Photon Rifle")


def flamethrower(run):
    """Held fire shrinks the flame to sv_ftMinRange (1 m) after sv_ftExpirationTimer (Game.cpp shootSV), shorter than the
    bot's 1.5 m standoff (Bot.cpp flankGoal). A bot never lets go of the trigger, so its flame stops reaching (ROBO-241).
    Timer 0 keeps the full range"""
    bot_kills_with(run, FLAME_THROWER, "Flame Thrower", setup=b"set sv_ftExpirationTimer 0\n")


def grenade(run):
    """Bots throw a grenade every fourth shot at 2 to 6.5 m (Bot.cpp), never in a team game with friendly fire on
    (holdsForTeammate). Pro rules with zero SMG damage leave the grenade (and the knife, under 1 m) as the only way the
    bot can hurt; ROBO-198: the client dropped every grenade explosion"""
    bot_kills_with(run, GRENADE, "Grenade", gun=0, setup=b"set sv_serverType 1\nset sv_smgDamage 0\nset sv_friendlyFire false\n")  # gun 0: SMG


def nuke(run):
    """Admin nuke on the bot: an explosion with no owner (playerID -1), which the client dropped at random before
    cf6a98e. The blast counts as the nuked player's (Server::nukePlayer), so it kills e2e-a beside it. Pro rules with
    zero SMG damage and the bot on the SMG: only the nuke can kill e2e-a (friendly fire on: no bot grenades)"""
    human, bot, client = fight(run, "Nuke Bot", b"set sv_serverType 1\nset sv_smgDamage 0\nset sv_friendlyFire true\n")
    run.command(f"forceplayerspawn {bot} 8 8 0")
    run.command(f"forceplayerspawn {human} 6 8")
    run.command(f"nukeid {bot}")
    wait(lambda: killed(run, human, NUCLEAR), 20, "e2e-a killed by the nuke")
    no_dropped_messages(run, client)


SCENARIOS = {f.__name__: f for f in (join_leave, password, chat, kick, ban, master_list, browser_tab_list, ctf_capture, ghost,
                                     user_map, weapon_choice, grenade, rocket, flamethrower, photon_rifle, nuke)}

if __name__ == "__main__":
    runtime, scratch, name = sys.argv[1:4]
    run = Run(runtime, scratch, name)
    try:
        SCENARIOS[name](run)
    except Exception as e:
        print(f"FAIL {name}: {e}")
        print("--- server stdout (tail)\n" + run.server_text()[-3000:])
        for bot in sorted(os.listdir(run.dir)):
            log = os.path.join(run.dir, bot, "client.log")
            if os.path.exists(log):
                print(f"--- {bot} (tail)\n" + open(log, errors="replace").read()[-3000:])
        sys.exit(1)
    finally:
        run.stop()
    print(f"ok: e2e {name}")
