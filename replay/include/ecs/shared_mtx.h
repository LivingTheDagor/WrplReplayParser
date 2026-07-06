#pragma once
#include "shared_mutex"

#include <tracy/Tracy.hpp>

namespace dagor {
#ifndef ECS_USE_WIN_MTX
  class shared_mutex {
    std::shared_mutex mtx_;
  public:
    shared_mutex() = default;
    shared_mutex(const shared_mutex&) = delete;
    shared_mutex& operator=(const shared_mutex&) = delete;

    void lock() { ZoneScoped; mtx_.lock(); }
    void unlock() { ZoneScoped; mtx_.unlock(); }

    void lock_shared() { ZoneScoped; mtx_.lock_shared(); }
    void unlock_shared() { ZoneScoped; mtx_.unlock_shared(); }

    bool try_lock() { ZoneScoped; return mtx_.try_lock(); }
    bool try_lock_shared() { ZoneScoped; return mtx_.try_lock_shared(); }
  };
#else
  // copied directly from MSVC STL library

  using _Smtx_t = void*;
  // shared mutex
  // these declarations must be in sync with those in sharedmutex.cpp
  void __cdecl _Smtx_lock_exclusive(_Smtx_t*) noexcept;
  void __cdecl _Smtx_lock_shared(_Smtx_t*) noexcept;
  int __cdecl _Smtx_try_lock_exclusive(_Smtx_t*) noexcept;
  int __cdecl _Smtx_try_lock_shared(_Smtx_t*) noexcept;
  void __cdecl _Smtx_unlock_exclusive(_Smtx_t*) noexcept;
  void __cdecl _Smtx_unlock_shared(_Smtx_t*) noexcept;


  class shared_mutex { // class for mutual exclusion shared across threads
  public:
    using native_handle_type = _Smtx_t*;

    shared_mutex()                               = default;
    shared_mutex(const shared_mutex&)            = delete;
    shared_mutex& operator=(const shared_mutex&) = delete;

    void lock() noexcept /* strengthened */ { // lock exclusive
      ZoneScoped;
      _Smtx_lock_exclusive(&_Myhandle);
    }

    [[nodiscard]] bool try_lock() noexcept /* strengthened */ { // try to lock exclusive
      ZoneScoped;
      return _Smtx_try_lock_exclusive(&_Myhandle) != 0;
    }

    void unlock() noexcept /* strengthened */ { // unlock exclusive
      ZoneScoped;
      _Smtx_unlock_exclusive(&_Myhandle);
    }

    void lock_shared() noexcept /* strengthened */ { // lock non-exclusive
      ZoneScoped;
      _Smtx_lock_shared(&_Myhandle);
    }

    [[nodiscard]] bool try_lock_shared() noexcept /* strengthened */ { // try to lock non-exclusive
      ZoneScoped;
      return _Smtx_try_lock_shared(&_Myhandle) != 0;
    }

    void unlock_shared() noexcept /* strengthened */ { // unlock non-exclusive
      ZoneScoped;
      _Smtx_unlock_shared(&_Myhandle);
    }

    [[nodiscard]] native_handle_type native_handle() noexcept /* strengthened */ { // get native handle
      ZoneScoped;
      return &_Myhandle;
    }

  private:
    _Smtx_t _Myhandle = nullptr;
  };
#endif
}