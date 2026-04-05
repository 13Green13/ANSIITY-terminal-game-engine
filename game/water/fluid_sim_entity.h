#pragma once

#include "../../engine/engine.h"
#include "../../engine/input_manager.h"
#include "../../engine/constants.h"
#include <vector>
#include <cmath>
#include <cstring>
#include <algorithm>

// ============================================================
// Position-Based Fluid Simulation
// (Clavet et al. 2005 - "Particle-based Viscoelastic Fluid")
//
// Position-based, not force-based SPH. Directly constrains
// particle positions to maintain target density. Near-density
// term acts as surface tension. Inherently stable.
// ============================================================

struct Particle {
    float x, y;
    float prevX, prevY;
    float vx, vy;
};

// Spatial hash for neighbor lookup
class SpatialHash {
    static constexpr int BUCKET_COUNT = 4096;
    std::vector<int> _buckets[BUCKET_COUNT];
    float _cellSize;

    int hashCoord(int cx, int cy) const {
        unsigned int h = (unsigned int)(cx * 92837111) ^ (unsigned int)(cy * 689287499);
        return (int)(h & (BUCKET_COUNT - 1));
    }

public:
    void init(float cellSize) { _cellSize = cellSize; }

    void clear() {
        for (int i = 0; i < BUCKET_COUNT; i++) _buckets[i].clear();
    }

    void insert(int idx, float x, float y) {
        int cx = (int)std::floor(x / _cellSize);
        int cy = (int)std::floor(y / _cellSize);
        _buckets[hashCoord(cx, cy)].push_back(idx);
    }

    template<typename Fn>
    void queryNeighbors(float x, float y, Fn&& fn) const {
        int cx = (int)std::floor(x / _cellSize);
        int cy = (int)std::floor(y / _cellSize);
        for (int dx = -1; dx <= 1; dx++) {
            for (int dy = -1; dy <= 1; dy++) {
                int h = hashCoord(cx + dx, cy + dy);
                for (int idx : _buckets[h]) {
                    fn(idx);
                }
            }
        }
    }
};

// Renderer
class FluidRenderer : public ANSIIRenderer {
    std::vector<Particle>* _particles = nullptr;
    int* _cursorX = nullptr;
    int* _cursorY = nullptr;

public:
    FluidRenderer() {
        width = WIDTH;
        height = HEIGHT;
        screenSpace = true;
        zOrder = 10;
    }

    void bind(std::vector<Particle>* p, int* cx, int* cy) {
        _particles = p; _cursorX = cx; _cursorY = cy;
    }

    void draw(char sc[WIDTH][HEIGHT], int sco[WIDTH][HEIGHT],
              bool occ[WIDTH][HEIGHT], int, int) const override {
        if (!_particles) return;

        float buf[WIDTH][HEIGHT];
        std::memset(buf, 0, sizeof(buf));

        for (auto& p : *_particles) {
            int px = (int)p.x;
            int py = (int)p.y;
            if (px >= 0 && px < WIDTH && py >= 0 && py < HEIGHT) {
                buf[px][py] += 1.0f;
            }
        }

        for (int x = 0; x < WIDTH; x++) {
            for (int y = 0; y < HEIGHT; y++) {
                if (occ[x][y]) continue;
                float d = buf[x][y];
                if (d <= 0) continue;

                bool surface = false;
                for (int dx = -1; dx <= 1 && !surface; dx++)
                    for (int dy = -1; dy <= 1 && !surface; dy++) {
                        int nx = x + dx, ny = y + dy;
                        if (nx < 0 || nx >= WIDTH || ny < 0 || ny >= HEIGHT ||
                            buf[nx][ny] <= 0)
                            surface = true;
                    }

                if (surface) {
                    sc[x][y] = '~';
                    sco[x][y] = 96;
                } else if (d >= 3.0f) {
                    sc[x][y] = '#';
                    sco[x][y] = 34;
                } else if (d >= 1.5f) {
                    sc[x][y] = '=';
                    sco[x][y] = 94;
                } else {
                    sc[x][y] = '-';
                    sco[x][y] = 36;
                }
                occ[x][y] = true;
            }
        }

        // Cursor
        if (_cursorX && _cursorY) {
            int cx = *_cursorX, cy = *_cursorY;
            static const int offs[][2] = {{0,0},{-1,0},{1,0},{0,-1},{0,1}};
            for (auto& o : offs) {
                int px = cx + o[0], py = cy + o[1];
                if (px >= 0 && px < WIDTH && py >= 0 && py < HEIGHT) {
                    sc[px][py] = '+';
                    sco[px][py] = 93;
                    occ[px][py] = true;
                }
            }
        }

        // HUD
        const char* help = "Arrows:Move  Space:Pour  Q:Quit";
        for (int i = 0; help[i]; i++) {
            int hx = 2 + i;
            if (hx < WIDTH) {
                sc[hx][0] = help[i];
                sco[hx][0] = 90;
                occ[hx][0] = true;
            }
        }
        char pbuf[32];
        int len = snprintf(pbuf, sizeof(pbuf), "Particles: %d", (int)_particles->size());
        for (int i = 0; i < len; i++) {
            int hx = WIDTH - len - 2 + i;
            if (hx >= 0 && hx < WIDTH) {
                sc[hx][0] = pbuf[i];
                sco[hx][0] = 90;
                occ[hx][0] = true;
            }
        }
    }
};

