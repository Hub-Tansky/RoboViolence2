#!/usr/bin/env python3
"""Phase T4: end-to-end scenarios: bv2dedicated with the admin API, plus bv2 autotest clients under Xvfb.

Usage: e2e.py <runtime dir> <scratch dir> <scenario>
Scenarios: join_leave, password, chat, kick, ban, master_list. Each has one observable: the admin API's
`GET /status` player count, the server's stdout, or a client's autotest result (ADR 0025). Linux only (Xvfb).
"""
import os
import shutil
import subprocess
import sys
import threading
import time
import urllib.request

ADMIN_PASS = "e2e-admin-pass"
ADMIN_PORT = 18080
JOIN_PASSWORD = "e2e-secret"


class Run:
    def __init__(self, runtime, scratch, name):
        self.runtime = os.path.abspath(runtime)
        self.dir = os.path.join(scratch, name)
        shutil.rmtree(self.dir, ignore_errors=True)
        os.makedirs(self.dir)
        self.procs = []
        self.server_out = []
        read, write = os.pipe()  # Xvfb writes its display number here
        self.start(["Xvfb", "-displayfd", str(write), "-screen", "0", "1280x720x24", "-nolisten", "tcp"],
                   pass_fds=[write], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        os.close(write)
        with os.fdopen(read) as f:
            self.display = ":" + f.readline().strip()

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

    def master(self):
        home = os.path.join(self.dir, "master")
        os.makedirs(home)
        for db in ("master.db", "web.db"):
            shutil.copy(os.path.join(self.runtime, db), home)
        self.start([os.path.join(self.runtime, "bv2master")], cwd=home, stdout=subprocess.DEVNULL, stderr=subprocess.STDOUT)
        wait(lambda: port_open(10207), 10, "master listening on 10207")

    def server(self, **env):
        env = dict(os.environ, BV2_PREF_DIR=os.path.join(self.dir, "server"), BV2_ADMIN_PASS=ADMIN_PASS,
                   BV2_ADMIN_API_PORT=str(ADMIN_PORT), **env)
        p = self.start([os.path.join(self.runtime, "bv2dedicated")], cwd=self.runtime, env=env, stdin=subprocess.PIPE,
                       stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        threading.Thread(target=self.collect, args=(p.stdout, self.server_out), daemon=True).start()
        p.stdin.write(b"execute CTF\n")
        p.stdin.flush()
        wait(lambda: "Server Created on port 3333" in self.server_text(), 30, "server created")
        wait(lambda: self.players() == 0, 10, "admin API answering")

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

    def players(self):
        """Player count from GET /status, or None while the API doesn't answer"""
        time.sleep(1.2)  # the API's budget refills one request per second
        try:
            for line in self.admin("GET", "/status").splitlines():
                if line.startswith("players "):
                    return int(line.split()[1])
        except OSError:
            return None

    def command(self, text):
        return self.admin("POST", "/command", text.encode())

    def client(self, name, steps, **env):
        """Starts a bv2 autotest client named `name`; returns its process (exit 0 = its script passed)"""
        pref = os.path.join(self.dir, name)
        os.makedirs(pref)
        script, config = os.path.join(pref, "script.txt"), os.path.join(pref, "e2e.cfg")
        with open(script, "w") as f:
            f.write("\n".join(steps) + "\n")
        with open(config, "w") as f:
            f.write(f'r_fullScreen false\ncl_playerName "{name}"\n')
        env = dict(os.environ, DISPLAY=self.display, BV2_PREF_DIR=pref, SDL_AUDIODRIVER="dummy", **env)
        log = open(os.path.join(pref, "client.log"), "w")
        return self.start([os.path.join(self.runtime, "bv2"), "--placeholders", "--config", config, "--autotest", script],
                          cwd=self.runtime, env=env, stdout=log, stderr=subprocess.STDOUT)

    def finished(self, client, timeout=90):
        """The client's exit code once its script ended"""
        try:
            return client.wait(timeout)
        except subprocess.TimeoutExpired:
            return None


def port_open(port):
    import socket
    with socket.socket() as s:
        return s.connect_ex(("127.0.0.1", port)) == 0


def wait(condition, seconds, what):
    end = time.time() + seconds
    while time.time() < end:
        if condition():
            return
        time.sleep(0.5)
    raise AssertionError(f"timed out after {seconds} s waiting for: {what}")


def check(ok, what):
    if not ok:
        raise AssertionError(what)


JOIN = ["expect-scene menu", "connect 127.0.0.1", "expect-scene game"]


def join_leave(run):
    run.server()
    client = run.client("e2e-a", JOIN + ["wait 6"])
    wait(lambda: run.players() == 1, 40, "/status players 1 after the join")
    check(run.finished(client) == 0, "client script passed")
    wait(lambda: run.players() == 0, 20, "/status players 0 after the client left")


def password(run):
    run.server(BV2_SV_PASSWORD=JOIN_PASSWORD)
    wrong = run.client("e2e-wrong", ["expect-scene menu", "connect 127.0.0.1 3333 wrong", "expect-console Server disconnected"])
    check(run.finished(wrong) == 0, "wrong password: client sees the server disconnect it")
    check(run.players() == 0, "wrong password: /status players 0")
    right = run.client("e2e-right", ["expect-scene menu", f"connect 127.0.0.1 3333 {JOIN_PASSWORD}", "expect-scene game", "wait 6"])
    wait(lambda: run.players() == 1, 40, "right password: /status players 1")
    check(run.finished(right) == 0, "right password: client script passed")


def chat(run):
    run.server()
    client = run.client("e2e-a", JOIN + ["wait 2", "sayall e2e-hello", "wait 3"])
    check(run.finished(client) == 0, "client script passed")
    check("e2e-hello" in run.server_text(), "server stdout has the chat message")


def kick(run):
    run.server()
    client = run.client("e2e-a", JOIN + ["expect-console Server disconnected"])
    wait(lambda: run.players() == 1, 40, "/status players 1 before the kick")
    run.command("kick e2e-a")
    wait(lambda: run.players() == 0, 10, "/status players 0 after POST /command kick")
    check(run.finished(client) == 0, "kicked client sees the server disconnect it")


def ban(run):
    run.server()
    client = run.client("e2e-a", JOIN + ["expect-console Server disconnected"])
    wait(lambda: run.players() == 1, 40, "/status players 1 before the ban")
    run.command("ban e2e-a")
    wait(lambda: run.players() == 0, 10, "/status players 0 after POST /command ban")
    check(run.finished(client) == 0, "banned client sees the server disconnect it")
    again = run.client("e2e-b", ["expect-scene menu", "connect 127.0.0.1", "wait 10"])
    seen = [run.players() for _ in range(6)]
    check(run.finished(again) == 0 and 1 not in seen, f"banned address can't rejoin: /status players {seen}")


def master_list(run):
    run.master()
    masters = "127.0.0.1:10207"
    run.server(BV2_MASTER_SERVERS=masters)
    client = run.client("e2e-a", ["expect-scene menu", "wait 3", "refreshgames", "expect-console Games found : 1"],
                        BV2_MASTER_SERVERS=masters)
    check(run.finished(client) == 0, "client lists the server: Games found : 1")


SCENARIOS = {f.__name__: f for f in (join_leave, password, chat, kick, ban, master_list)}

if __name__ == "__main__":
    runtime, scratch, name = sys.argv[1:4]
    run = Run(runtime, scratch, name)
    try:
        SCENARIOS[name](run)
    except AssertionError as e:
        print(f"FAIL {name}: {e}")
        print("--- server stdout (tail)\n" + run.server_text()[-3000:])
        for client in sorted(os.listdir(run.dir)):
            log = os.path.join(run.dir, client, "client.log")
            if os.path.exists(log):
                print(f"--- {client} (tail)\n" + open(log, errors="replace").read()[-3000:])
        sys.exit(1)
    finally:
        run.stop()
    print(f"ok: e2e {name}")
