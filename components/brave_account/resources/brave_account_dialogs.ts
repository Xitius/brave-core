/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import {
  AccountState,
  AccountStateFieldTags,
  LoggedOutVerificationIntent,
  whichAccountState,
} from './brave_account.mojom-webui.js'
import { assert } from '//resources/js/assert.js'
import { CrLitElement } from '//resources/lit/v3_0/lit.rollup.js'
import { EventTracker } from '//resources/js/event_tracker.js'
// <if expr="not is_android and not is_ios">
import { hasKeyModifiers } from '//resources/js/util.js'
// </if>

import {
  BraveAccountBrowserProxy,
  BraveAccountBrowserProxyImpl,
} from './brave_account_browser_proxy.js'
import { getHtml } from './brave_account_dialogs.html.js'

export type Dialog =
  | 'CREATE'
  | 'ENTRY'
  | 'FORGOT_PASSWORD'
  | 'OTP'
  | 'SET_NEW_PASSWORD'
  | 'SIGN_IN'

export class BraveAccountDialogsElement extends CrLitElement {
  static get is() {
    return 'brave-account-dialogs'
  }

  override render() {
    return getHtml.bind(this)()
  }

  static override get properties() {
    return {
      dialog: { type: String },
      intent: { type: Number, state: true },
      isCapsLockOn: { type: Boolean, state: true },
      resetPasswordEmail: { type: String, state: true },
    }
  }

  protected onBackButtonClicked() {
    assert(this.dialog)
    this.dialog = this.dialog === 'FORGOT_PASSWORD' ? 'SIGN_IN' : 'ENTRY'
  }

  protected onPasswordResetRequested(e: Event) {
    const detail = (e as CustomEvent<{ email: string }>).detail
    this.resetPasswordEmail = detail.email
  }

  protected onResetPasswordVerified() {
    this.dialog = 'SET_NEW_PASSWORD'
  }

  protected onCloseDialog() {
    this.browserProxy.closeDialog()
  }

  private browserProxy: BraveAccountBrowserProxy =
    BraveAccountBrowserProxyImpl.getInstance()

  protected accessor dialog: Dialog | undefined = undefined
  protected accessor intent: LoggedOutVerificationIntent =
    LoggedOutVerificationIntent.kRegistration
  protected accessor isCapsLockOn: boolean = false
  protected accessor resetPasswordEmail: string = ''

  private accountStateListenerId: number | null = null
  private eventTracker = new EventTracker()

  override connectedCallback() {
    super.connectedCallback()

    this.eventTracker.add(this, 'back-button-clicked', this.onBackButtonClicked)
    this.eventTracker.add(this, 'close-dialog', this.onCloseDialog)
    this.eventTracker.add(
      this,
      'password-reset-requested',
      this.onPasswordResetRequested,
    )
    this.eventTracker.add(
      this,
      'reset-password-verified',
      this.onResetPasswordVerified,
    )
    // <if expr="not is_android and not is_ios">
    this.eventTracker.add(document, 'keydown', this.onKeyDown)
    this.eventTracker.add(document, 'keyup', this.onKeyUp)
    // </if>

    // Handle account state changes.
    // LOGGED_OUT (no verification): show the ENTRY dialog
    // LOGGED_OUT (with verification): show the OTP dialog
    // LOGGED_IN: close the native dialog
    // Exception: if the user has already advanced past OTP into
    // SET_NEW_PASSWORD locally (reset-password flow), keep that dialog —
    // the underlying account state hasn't changed yet, but the renderer has.
    // Since account state is profile-wide, this automatically updates dialogs
    // across all tabs.
    this.accountStateListenerId =
      this.browserProxy.authenticationObserverCallbackRouter.onAccountStateChanged.addListener(
        (state: AccountState) => {
          switch (whichAccountState(state)) {
            case AccountStateFieldTags.LOGGED_OUT:
              if (state.loggedOut!.verification) {
                this.intent = state.loggedOut!.verification.intent
                if (this.dialog !== 'SET_NEW_PASSWORD') {
                  this.dialog = 'OTP'
                }
              } else {
                this.dialog = 'ENTRY'
              }
              break
            case AccountStateFieldTags.LOGGED_IN:
              this.onCloseDialog()
              break
          }
        },
      )
  }

  override disconnectedCallback() {
    super.disconnectedCallback()

    assert(this.accountStateListenerId)
    this.browserProxy.authenticationObserverCallbackRouter.removeListener(
      this.accountStateListenerId,
    )

    this.eventTracker.removeAll()
  }

  // <if expr="not is_android and not is_ios">
  private onKeyDown = (e: KeyboardEvent) => {
    this.isCapsLockOn = e.getModifierState('CapsLock')

    // Ignore keys pressed with modifiers (Ctrl, Shift, etc.).
    if (hasKeyModifiers(e)) {
      return
    }

    switch (e.key) {
      // Clicks the action button (only if there's exactly one enabled).
      case 'Enter': {
        const dialog = [...(this.shadowRoot?.children ?? [])].find(
          (el) => el instanceof HTMLElement && el.shadowRoot,
        )

        const buttons = dialog?.shadowRoot?.querySelectorAll<HTMLElement>(
          'leo-button[slot="buttons"]:not([isDisabled])',
        )

        if (buttons?.length === 1) {
          buttons[0]!.click()
          e.preventDefault()
        }
        break
      }
      // Closes the dialog.
      case 'Escape':
        this.onCloseDialog()
        e.preventDefault()
        break
    }
  }

  private onKeyUp = (e: KeyboardEvent) => {
    this.isCapsLockOn = e.getModifierState('CapsLock')
  }
  // </if>
}

declare global {
  interface HTMLElementTagNameMap {
    'brave-account-dialogs': BraveAccountDialogsElement
  }
}

customElements.define(BraveAccountDialogsElement.is, BraveAccountDialogsElement)
