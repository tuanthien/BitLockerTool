#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <Shlobj.h>
#include <shellapi.h>

#include <string_view>

namespace Blt {
constexpr auto toCompatView(auto capture) -> std::string_view
{
  auto view          = capture.to_view();
  using ViewCharType = typename std::remove_cvref_t<decltype(view)>::value_type;
  return std::string_view(reinterpret_cast<const char *>(view.data()), view.size() * sizeof(ViewCharType));
};

namespace Detail {
  template<typename Fn>
  struct scope_exit_impl : Fn
  {
    scope_exit_impl(Fn &&fn)
      : Fn(std::forward<Fn>(fn))
    {}
    ~scope_exit_impl() noexcept { (*this)(); }
  };
  struct make_scope_exit_impl
  {
    template<typename Fn>
    auto operator->*(Fn &&fn) const
    {
      return scope_exit_impl(std::forward<Fn>(fn));
    }
  };
  template<typename Fn>
  scope_exit_impl(Fn &&fn) -> scope_exit_impl<Fn>;

}// namespace Detail

#define BLT_CONC(A, B) BLT_CONC_(A, B)
#define BLT_CONC_(A, B) A##B

#define blt_defer auto BLT_CONC(_blt_scope_exit_, __LINE__) = ::Blt::Detail::make_scope_exit_impl{}->*[&]()

}