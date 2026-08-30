#pragma once
#include <atomic>
#include <chrono>
#include <future>
#include <mutex>
#include <string>
#include "GraphSample.h"
#include "LocalTest.h"
#include "Particles.h"
#include "SampleBase.h"
#include "VirtualTexture.h"
#include "WindowEvent.h"
#include "World.h"
#include "shadow/ContactShadow.h"
#include "shadow/ShadowMap.h"

namespace raum {
using platform::Window;

constexpr uint32_t s_width = 1080u;
constexpr uint32_t s_height = 720u;

class Sample {
public:
    enum class LoadingState : uint8_t {
        Loading,
        Finalizing,
        Ready,
        Failed,
    };

    struct LoadingStatus {
        LoadingState state{LoadingState::Loading};
        float progress{0.0f};
        std::string message;
    };

    Sample(int argc, char** argv) {
        _world = new framework::World();

        _window = std::make_shared<platform::Window>(argc, argv, s_width, s_height);
        _lastFpsTime = std::chrono::steady_clock::now();
        _tickID = _window->addTick([&](std::chrono::milliseconds) {
            auto now = std::chrono::steady_clock::now();
            _frameCount.fetch_add(1, std::memory_order_relaxed);
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - _lastFpsTime).count();
            if (elapsed >= 500) {
                int count = _frameCount.exchange(0, std::memory_order_relaxed);
                _fps.store(static_cast<float>(count) * 1000.0f / static_cast<float>(elapsed), std::memory_order_relaxed);
                _lastFpsTime = now;
            }
            this->show();
        });

        _world->attachWindow(_window);

        _samples = {
            // std::make_shared<sample::GraphSample>(&_world->director()),

            std::make_shared<sample::LocalTestSample>(&_world->director()),
            // std::make_shared<sample::ParticlesSample>(&_world->director()),
            // std::make_shared<sample::VirtualTextureSample>(&_world->director()),
            // std::make_shared<sample::ShadowMapSample>(&_world->director()),
            // std::make_shared<sample::ContactShadowSample>(&_world->director()),

        };
        if (!_samples.empty()) {
            beginInitialLoad();
        }
    }

    ~Sample() {
        shutdown();
    }

    void shutdown() {
        if (!_world) {
            return;
        }

        if (_window && _tickID) {
            _window->removeTick(_tickID);
            _tickID = 0;
        }
        _resizeListener.remove();
        _resizeListener.add({});
        _closeListener.remove();
        _closeListener.add({});
        waitForInitialLoad();
        if (_currIndex < _samples.size()) {
            _samples[_currIndex]->deactivate();
        }
        _samples.clear();
        delete _world;
        _world = nullptr;
    }

    void showWindow() {
        if (!_windowShown && _window) {
            _windowShown = true;
            _window->show();
        }
    }

    void show() {
        if (_loadingState.load(std::memory_order_acquire) == LoadingState::Ready &&
            _currIndex < _samples.size()) {
            _samples[_currIndex]->render();
        }
    }

    void pollLoading() {
        if (_loadingState.load(std::memory_order_acquire) != LoadingState::Loading ||
            !_loadFuture.valid() ||
            _loadFuture.wait_for(std::chrono::seconds{0}) != std::future_status::ready) {
            return;
        }

        try {
            _loadFuture.get();
            setLoadingStatus(LoadingState::Finalizing, 0.99f, "Finalizing renderer state");
            _samples[_currIndex]->activate();

            auto pipeline = _world->director().pipeline();
            const auto size = _window->size();
            pipeline->resizeSwapchain(size.width, size.height, _window->handle());
            _resizeListener.add([this, pipeline](uint32_t width, uint32_t height) {
                pipeline->resizeSwapchain(width, height, _window->handle());
            });
            _world->run();
            setLoadingStatus(LoadingState::Ready, 1.0f, "Rendering first frame");
        } catch (const std::exception& error) {
            setLoadingStatus(LoadingState::Failed, 0.0f, error.what());
        } catch (...) {
            setLoadingStatus(LoadingState::Failed, 0.0f, "Unknown scene loading error");
        }
    }

    LoadingStatus loadingStatus() const {
        LoadingStatus status;
        status.state = _loadingState.load(std::memory_order_acquire);
        status.progress = _loadingProgress.load(std::memory_order_relaxed);
        {
            std::lock_guard lock(_loadingStatusMutex);
            status.message = _loadingMessage;
        }
        return status;
    }

    bool ready() const {
        return _loadingState.load(std::memory_order_acquire) == LoadingState::Ready;
    }

    bool readyForDisplay() const {
        return ready() && _world && _world->director().hasPresentedFrame();
    }

    platform::WindowPtr window() {
        return _window;
    }

    bool changeSample(uint32_t index) {
        if (!ready() || index >= _samples.size() || index == _currIndex) {
            return false;
        }
        // Initialize first so a failed lazy initialization leaves the current
        // sample active and usable.
        _samples[index]->initialize();
        _samples[_currIndex]->deactivate();
        _currIndex = index;
        _samples[_currIndex]->activate();
        auto ppl = _world->director().pipeline();
        ppl->graphScheduler().needWarmUp();
        return true;
    }

    const std::vector<std::shared_ptr<sample::SampleBase>>& samples() const {
        return _samples;
    }

    sample::SampleBase* currentSample() {
        if (_currIndex < _samples.size()) {
            return _samples[_currIndex].get();
        }
        return nullptr;
    }

    uint32_t currentSampleIndex() const {
        return _currIndex;
    }

    float getFps() const {
        return _fps.load(std::memory_order_relaxed);
    }

private:
    void beginInitialLoad() {
        setLoadingStatus(LoadingState::Loading, 0.0f, "Preparing scene resources");
        auto initialSample = _samples[_currIndex];
        _loadFuture = std::async(std::launch::async, [this, initialSample = std::move(initialSample)] {
            initialSample->load([this](float progress, std::string_view message) {
                setLoadingStatus(LoadingState::Loading, progress * 0.98f, message);
            });
        });
    }

    void waitForInitialLoad() noexcept {
        if (!_loadFuture.valid()) {
            return;
        }
        try {
            _loadFuture.get();
        } catch (...) {
            // Shutdown still has to release partially loaded renderer resources.
        }
    }

    void setLoadingStatus(LoadingState state, float progress, std::string_view message) {
        {
            std::lock_guard lock(_loadingStatusMutex);
            _loadingMessage.assign(message);
        }
        _loadingProgress.store(progress, std::memory_order_relaxed);
        _loadingState.store(state, std::memory_order_release);
    }

    uint32_t _currIndex{0};
    std::vector<std::shared_ptr<sample::SampleBase>> _samples;
    platform::WindowPtr _window;
    framework::World* _world{nullptr};
    framework::EventListener<framework::ResizeEventTag> _resizeListener;
    framework::EventListener<framework::CloseEventTag> _closeListener;
    platform::TickID _tickID{0};
    std::atomic<float> _fps{0.0f};
    std::atomic<int> _frameCount{0};
    std::chrono::steady_clock::time_point _lastFpsTime;
    std::future<void> _loadFuture;
    std::atomic<LoadingState> _loadingState{LoadingState::Loading};
    std::atomic<float> _loadingProgress{0.0f};
    mutable std::mutex _loadingStatusMutex;
    std::string _loadingMessage;
    bool _windowShown{false};
};

} // namespace raum