class FluidSimEntity : public Entity {
    std::vector<Particle> _particles;
    SpatialHash _hash;

    int _cursorX, _cursorY;
    float _fx, _fy;
    float _cursorSpeed = 120.0f;
    float _spawnTimer = 0;

    // --- Simulation parameters ---
    static constexpr float H              = 5.0f;   // interaction radius
    static constexpr float REST_DENSITY   = 4.0f;   // target density (q-units)
    static constexpr float STIFFNESS      = 0.5f;   // pressure strength
    static constexpr float NEAR_STIFFNESS = 1.0f;   // near-pressure = surface tension
    static constexpr float GRAVITY_ACCEL  = 60.0f;
    static constexpr float VISC_SIGMA     = 0.5f;   // linear viscosity
    static constexpr float VISC_BETA      = 0.3f;   // quadratic viscosity
    static constexpr float WALL_DAMP      = 0.3f;   // velocity retained on wall bounce
    static constexpr int   MAX_PARTICLES  = 5000;
    static constexpr int   SUB_STEPS      = 3;

    void step(float dt) {
        int n = (int)_particles.size();
        if (n == 0) return;

        // === 1. Apply gravity, save old pos, predict new pos ===
        for (int i = 0; i < n; i++) {
            Particle& p = _particles[i];
            p.vy += GRAVITY_ACCEL * dt;
            p.prevX = p.x;
            p.prevY = p.y;
            p.x += p.vx * dt;
            p.y += p.vy * dt;
        }

        // === 2. Build spatial hash on predicted positions ===
        _hash.clear();
        for (int i = 0; i < n; i++) {
            _hash.insert(i, _particles[i].x, _particles[i].y);
        }

        // === 3. Viscosity impulses (before relaxation) ===
        // Reduces relative velocity between neighbors — makes
        // the fluid flow cohesively instead of as individual dots.
        for (int i = 0; i < n; i++) {
            Particle& pi = _particles[i];
            _hash.queryNeighbors(pi.x, pi.y, [&](int j) {
                if (j <= i) return;
                Particle& pj = _particles[j];
                float dx = pj.x - pi.x;
                float dy = pj.y - pi.y;
                float r2 = dx * dx + dy * dy;
                if (r2 >= H * H || r2 < 1e-6f) return;
                float r = std::sqrt(r2);
                float q = 1.0f - r / H;
                float nx = dx / r, ny = dy / r;

                // Inward relative velocity
                float u = (pi.vx - pj.vx) * nx + (pi.vy - pj.vy) * ny;
                if (u > 0) {
                    float impulse = dt * q * (VISC_SIGMA * u + VISC_BETA * u * u);
                    float ix = impulse * nx * 0.5f;
                    float iy = impulse * ny * 0.5f;
                    pi.vx -= ix;
                    pi.vy -= iy;
                    pj.vx += ix;
                    pj.vy += iy;
                }
            });
        }

        // === 4. Double density relaxation ===
        // Core of Clavet et al.: directly move particle positions to
        // satisfy density constraints.
        //
        // density = sum(q^2) where q = 1 - r/h for each neighbor
        //   → measures how crowded this particle is
        //
        // nearDensity = sum(q^3) (steeper falloff)
        //   → penalizes VERY close pairs → acts as surface tension
        //     because surface particles have asymmetric neighbors
        //     and get pushed outward less, forming a cohesive boundary
        //
        // Pressure P = k * (density - restDensity)
        //   → positive when too crowded (repel), negative when sparse (attract slightly)
        //
        // NearPressure Pn = k_near * nearDensity
        //   → always positive (always repulsive at short range)
        //
        // Displacement D = dt^2 * (P*q + Pn*q^2) along pairwise direction
        //   → directly moves positions, no force integration needed

        for (int i = 0; i < n; i++) {
            Particle& pi = _particles[i];
            float density = 0;
            float nearDensity = 0;

            _hash.queryNeighbors(pi.x, pi.y, [&](int j) {
                if (j == i) return;
                float dx = _particles[j].x - pi.x;
                float dy = _particles[j].y - pi.y;
                float r2 = dx * dx + dy * dy;
                if (r2 >= H * H) return;
                float r = std::sqrt(r2);
                float q = 1.0f - r / H;
                density += q * q;
                nearDensity += q * q * q;
            });

            float P = STIFFNESS * (density - REST_DENSITY);
            float Pnear = NEAR_STIFFNESS * nearDensity;

            float dispX = 0, dispY = 0;

            _hash.queryNeighbors(pi.x, pi.y, [&](int j) {
                if (j == i) return;
                Particle& pj = _particles[j];
                float dx = pj.x - pi.x;
                float dy = pj.y - pi.y;
                float r2 = dx * dx + dy * dy;
                if (r2 >= H * H || r2 < 1e-6f) return;
                float r = std::sqrt(r2);
                float q = 1.0f - r / H;
                float nx = dx / r, ny = dy / r;

                float D = dt * dt * (P * q + Pnear * q * q);
                float dxD = D * nx * 0.5f;
                float dyD = D * ny * 0.5f;

                pj.x += dxD;
                pj.y += dyD;
                dispX -= dxD;
                dispY -= dyD;
            });

            pi.x += dispX;
            pi.y += dispY;
        }

        // === 5. Derive velocity from position change ===
        // This is fundamental to position-based methods: velocity is
        // an output, not input. The relaxation step implicitly solved
        // the pressure field by moving positions.
        for (int i = 0; i < n; i++) {
            Particle& p = _particles[i];
            p.vx = (p.x - p.prevX) / dt;
            p.vy = (p.y - p.prevY) / dt;
        }

        // === 6. Boundary ===
        float minB = 1.0f;
        float maxXB = (float)(WIDTH - 2);
        float maxYB = (float)(HEIGHT - 2);

        for (int i = 0; i < n; i++) {
            Particle& p = _particles[i];
            if (p.x < minB)  { p.x = minB;  p.vx =  std::abs(p.vx) * WALL_DAMP; }
            if (p.x > maxXB) { p.x = maxXB; p.vx = -std::abs(p.vx) * WALL_DAMP; }
            if (p.y < minB)  { p.y = minB;  p.vy =  std::abs(p.vy) * WALL_DAMP; }
            if (p.y > maxYB) { p.y = maxYB; p.vy = -std::abs(p.vy) * WALL_DAMP; }
        }
    }

public:
    FluidSimEntity(float, float, const Props&) {
        tag = "fluid_sim";

        _cursorX = WIDTH / 2;
        _cursorY = 10;
        _fx = (float)_cursorX;
        _fy = (float)_cursorY;

        _hash.init(H);

        auto* fr = new FluidRenderer();
        fr->bind(&_particles, &_cursorX, &_cursorY);
        renderer = fr;

        auto& input = InputManager::getInstance();
        input.watchKey(VK_LEFT);
        input.watchKey(VK_RIGHT);
        input.watchKey(VK_UP);
        input.watchKey(VK_DOWN);
        input.watchKey(VK_SPACE);
    }

