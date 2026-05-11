/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ACCOUNT_STATE_BASE_H_
#define BRAVE_COMPONENTS_BRAVE_ACCOUNT_STATE_BASE_H_

#include <list>
#include <string>
#include <utility>

#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/memory/raw_ref.h"
#include "base/memory/scoped_refptr.h"
#include "brave/components/brave_account/brave_account_state_prefs.h"
#include "brave/components/brave_account/endpoint_client/client.h"
#include "brave/components/brave_account/endpoint_client/request_handle.h"
#include "components/os_crypt/async/common/encryptor.h"

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

namespace brave_account {

// `~StateBase` cancels all of its in-flight requests, so callers issuing
// those requests do not need per-callback "Did the state change?" checks:
// the destruction of the owning state is the cancellation. The rest of this
// comment explains how that cancellation is made safe in the face of
// already-queued responses.
//
// `~in_flight_` cancels the underlying loaders via `DeleteSoon()`, which is
// not synchronous: between "network request destruction begins" and "the
// corresponding loader is actually destroyed on the next loop iteration", a
// response that was already queued on the response task runner can still
// fire the bound user callback. Running that callback would apply a decision
// made by a state that is no longer current, even if it only touches
// long-lived entities (e.g. writing `account_state_prefs_` - owned by
// `BraveAccountService`, so still alive - with a value the new state has
// already overwritten).
//
// Two `WeakPtr`s drop those stale callbacks: the derived state's
// `weak_factory_` invalidates the user callback (the derived destructor
// runs before `~StateBase`), and `StateBase::weak_factory_` invalidates the
// internal slot-erase bind. Both factories are declared last in their
// respective classes so they destruct first. Response callbacks for
// `SendStateOwnedRequest()` must therefore be bound to a *member* function
// with `weak_factory_.GetWeakPtr()` - a static-function bind has no
// `WeakPtr` to invalidate.
class StateBase {
 public:
  StateBase(const StateBase&) = delete;
  StateBase& operator=(const StateBase&) = delete;

 protected:
  StateBase(AccountStatePrefs& account_state_prefs,
            scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
            const os_crypt_async::Encryptor& encryptor);

  ~StateBase();

  std::string Encrypt(const std::string& plain_text) const;
  std::string Decrypt(const std::string& base64) const;

  // Caller-owned: the returned handle cancels the request when destroyed.
  // Use when the request's lifetime is tied to something other than
  // `~StateBase` (e.g. replaced by the next scheduled attempt).
  template <typename Endpoint, typename Request, typename Response>
  [[nodiscard]] endpoint_client::RequestHandle SendCallerOwnedRequest(
      Request request,
      base::OnceCallback<void(Response)> callback) {
    return endpoint_client::Client<Endpoint>::template Send<
        endpoint_client::RequestCancelability::kCancelable>(
        url_loader_factory_, std::move(request), std::move(callback));
  }

  // State-owned: the handle is parked in `in_flight_` and cancelled by
  // `~StateBase`. The default for response-driven flows.
  template <typename Endpoint, typename Request, typename Response>
  void SendStateOwnedRequest(Request request,
                             base::OnceCallback<void(Response)> callback) {
    auto slot = in_flight_.emplace(in_flight_.end());
    *slot = SendCallerOwnedRequest<Endpoint>(
        std::move(request), std::move(callback).Then(base::BindOnce(
                                &StateBase::RemoveRequestHandle,
                                weak_factory_.GetWeakPtr(), slot)));
  }

  // Unowned: fire-and-forget, no callback, not cancelable. Use only for
  // best-effort notifications whose response is intentionally ignored.
  template <typename Endpoint, typename Request>
  void SendUnownedRequest(Request request) {
    endpoint_client::Client<Endpoint>::Send(
        url_loader_factory_, std::move(request),
        base::BindOnce([](typename Endpoint::Response) {}));
  }

  const raw_ref<AccountStatePrefs> account_state_prefs_;

 private:
  void RemoveRequestHandle(
      std::list<endpoint_client::RequestHandle>::iterator slot);

  const scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  const raw_ref<const os_crypt_async::Encryptor> encryptor_;
  std::list<endpoint_client::RequestHandle> in_flight_;
  base::WeakPtrFactory<StateBase> weak_factory_{this};
};

}  // namespace brave_account

#endif  // BRAVE_COMPONENTS_BRAVE_ACCOUNT_STATE_BASE_H_
