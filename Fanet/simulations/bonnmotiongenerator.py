import random, math

N = 25
T = 60.0

# area must match your constraintArea*
MINX, MAXX = 0.0, 1250.0
MINY, MAXY = 0.0, 1250.0

# movement knobs (tune)
V_MIN, V_MAX = 5.0, 10.0          # m/s
PAUSE_MIN, PAUSE_MAX = 0.0, 0.2   # s
SEED = 1

random.seed(SEED)

def clamp(v, lo, hi): return lo if v < lo else hi if v > hi else v

lines = []
for i in range(N):
    t = 0.0
    x = random.uniform(100.0, 1100.0)
    y = random.uniform(100.0, 1100.0)
    pts = [(t, x, y)]

    while t < T:
        # pause
        pause = random.uniform(PAUSE_MIN, PAUSE_MAX)
        t2 = min(T, t + pause)
        if t2 > t:
            pts.append((t2, x, y))
            t = t2
            if t >= T: break

        # choose next waypoint
        tx = random.uniform(MINX, MAXX)
        ty = random.uniform(MINY, MAXY)
        v  = random.uniform(V_MIN, V_MAX)

        dx, dy = tx - x, ty - y
        dist = math.hypot(dx, dy)
        if dist < 1e-9:
            continue

        travel = dist / v
        t2 = min(T, t + travel)

        # linear interpolation if truncated by end time
        frac = (t2 - t) / travel
        nx = x + dx * frac
        ny = y + dy * frac

        nx = clamp(nx, MINX, MAXX)
        ny = clamp(ny, MINY, MAXY)

        pts.append((t2, nx, ny))
        t, x, y = t2, nx, ny

    # BonnMotion native format: space-separated floats
    line = " ".join(f"{tt:.6f} {xx:.6f} {yy:.6f}" for (tt, xx, yy) in pts)
    lines.append(line)

with open("bonnmotion.movements", "w") as f:
    f.write("\n".join(lines) + "\n")

print("Wrote bonnmotion.movements with", N, "lines")