    void update(float dt) override {
        auto& input = InputManager::getInstance();

        if (input.isKeyHeld(VK_LEFT))  _fx -= _cursorSpeed * dt;
        if (input.isKeyHeld(VK_RIGHT)) _fx += _cursorSpeed * dt;
        if (input.isKeyHeld(VK_UP))    _fy -= _cursorSpeed * dt;
        if (input.isKeyHeld(VK_DOWN))  _fy += _cursorSpeed * dt;
        _fx = std::clamp(_fx, 0.0f, (float)(WIDTH - 1));
        _fy = std::clamp(_fy, 0.0f, (float)(HEIGHT - 1));
        _cursorX = (int)_fx;
        _cursorY = (int)_fy;

        // Spawn
        if (input.isKeyHeld(VK_SPACE) && (int)_particles.size() < MAX_PARTICLES) {
            _spawnTimer += dt;
            float spawnRate = 0.008f;
            while (_spawnTimer >= spawnRate && (int)_particles.size() < MAX_PARTICLES) {
                _spawnTimer -= spawnRate;
                Particle p;
                p.x = _fx + ((float)(rand() % 100) / 100.0f - 0.5f) * 2.0f;
                p.y = _fy + ((float)(rand() % 100) / 100.0f - 0.5f) * 1.0f;
                p.prevX = p.x;
                p.prevY = p.y;
                p.vx = ((float)(rand() % 100) / 100.0f - 0.5f) * 5.0f;
                p.vy = 0;
                _particles.push_back(p);
            }
        } else {
            _spawnTimer = 0;
        }

        float simDt = std::min(dt, 1.0f / 30.0f);
        float subDt = simDt / (float)SUB_STEPS;
        for (int s = 0; s < SUB_STEPS; s++) {
            step(subDt);
        }
    }
};
