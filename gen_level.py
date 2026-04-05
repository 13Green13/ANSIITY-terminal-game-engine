import os

W = 500
H = 60
grid = [['0'] * W for _ in range(H)]

# Ground: rows 57-59
for y in range(57, 60):
    for x in range(W):
        grid[y][x] = '1'

# Ground gaps (pits)
for y in range(57, 60):
    for x in range(100, 105): grid[y][x] = '0'
    for x in range(230, 236): grid[y][x] = '0'
    for x in range(370, 376): grid[y][x] = '0'

# Low platforms (row 52)
for x in range(50, 59):   grid[52][x] = '2'
for x in range(120, 131):  grid[52][x] = '2'
for x in range(250, 261):  grid[52][x] = '2'
for x in range(380, 391):  grid[52][x] = '2'
for x in range(420, 431):  grid[52][x] = '2'

# Mid platforms (row 47)
for x in range(135, 146):  grid[47][x] = '2'
for x in range(275, 286):  grid[47][x] = '2'
for x in range(310, 321):  grid[47][x] = '2'

# High platform (row 42)
for x in range(312, 319):  grid[42][x] = '2'

# Question blocks (row 49)
for x in range(35, 38):    grid[49][x] = '3'
for x in range(70, 73):    grid[49][x] = '3'
for x in range(170, 173):  grid[49][x] = '3'
grid[49][290] = '3'

# Pipes
# Short pipe: x=85-86, rows 55-56
for y in range(55, 57):
    grid[y][85] = '4'
    grid[y][86] = '4'

# Medium pipe: x=200-201, rows 53-56
for y in range(53, 57):
    grid[y][200] = '4'
    grid[y][201] = '4'

# Tall pipe: x=340-341, rows 51-56
for y in range(51, 57):
    grid[y][340] = '4'
    grid[y][341] = '4'

# Staircase: x=390-397
for step in range(6):
    col = 390 + step
    for y in range(56 - step, 57):
        grid[y][col] = '1'
# Flat top of staircase
for x in range(396, 400):
    for y in range(51, 57):
        grid[y][x] = '1'

os.makedirs('levels', exist_ok=True)
with open('levels/level1.tilemap', 'w') as f:
    f.write(f'{W}x{H}\n')
    for row in grid:
        f.write(''.join(row) + '\n')

print(f'Generated {W}x{H} tilemap')
