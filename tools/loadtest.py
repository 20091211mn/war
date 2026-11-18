#!/usr/bin/env python3
"""قياس حمل الخادم المخصص: يشغّل خادم headless ثم عملاء وهميين (نسخ كاملة من اللعبة headless تتحرك وتدور)
ويقيس استهلاك معالج *الخادم فقط* من /proc. الاستعمال:
  python3 tools/loadtest.py --godot /path/to/godot [--clients 0,1,2,4] [--bots 3] [--secs 20]
ملاحظة: العملاء تتشارك المعالج مع الخادم على نفس الجهاز، فالأرقام تقديرية وأدنى من الواقع إن كانت الأنوية قليلة."""
import argparse, os, subprocess, time, signal, sys

ap = argparse.ArgumentParser()
ap.add_argument("--godot", required=True); ap.add_argument("--project", default=os.path.join(os.path.dirname(__file__), "..", "godot"))
ap.add_argument("--clients", default="0,1,2,4"); ap.add_argument("--bots", type=int, default=3); ap.add_argument("--maxp", type=int, default=6)
ap.add_argument("--secs", type=int, default=20); ap.add_argument("--port", type=int, default=9500)
a = ap.parse_args()
HZ = os.sysconf("SC_CLK_TCK"); NC = os.cpu_count()

def cpu_ticks(pid):
    with open(f"/proc/{pid}/stat") as f: p = f.read().rsplit(")", 1)[1].split()
    return int(p[11]) + int(p[12])   # utime + stime

def run(n):
    base = [a.godot, "--headless", "--path", a.project, "--", f"--port={a.port}"]
    srv = subprocess.Popen(base + ["--dedicated", f"--maxp={a.maxp}", f"--nbots={a.bots}", "--map=1"], stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    time.sleep(8)
    cl = [subprocess.Popen(base + ["--lanjoin=127.0.0.1", f"--frames={(a.secs + 20) * 60}"], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL) for _ in range(n)]
    time.sleep(10)                       # انتظار انضمام العملاء واستقرار المباراة
    t0, c0 = time.time(), cpu_ticks(srv.pid)
    time.sleep(a.secs)
    t1, c1 = time.time(), cpu_ticks(srv.pid)
    cpu = (c1 - c0) / HZ / (t1 - t0) * 100.0          # نسبة من نواة واحدة
    srv.send_signal(signal.SIGTERM)
    out = srv.communicate(timeout=10)[0].splitlines()
    for c in cl: c.kill()
    last = [l for l in out if l.startswith("DED")][-1:] or ["?"]
    return cpu, last[0]

print(f"أنوية الجهاز: {NC} | غرفة {a.maxp + 2} خانات + {a.bots} بوتات")
for n in [int(x) for x in a.clients.split(",")]:
    cpu, last = run(n)
    print(f"عملاء={n}: معالج الخادم ≈ {cpu:.0f}% من نواة | {last}", flush=True)
    time.sleep(2)
