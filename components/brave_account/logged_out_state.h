/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ACCOUNT_LOGGED_OUT_STATE_H_
#define BRAVE_COMPONENTS_BRAVE_ACCOUNT_LOGGED_OUT_STATE_H_

#include <string>

#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_account/brave_account_state_prefs.h"
#include "brave/components/brave_account/endpoints/login_finalize.h"
#include "brave/components/brave_account/endpoints/login_init.h"
#include "brave/components/brave_account/endpoints/password_finalize.h"
#include "brave/components/brave_account/endpoints/password_init.h"
#include "brave/components/brave_account/endpoints/verify_complete.h"
#include "brave/components/brave_account/endpoints/verify_resend.h"
#include "brave/components/brave_account/mojom/brave_account.mojom.h"
#include "brave/components/brave_account/state_base.h"
#include "components/os_crypt/async/common/encryptor.h"

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

namespace brave_account {

class LoggedOutState : private StateBase {
 public:
  LoggedOutState(
      AccountStatePrefs& account_state_prefs,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      const os_crypt_async::Encryptor& encryptor);

  LoggedOutState(const LoggedOutState&) = delete;
  LoggedOutState& operator=(const LoggedOutState&) = delete;

  ~LoggedOutState();

  void RegisterInitialize(
      mojom::Service initiating_service,
      const std::string& email,
      const std::string& blinded_message,
      mojom::Authentication::RegisterInitializeCallback callback);

  void RegisterFinalize(
      const std::string& encrypted_verification_token,
      const std::string& serialized_record,
      mojom::Authentication::RegisterFinalizeCallback callback);

  void RegisterVerify(const std::string& code,
                      mojom::Authentication::RegisterVerifyCallback callback);

  void ResendConfirmationEmail(
      mojom::Authentication::ResendConfirmationEmailCallback callback);

  void CancelRegistration();

  void LoginInitialize(mojom::Service initiating_service,
                       const std::string& email,
                       const std::string& serialized_ke1,
                       mojom::Authentication::LoginInitializeCallback callback);

  void LoginFinalize(const std::string& encrypted_login_token,
                     const std::string& client_mac,
                     mojom::Authentication::LoginFinalizeCallback callback);

 private:
  void OnRegisterInitialize(
      mojom::Authentication::RegisterInitializeCallback callback,
      endpoints::PasswordInit::Response response);

  void OnRegisterFinalize(
      mojom::Authentication::RegisterFinalizeCallback callback,
      const std::string& encrypted_verification_token,
      endpoints::PasswordFinalize::Response response);

  void OnRegisterVerify(mojom::Authentication::RegisterVerifyCallback callback,
                        endpoints::VerifyComplete::Response response);

  void OnResendConfirmationEmail(
      mojom::Authentication::ResendConfirmationEmailCallback callback,
      endpoints::VerifyResend::Response response);

  void OnLoginInitialize(
      mojom::Authentication::LoginInitializeCallback callback,
      endpoints::LoginInit::Response response);

  void OnLoginFinalize(mojom::Authentication::LoginFinalizeCallback callback,
                       endpoints::LoginFinalize::Response response);

  base::WeakPtrFactory<LoggedOutState> weak_factory_{this};
};

}  // namespace brave_account

#endif  // BRAVE_COMPONENTS_BRAVE_ACCOUNT_LOGGED_OUT_STATE_H_
