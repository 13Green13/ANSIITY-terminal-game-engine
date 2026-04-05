"""
Crystal Caverns — procedural cave generator
Outputs: cavern.tilemap + cavern.scene

Tile IDs (matching cavern.tilepalette):
  0  = air (transparent)
  3  = stone upper (gray #)
  4  = stone upper dark (dark gray #)
  5  = stone upper rough (gray %)
  6  = stone upper smooth (gray =)
  7  = mossy stone (green #)
  8  = moss detail (bright green &)
  9  = crystal stone (cyan #)
  10 = crystal detail (bright cyan *)
  11 = deep stone (yellow #)
  12 = deep stone (red #)
  13 = stalactite (v)
  14 = stalagmite (^)
  15 = crystal cyan (*)
  16 = crystal magenta (*)
  17 = water surface (~)
  18 = vine (|)
  19 = drip (~)
  20 = glow (')
"""

import random
import os

random.seed(42)

W = 800
H = 200

# Tile IDs
AIR = 0
STONE_UPPER = 3
STONE_UPPER_DARK = 4
STONE_UPPER_ROUGH = 5
STONE_UPPER_SMOOTH = 6
STONE_MOSS = 7
STONE_MOSS_DETAIL = 8
STONE_CRYSTAL = 9
STONE_CRYSTAL_DETAIL = 10
STONE_DEEP = 11
STONE_DEEP_RED = 12
STALACTITE = 13
STALAGMITE = 14
CRYSTAL_CYAN = 15
CRYSTAL_MAGENTA = 16
WATER = 17
VINE = 18
DRIP = 19
GLOW = 20

# ============================================================
# 1) Cellular automata cave generation
# ============================================================
grid = [[1] * W for _ in range(H)]

# Random fill (45% empty)
for y in range(2, H - 2):
    for x in range(2, W - 2):
        grid[y][x] = 0 if random.random() < 0.45 else 1

# Ensure borders are solid
for x in range(W):
    for t in range(3):
        grid[t][x] = 1
        grid[H - 1 - t][x] = 1
for y in range(H):
    for t in range(3):
        grid[y][t] = 1
        grid[y][W - 1 - t] = 1

# Cellular automata iterations (B678/S345678 - generates connected caves)
for iteration in range(5):
    new_grid = [row[:] for row in grid]
    for y in range(1, H - 1):
        for x in range(1, W - 1):
            neighbors = 0
            for dy in range(-1, 2):
                for dx in range(-1, 2):
                    if dy == 0 and dx == 0:
                        continue
                    neighbors += grid[y + dy][x + dx]
            if grid[y][x] == 0:
                # Birth: become wall if 5+ neighbors are walls
                new_grid[y][x] = 1 if neighbors >= 5 else 0
            else:
                # Survival: stay wall if 4+ neighbors are walls
                new_grid[y][x] = 1 if neighbors >= 4 else 0
    grid = new_grid

