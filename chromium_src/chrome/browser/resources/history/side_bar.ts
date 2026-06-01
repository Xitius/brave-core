// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import 'chrome://resources/cr_elements/cr_toggle/cr_toggle.js'

import {injectStyle} from '//resources/brave/lit_overriding.js'
import {loadTimeData} from '//resources/js/load_time_data.js'
import {css} from '//resources/lit/v3_0/lit.rollup.js'

import {HistorySideBarElement} from './side_bar-chromium.js'

injectStyle(HistorySideBarElement, css`
  .cr-nav-menu-item {
    min-height: 20px !important;
    border-end-end-radius: 0px !important;
    border-start-end-radius: 0px !important;
    box-sizing: content-box !important;
  }
  .cr-nav-menu-item:hover {
    background: transparent !important;
  }
  .cr-nav-menu-item[selected] {
    --iron-icon-fill-color: var(--cr-link-color) !important;
    color: var(--cr-link-color) !important;
    background: transparent !important;
  }
  .cr-nav-menu-item cr-icon {
    display: none !important;
  }
  .cr-nav-menu-item cr-ripple {
    display: none !important;
  }
  #brave-history-embeddings-toggle {
    align-items: center;
    border-top: 1px solid var(--cr-separator-color);
    display: flex;
    gap: 12px;
    margin-top: 8px;
    padding: 12px 16px 8px;
  }
  #brave-history-embeddings-toggle .text {
    flex: 1;
  }
  #brave-history-embeddings-toggle .label {
    font-weight: 500;
  }
  #brave-history-embeddings-toggle .description {
    color: var(--cr-secondary-text-color);
    font-size: 12px;
    margin-top: 2px;
  }
`)

// Exposed as module-level bindings (not class properties) so the
// lit_mangler-injected template in side_bar.html.ts can reference them
// without tripping @webui-eslint/lit-reactive-properties, which only fires
// on `this.<prop>` accesses.
const featureEnabled =
    loadTimeData.valueExists('isHistoryEmbeddingsFeatureEnabled') &&
    loadTimeData.getBoolean('isHistoryEmbeddingsFeatureEnabled')
const initialEnabled =
    loadTimeData.valueExists('isHistoryEmbeddingsEnabled') &&
    loadTimeData.getBoolean('isHistoryEmbeddingsEnabled')

declare global {
  var braveHistoryEmbeddingsFeatureEnabled: boolean
  var braveHistoryEmbeddingsEnabled: boolean
  var braveHistoryEmbeddingsOnChange: (e: CustomEvent<boolean>) => void
}

globalThis.braveHistoryEmbeddingsFeatureEnabled = featureEnabled
globalThis.braveHistoryEmbeddingsEnabled = initialEnabled
globalThis.braveHistoryEmbeddingsOnChange = (e: CustomEvent<boolean>) => {
  chrome.send('setHistoryEmbeddingsEnabled', [e.detail])
  // The upstream history WebUI consumes `enableHistoryEmbeddings` once at
  // page load via loadTimeData, so the search UI doesn't react to pref
  // changes mid-session. Reload so the new state takes effect immediately.
  window.location.reload()
}

export * from './side_bar-chromium.js'
