/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_account/brave_account_service.h"

#include <utility>
#include <variant>

#include "absl/functional/overload.h"
#include "base/check.h"
#include "base/check_deref.h"
#include "base/check_is_test.h"
#include "base/functional/bind.h"
#include "base/types/expected.h"
#include "brave/components/brave_account/state_internal.h"
#include "components/os_crypt/async/browser/os_crypt_async.h"
#include "components/prefs/pref_service.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace brave_account {

using internal::MakeClientError;

BraveAccountService::BraveAccountService(
    PrefService* pref_service,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    os_crypt_async::OSCryptAsync* os_crypt_async)
    : account_state_prefs_(CHECK_DEREF(pref_service)),
      url_loader_factory_(std::move(url_loader_factory)) {
  CHECK(url_loader_factory_);

  CHECK_DEREF(os_crypt_async)
      .GetInstance(base::BindOnce(&BraveAccountService::FinishInitialization,
                                  weak_factory_.GetWeakPtr()));
}

BraveAccountService::~BraveAccountService() = default;

void BraveAccountService::BindInterface(
    mojo::PendingReceiver<mojom::Authentication> pending_receiver) {
  if (std::holds_alternative<std::monostate>(state_)) {
    return pending_receivers_.push_back(std::move(pending_receiver));
  }
  authentication_receivers_.Add(this, std::move(pending_receiver));
}

base::OneShotTimer* BraveAccountService::AuthValidateTimerForTesting() {
  CHECK_IS_TEST();
  auto* logged_in = std::get_if<LoggedInState>(&state_);
  return logged_in ? &logged_in->auth_validate_timer_for_testing()  // IN-TEST
                   : nullptr;
}

void BraveAccountService::FinishInitialization(
    os_crypt_async::Encryptor encryptor) {
  encryptor_ = std::move(encryptor);

  EnsureState(account_state_prefs_.GetAccountState()->which());

  account_state_prefs_.StartObserving(base::BindRepeating(
      &BraveAccountService::OnAccountStateChanged, base::Unretained(this)));

  for (auto& pending_receiver : pending_receivers_) {
    authentication_receivers_.Add(this, std::move(pending_receiver));
  }
  decltype(pending_receivers_)().swap(pending_receivers_);
}

void BraveAccountService::AddObserver(
    mojo::PendingRemote<mojom::AuthenticationObserver> observer) {
  const auto observer_id = observers_.Add(std::move(observer));
  CHECK_DEREF(observers_.Get(observer_id))
      .OnAccountStateChanged(account_state_prefs_.GetAccountState());
}

void BraveAccountService::RegisterInitialize(
    mojom::Service initiating_service,
    const std::string& email,
    const std::string& blinded_message,
    RegisterInitializeCallback callback) {
  CHECK_DEREF(std::get_if<LoggedOutState>(&state_))
      .RegisterInitialize(initiating_service, email, blinded_message,
                          std::move(callback));
}

void BraveAccountService::RegisterFinalize(
    const std::string& encrypted_verification_token,
    const std::string& serialized_record,
    RegisterFinalizeCallback callback) {
  CHECK_DEREF(std::get_if<LoggedOutState>(&state_))
      .RegisterFinalize(encrypted_verification_token, serialized_record,
                        std::move(callback));
}

void BraveAccountService::RegisterVerify(const std::string& code,
                                         RegisterVerifyCallback callback) {
  CHECK_DEREF(std::get_if<LoggedOutState>(&state_))
      .RegisterVerify(code, std::move(callback));
}

void BraveAccountService::ResendConfirmationEmail(
    ResendConfirmationEmailCallback callback) {
  CHECK_DEREF(std::get_if<LoggedOutState>(&state_))
      .ResendConfirmationEmail(std::move(callback));
}

void BraveAccountService::CancelRegistration() {
  CHECK_DEREF(std::get_if<LoggedOutState>(&state_)).CancelRegistration();
}

void BraveAccountService::LoginInitialize(mojom::Service initiating_service,
                                          const std::string& email,
                                          const std::string& serialized_ke1,
                                          LoginInitializeCallback callback) {
  CHECK_DEREF(std::get_if<LoggedOutState>(&state_))
      .LoginInitialize(initiating_service, email, serialized_ke1,
                       std::move(callback));
}

void BraveAccountService::LoginFinalize(
    const std::string& encrypted_login_token,
    const std::string& client_mac,
    LoginFinalizeCallback callback) {
  CHECK_DEREF(std::get_if<LoggedOutState>(&state_))
      .LoginFinalize(encrypted_login_token, client_mac, std::move(callback));
}

void BraveAccountService::LogOut() {
  CHECK_DEREF(std::get_if<LoggedInState>(&state_)).LogOut();
}

void BraveAccountService::GetServiceToken(mojom::Service service,
                                          GetServiceTokenCallback callback) {
  std::visit(
      absl::Overload{
          [&](LoggedInState& state) {
            state.GetServiceToken(service, std::move(callback));
          },
          [&](const LoggedOutState&) {
            DUMP_WILL_BE_CHECK(false)
                << "GetServiceToken() called in LoggedOutState; wait for "
                   "LoggedInState via "
                   "AuthenticationObserver before requesting service tokens.";
            std::move(callback).Run(
                base::unexpected(MakeClientError<mojom::GetServiceTokenError>(
                    mojom::GetServiceTokenClientErrorCode::kUnexpected)));
          },
          [](std::monostate) {
            // BindInterface() queues receivers until initialization completes,
            // so this is unreachable. Listed only for visitor exhaustiveness.
            CHECK(false);
          }},
      state_);
}

void BraveAccountService::OnAccountStateChanged() {
  const auto account_state = account_state_prefs_.GetAccountState();
  EnsureState(account_state->which());

  for (auto& observer : observers_) {
    observer->OnAccountStateChanged(account_state.Clone());
  }
}

void BraveAccountService::EnsureState(mojom::AccountState::Tag which) {
  // NOLINTBEGIN(readability/braces) - false positive on templated lambda.
  const auto ensure_state = [&]<typename State>() {
    if (!std::holds_alternative<State>(state_)) {
      state_.emplace<State>(account_state_prefs_, url_loader_factory_,
                            *encryptor_);
    }
  };
  // NOLINTEND(readability/braces)

  switch (which) {
    case mojom::AccountState::Tag::kLoggedOut:
      return ensure_state.operator()<LoggedOutState>();
    case mojom::AccountState::Tag::kLoggedIn:
      return ensure_state.operator()<LoggedInState>();
  }
}

}  // namespace brave_account
