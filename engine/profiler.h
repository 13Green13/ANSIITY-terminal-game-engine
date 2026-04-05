#pragma once

#include <windows.h>
#include <fstream>
#include <string>

class Profiler {
    LARGE_INTEGER _freq;
    LARGE_INTEGER _frameStart;
    LARGE_INTEGER _sectionStart;

    // Per-frame timings (microseconds)
    float _inputTime = 0;
    float _updateTime = 0;
    float _collisionTime = 0;
    float _renderTime = 0;
    float _renderBuildTime = 0;
    float _renderFlushTime = 0;
    float _totalFrameTime = 0;

    // Rolling stats
    float _avgFrameTime = 0;
    float _maxFrameTime = 0;
    int _frameCount = 0;
    int _logInterval = 60;  // Log every N frames

    std::ofstream _logFile;
    bool _enabled = false;

    Profiler() {
        QueryPerformanceFrequency(&_freq);
    }

public:
    static Profiler& getInstance() {
        static Profiler instance;
        return instance;
    }

    Profiler(const Profiler&) = delete;
    void operator=(const Profiler&) = delete;

    void enable(const std::string& logPath = "perf_log.txt", int logEveryNFrames = 60) {
        _logFile.open(logPath, std::ios::trunc);
        _logInterval = logEveryNFrames;
        _enabled = true;
        _logFile << "Frame | Total(us) | Input(us) | Update(us) | Collision(us) | Render(us) | Build(us) | Flush(us) | Entities\n";
        _logFile << "------+-----------+-----------+------------+---------------+------------+-----------+-----------+---------\n";
    }

    void beginFrame() {
        if (!_enabled) return;
        QueryPerformanceCounter(&_frameStart);
    }

    void beginSection() {
        if (!_enabled) return;
        QueryPerformanceCounter(&_sectionStart);
    }

    float endSection() {
        if (!_enabled) return 0;
        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);
        return (float)(now.QuadPart - _sectionStart.QuadPart) * 1000000.0f / _freq.QuadPart;
    }

    void recordInput(float us) { _inputTime = us; }
    void recordUpdate(float us) { _updateTime = us; }
    void recordCollision(float us) { _collisionTime = us; }
    void recordRender(float us) { _renderTime = us; }
    void recordRenderBuild(float us) { _renderBuildTime = us; }
    void recordRenderFlush(float us) { _renderFlushTime = us; }

    void endFrame(int entityCount) {
        if (!_enabled) return;

        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);
        _totalFrameTime = (float)(now.QuadPart - _frameStart.QuadPart) * 1000000.0f / _freq.QuadPart;

        _frameCount++;

        // Rolling average
        _avgFrameTime = _avgFrameTime * 0.95f + _totalFrameTime * 0.05f;
        if (_totalFrameTime > _maxFrameTime) {
            _maxFrameTime = _totalFrameTime;
        }

        // Log periodically
        if (_frameCount % _logInterval == 0) {
            char buf[256];
            snprintf(buf, sizeof(buf),
                "%5d | %9.1f | %9.1f | %10.1f | %13.1f | %10.1f | %9.1f | %9.1f | %d\n",
                _frameCount, _totalFrameTime, _inputTime, _updateTime,
                _collisionTime, _renderTime, _renderBuildTime, _renderFlushTime, entityCount);
            _logFile << buf;
            _logFile.flush();
        }
    }

    void writeSummary() {
        if (!_enabled || !_logFile.is_open()) return;
        _logFile << "\n--- SUMMARY ---\n";
        _logFile << "Total frames: " << _frameCount << "\n";
        _logFile << "Avg frame time: " << _avgFrameTime << " us ("
                 << (_avgFrameTime / 1000.0f) << " ms)\n";
        _logFile << "Max frame time: " << _maxFrameTime << " us ("
                 << (_maxFrameTime / 1000.0f) << " ms)\n";
        _logFile << "Avg FPS (compute only): " << (1000000.0f / _avgFrameTime) << "\n";
        _logFile.close();
    }

    bool isEnabled() const { return _enabled; }
};
