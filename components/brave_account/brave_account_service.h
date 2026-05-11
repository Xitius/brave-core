/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ACCOUNT_BRAVE_ACCOUNT_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_ACCOUNT_BRAVE_ACCOUNT_SERVICE_H_

#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_account/brave_account_state_prefs.h"
#include "brave/components/brave_account/logged_in_state.h"
#include "brave/components/brave_account/logged_out_state.h"
#include "brave/components/brave_account/mojom/brave_account.mojom.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/os_crypt/async/common/encryptor.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "mojo/public/cpp/bindings/remote_set.h"

class PrefService;

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

namespace os_crypt_async {
class OSCryptAsync;
}  // namespace os_crypt_async

namespace brave_account {

// BraveAccountService has no non-Mojom callers. Its only public entrypoint is
// `BindInterface()`, and this should remain the case. Receiver binding is
// deferred until `FinishInitialization()` installs the encryptor, and any
// service-initiated work that can reach encryption is also started only after
// that point.
//
// State-scoped network requests:
//
// `state_` is a `std::variant<>` whose active alternative reflects the current
// account state: `LoggedOutState` or `LoggedInState`. Each alternative owns
// the in-flight network requests that are only valid in its state. When
// `OnAccountStateChanged()` observes a pref transition between the two
// states, it replaces `state_`; the previous alternative's destructor cancels
// any requests it still holds, so their response callbacks can no longer run
// against a state they don't belong to.
class BraveAccountService : public KeyedService, public mojom::Authentication {
 public:
  BraveAccountService(
      PrefService* pref_service,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      os_crypt_async::OSCryptAsync* os_crypt_async);

  BraveAccountService(const BraveAccountService&) = delete;
  BraveAccountService& operator=(const BraveAccountService&) = delete;

  ~BraveAccountService() override;

  void BindInterface(
      mojo::PendingReceiver<mojom::Authentication> pending_receiver);

  base::OneShotTimer* AuthValidateTimerForTesting();

 private:
  void FinishInitialization(os_crypt_async::Encryptor encryptor);

  void AddObserver(
      mojo::PendingRemote<mojom::AuthenticationObserver> observer) override;

  void RegisterInitialize(mojom::Service initiating_service,
                          const std::string& email,
                          const std::string& blinded_message,
                          RegisterInitializeCallback callback) override;

  void RegisterFinalize(const std::string& encrypted_verification_token,
                        const std::string& serialized_record,
                        RegisterFinalizeCallback callback) override;

  void RegisterVerify(const std::string& code,
                      RegisterVerifyCallback callback) override;

  void ResendConfirmationEmail(
      ResendConfirmationEmailCallback callback) override;

  void CancelRegistration() override;

  void LoginInitialize(mojom::Service initiating_service,
                       const std::string& email,
                       const std::string& serialized_ke1,
                       LoginInitializeCallback callback) override;

  void LoginFinalize(const std::string& encrypted_login_token,
                     const std::string& client_mac,
                     LoginFinalizeCallback callback) override;

  void LogOut() override;

  void GetServiceToken(mojom::Service service,
                       GetServiceTokenCallback callback) override;

  void OnAccountStateChanged();

  void EnsureState(mojom::AccountState::Tag which);

  AccountStatePrefs account_state_prefs_;
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  std::optional<os_crypt_async::Encryptor> encryptor_;
  std::vector<mojo::PendingReceiver<mojom::Authentication>> pending_receivers_;
  mojo::ReceiverSet<mojom::Authentication> authentication_receivers_;
  mojo::RemoteSet<mojom::AuthenticationObserver> observers_;
  std::variant<std::monostate, LoggedOutState, LoggedInState> state_;
  base::WeakPtrFactory<BraveAccountService> weak_factory_{this};
};

}  // namespace brave_account

#endif  // BRAVE_COMPONENTS_BRAVE_ACCOUNT_BRAVE_ACCOUNT_SERVICE_H_