# ============================================================
# 2) Carve guaranteed corridors for connectivity
# ============================================================
# Horizontal corridors at key depths
for corridor_y in [H // 4, H // 2, 3 * H // 4]:
    for x in range(5, W - 5):
        for dy in range(-2, 3):
            y = corridor_y + dy
            if 0 < y < H - 1:
                grid[y][x] = 0

# Vertical shafts connecting corridors
for shaft_x in range(60, W - 5, 120):
    for y in range(H // 4 - 2, 3 * H // 4 + 3):
        for dx in range(-2, 3):
            x = shaft_x + dx
            if 0 < x < W - 1:
                grid[y][x] = 0

# Carve large chambers
chambers = []
for _ in range(8):
    cx = random.randint(40, W - 40)
    cy = random.randint(20, H - 20)
    rw = random.randint(12, 25)
    rh = random.randint(8, 15)
    chambers.append((cx, cy, rw, rh))
    for y in range(max(2, cy - rh), min(H - 2, cy + rh)):
        for x in range(max(2, cx - rw), min(W - 2, cx + rw)):
            dx = (x - cx) / rw
            dy = (y - cy) / rh
            if dx * dx + dy * dy < 1.0:
                grid[y][x] = 0

# ============================================================
# 3) Assign biome-appropriate tile IDs
# ============================================================
tilemap = [[AIR] * W for _ in range(H)]

for y in range(H):
    depth_ratio = y / H  # 0=top, 1=bottom

    for x in range(W):
        if grid[y][x] == 0:
            tilemap[y][x] = AIR
            continue

        # Upper third: gray stone
        if depth_ratio < 0.35:
            r = random.random()
            if r < 0.50:
                tilemap[y][x] = STONE_UPPER
            elif r < 0.70:
                tilemap[y][x] = STONE_UPPER_DARK
            elif r < 0.85:
                tilemap[y][x] = STONE_UPPER_ROUGH
            else:
                tilemap[y][x] = STONE_UPPER_SMOOTH

        # Middle third: moss + crystal
        elif depth_ratio < 0.65:
            r = random.random()
            if r < 0.35:
                tilemap[y][x] = STONE_MOSS
            elif r < 0.50:
                tilemap[y][x] = STONE_CRYSTAL
            elif r < 0.65:
                tilemap[y][x] = STONE_UPPER
            elif r < 0.75:
                tilemap[y][x] = STONE_MOSS_DETAIL
            else:
                tilemap[y][x] = STONE_CRYSTAL_DETAIL

        # Lower third: warm deep stone
        else:
            r = random.random()
            if r < 0.40:
                tilemap[y][x] = STONE_DEEP
            elif r < 0.65:
                tilemap[y][x] = STONE_DEEP_RED
            elif r < 0.80:
                tilemap[y][x] = STONE_UPPER_DARK
            else:
                tilemap[y][x] = STONE_CRYSTAL

# ============================================================
# 4) Add decorative details (non-solid, placed on edges)
# ============================================================

def is_air(x, y):
    if x < 0 or x >= W or y < 0 or y >= H:
        return False
    return grid[y][x] == 0

# Stalactites: hang from ceilings
for y in range(2, H - 2):
    for x in range(2, W - 2):
        if grid[y][x] == 1 and is_air(x, y + 1):
            # This is a ceiling edge
            if random.random() < 0.15:
                length = random.randint(1, 4)
                for dy in range(1, length + 1):
                    if y + dy < H and is_air(x, y + dy):
                        tilemap[y + dy][x] = STALACTITE
                    else:
                        break

# Stalagmites: grow from floors
for y in range(2, H - 2):
    for x in range(2, W - 2):
        if grid[y][x] == 1 and is_air(x, y - 1):
            # This is a floor edge
            if random.random() < 0.12:
                length = random.randint(1, 3)
                for dy in range(1, length + 1):
                    if y - dy >= 0 and is_air(x, y - dy):
                        tilemap[y - dy][x] = STALAGMITE
                    else:
                        break

# Crystal clusters: in middle biome, on wall edges
for y in range(int(H * 0.3), int(H * 0.7)):
    for x in range(3, W - 3):
        if grid[y][x] == 0:
            # Check if adjacent to wall
            adj_wall = False
            for dx, dy in [(-1, 0), (1, 0), (0, -1), (0, 1)]:
                nx, ny = x + dx, y + dy
                if 0 <= nx < W and 0 <= ny < H and grid[ny][nx] == 1:
                    adj_wall = True
                    break
            if adj_wall and random.random() < 0.04:
                tilemap[y][x] = CRYSTAL_CYAN if random.random() < 0.6 else CRYSTAL_MAGENTA

# Vines: hang from ceilings in moss biome
for y in range(int(H * 0.25), int(H * 0.65)):
    for x in range(3, W - 3):
        if grid[y][x] == 1 and is_air(x, y + 1):
            if random.random() < 0.08:
                length = random.randint(2, 7)
                for dy in range(1, length + 1):
                    if y + dy < H and is_air(x, y + dy) and tilemap[y + dy][x] == AIR:
                        tilemap[y + dy][x] = VINE
                    else:
                        break

# Water pools: on flat floor sections in the deep biome
for y in range(int(H * 0.6), H - 5):
    for x in range(5, W - 5):
        if grid[y][x] == 1 and is_air(x, y - 1):
            # Check flatness (3 consecutive floor tiles)
            flat = True
            for fx in range(x - 1, x + 2):
                if fx < 0 or fx >= W or grid[y][fx] == 0 or not is_air(fx, y - 1):
                    flat = False
            if flat and random.random() < 0.06:
                # Place water surface
                pool_width = random.randint(3, 8)
                for wx in range(x - pool_width // 2, x + pool_width // 2 + 1):
                    if 0 < wx < W - 1 and is_air(wx, y - 1) and grid[y][wx] == 1:
                        tilemap[y - 1][wx] = WATER

# Glow particles: scattered in crystal zone
for y in range(int(H * 0.3), int(H * 0.7)):
    for x in range(5, W - 5):
        if tilemap[y][x] == AIR and random.random() < 0.008:
            tilemap[y][x] = GLOW

# Drip particles: under stalactites
for y in range(3, H - 3):
    for x in range(3, W - 3):
        if tilemap[y][x] == STALACTITE and tilemap[y + 1][x] == AIR:
            if random.random() < 0.3:
                tilemap[y + 1][x] = DRIP

# ============================================================
# 5) Place spawn points + collectibles + generate scene
# ============================================================

# Find valid spawn: open area near top-left
def find_open_spot(start_x, start_y, search_w, search_h):
    """Find a spot in air with enough vertical clearance for a 3x5 sprite"""
    for y in range(start_y, min(start_y + search_h, H - 2)):
        for x in range(start_x, min(start_x + search_w, W - 2)):
            # Need 7 tiles of vertical air (5 sprite + 2 margin)
            clear = True
            for cy in range(y - 1, y + 6):
                for cx in range(x, x + 3):
                    if not is_air(cx, cy):
                        clear = False
                        break
                if not clear:
                    break
            if clear:
                return (x, y)
    return None

# Player spawn near top-left
player_pos = find_open_spot(10, H // 4 - 5, 40, 20)
if not player_pos:
    player_pos = (15, H // 4 - 3)

# Exit door near bottom-right
exit_pos = find_open_spot(W - 60, 3 * H // 4 - 5, 50, 20)
if not exit_pos:
    exit_pos = (W - 30, 3 * H // 4 - 3)

# Gems: place in interesting locations (near walls, in chambers)
gem_positions = []
gem_types = ['gem_cyan', 'gem_green', 'gem_red']

# Place gems along corridors and in chambers
for _ in range(60):
    attempts = 0
    while attempts < 50:
        gx = random.randint(15, W - 15)
        gy = random.randint(10, H - 10)
        # Must be in air with floor below, away from player spawn
        if (is_air(gx, gy) and is_air(gx, gy - 1)
            and gy + 1 < H and grid[gy + 1][gx] == 1
            and tilemap[gy][gx] == AIR
            and abs(gx - player_pos[0]) + abs(gy - player_pos[1]) > 30):
            # Not too close to other gems
            too_close = False
            for ox, oy, _ in gem_positions:
                if abs(gx - ox) + abs(gy - oy) < 15:
                    too_close = True
                    break
            if not too_close:
                gem_type = random.choice(gem_types)
                gem_positions.append((gx, gy - 1, gem_type))
                break
        attempts += 1

# Bats: spawn in upper/mid cave in open areas
bat_positions = []
for _ in range(25):
    attempts = 0
    while attempts < 50:
        bx = random.randint(20, W - 20)
        by = random.randint(8, int(H * 0.7))
        # Must be in air with ceiling above (within 5 tiles)
        if is_air(bx, by) and is_air(bx, by - 1):
            has_ceiling = False
            for dy in range(1, 6):
                if by - dy >= 0 and grid[by - dy][bx] == 1:
                    has_ceiling = True
                    break
            if has_ceiling:
                too_close = False
                for ox, oy in bat_positions:
                    if abs(bx - ox) + abs(by - oy) < 20:
                        too_close = True
                        break
                if not too_close and abs(bx - player_pos[0]) + abs(by - player_pos[1]) > 40:
                    bat_positions.append((bx, by))
                    break
        attempts += 1

# ============================================================
# 6) Write tilemap file
# ============================================================
# Use hex-like encoding: 0-9 for IDs 0-9, a-k for IDs 10-20
os.makedirs('levels', exist_ok=True)
with open('levels/cavern.tilemap', 'w') as f:
    f.write(f'{W}x{H}\n')
    for row in tilemap:
        line = ''
        for tile_id in row:
            if tile_id < 10:
                line += str(tile_id)
            else:
                line += chr(ord('a') + tile_id - 10)
                # a=10, b=11, c=12, d=13, e=14, f=15, g=16, h=17, i=18, j=19, k=20
        f.write(line + '\n')

# ============================================================
# 7) Write scene file
# ============================================================
with open('levels/cavern.scene', 'w') as f:
    f.write('# Crystal Caverns\n')
    f.write('tilemap   levels/cavern.tilemap\n')
    f.write('palette   levels/cavern.tilepalette\n')
    f.write('\n')
    f.write('camera_follow   miner\n')
    f.write('camera_smooth   0\n')
    f.write('camera_deadzone 120 40\n')
    f.write(f'camera_start    {max(0, player_pos[0] - 200)} {max(0, player_pos[1] - 50)}\n')
    f.write('\n')
    f.write(f'# Player\n')
    f.write(f'spawn miner {player_pos[0]} {player_pos[1]}\n')
    f.write('\n')
    f.write(f'# Exit\n')
    f.write(f'spawn exit_door {exit_pos[0]} {exit_pos[1]}\n')
    f.write('\n')
    f.write(f'# HUD\n')
    f.write(f'spawn hud 2 2\n')
    f.write('\n')
    f.write(f'# Gems ({len(gem_positions)} total)\n')
    for gx, gy, gtype in gem_positions:
        f.write(f'spawn {gtype} {gx} {gy}\n')
    f.write('\n')
    f.write(f'# Bats ({len(bat_positions)} total)\n')
    for bx, by in bat_positions:
        f.write(f'spawn bat {bx} {by}\n')

print(f'Generated {W}x{H} cavern tilemap')
print(f'Player at {player_pos}')
print(f'Exit at {exit_pos}')
print(f'{len(gem_positions)} gems, {len(bat_positions)} bats')
