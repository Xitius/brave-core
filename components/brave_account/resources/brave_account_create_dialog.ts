/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { CrLitElement } from '//resources/lit/v3_0/lit.rollup.js'

import {
  BraveAccountBrowserProxy,
  BraveAccountBrowserProxyImpl,
} from './brave_account_browser_proxy.js'
import { getHtml } from './brave_account_create_dialog.html.js'
import {
  RegisterClientErrorCode,
  RegisterError,
  ResetPasswordClientErrorCode,
  ResetPasswordError,
} from './brave_account.mojom-webui.js'
import { showError } from './brave_account_common.js'

// @ts-expect-error
import { Registration } from 'chrome://resources/brave/opaque_ke.bundle.js'

export type CreateDialogMode = 'create' | 'resetPassword'

export class BraveAccountCreateDialogElement extends CrLitElement {
  static get is() {
    return 'brave-account-create-dialog'
  }

  override render() {
    return getHtml.bind(this)()
  }

  static override get properties() {
    return {
      email: { type: String },
      isCapsLockOn: { type: Boolean },
      isSubmitting: { type: Boolean, state: true },
      isEmailValid: { type: Boolean },
      isPasswordStrongEnough: { type: Boolean },
      isPasswordValid: { type: Boolean },
      mode: { type: String },
      password: { type: String },
      passwordConfirmation: { type: String },
    }
  }

  // The reason this happens here (rather than in BraveAccountService) is that
  // both `registration.start()` and `registration.finish()` invoke the OPAQUE
  // protocol in our WASM (compiled from Rust), and so the flow must run in the
  // renderer to manage the transient cryptographic state — the service only
  // transports the two server round trips. We'll revisit handling this
  // through Mojo in C++ if that proves practical.
  protected async onSubmitButtonClicked() {
    if (this.isSubmitting) return
    this.isSubmitting = true

    if (this.mode === 'resetPassword') {
      await this.runResetPassword()
    } else {
      await this.runCreateAccount()
    }

    this.isSubmitting = false
  }

  private async runCreateAccount() {
    try {
      const blindedMessage = this.registration.start(this.password)
      const { encryptedVerificationToken, serializedResponse } =
        await this.browserProxy.authentication.registerInitialize(
          this.browserProxy.getInitiatingService(),
          this.email,
          blindedMessage,
        )
      const serializedRecord = this.registration.finish(
        serializedResponse,
        this.password,
        this.email,
      )
      await this.browserProxy.authentication.registerFinalize(
        encryptedVerificationToken,
        serializedRecord,
      )
    } catch (e) {
      let error: RegisterError

      if (e && typeof e === 'object') {
        error = e as RegisterError
      } else if (typeof e === 'string') {
        error = {
          clientError: { errorCode: RegisterClientErrorCode.kOpaqueError },
        }
      } else {
        console.error('Unexpected error:', e)
        error = {
          clientError: { errorCode: RegisterClientErrorCode.kUnexpected },
        }
      }

      showError({ kind: 'register', details: error })
    }
  }

  private async runResetPassword() {
    try {
      const blindedMessage = this.registration.start(this.password)
      const { encryptedVerificationToken, serializedResponse } =
        await this.browserProxy.authentication.resetPasswordInitialize(
          blindedMessage,
        )
      const serializedRecord = this.registration.finish(
        serializedResponse,
        this.password,
        this.email,
      )
      await this.browserProxy.authentication.resetPasswordFinalize(
        encryptedVerificationToken,
        serializedRecord,
      )
    } catch (e) {
      let error: ResetPasswordError

      if (e && typeof e === 'object') {
        error = e as ResetPasswordError
      } else if (typeof e === 'string') {
        error = {
          clientError: { errorCode: ResetPasswordClientErrorCode.kOpaqueError },
        }
      } else {
        console.error('Unexpected error:', e)
        error = {
          clientError: { errorCode: ResetPasswordClientErrorCode.kUnexpected },
        }
      }

      showError({ kind: 'resetPassword', details: error })
    }
  }

  private browserProxy: BraveAccountBrowserProxy =
    BraveAccountBrowserProxyImpl.getInstance()

  accessor mode: CreateDialogMode = 'create'
  // For mode='resetPassword' this is set by the parent (forgot-password flow);
  // for mode='create' it's bound to the email-input field in the template.
  accessor email: string = ''

  protected accessor isCapsLockOn: boolean = false
  protected accessor isSubmitting: boolean = false
  protected accessor isEmailValid: boolean = false
  protected accessor isPasswordStrongEnough: boolean = false
  protected accessor isPasswordValid: boolean = false
  protected accessor password: string = ''
  protected accessor passwordConfirmation: string = ''
  protected registration = new Registration()
}

declare global {
  interface HTMLElementTagNameMap {
    'brave-account-create-dialog': BraveAccountCreateDialogElement
  }
}

customElements.define(
  BraveAccountCreateDialogElement.is,
  BraveAccountCreateDialogElement,
)
