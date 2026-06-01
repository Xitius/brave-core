// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { mangle } from 'lit_mangler'

// Inject a Semantic History Search toggle above the side bar's #spacer.
// References module-level globals (set up in side_bar.ts) instead of
// `this.<prop>` so @webui-eslint/lit-reactive-properties doesn't complain
// — the rule only checks class-property accesses through `this`.
mangle(
  (element) => {
    const spacer = element.querySelector('#spacer')
    if (!spacer) {
      throw new Error(
        '[Brave History] Could not find #spacer. Has upstream changed?',
      )
    }
    spacer.insertAdjacentHTML(
      'beforebegin',
      `<div id="brave-history-embeddings-toggle"
            ?hidden="\${!globalThis.braveHistoryEmbeddingsFeatureEnabled}">
         <div class="text">
           <div class="label">$i18n{braveHistoryEmbeddingsToggleLabel}</div>
           <div class="description">
             $i18n{braveHistoryEmbeddingsToggleDescription}
           </div>
         </div>
         <cr-toggle
             ?checked="\${globalThis.braveHistoryEmbeddingsEnabled}"
             @change="\${globalThis.braveHistoryEmbeddingsOnChange}">
         </cr-toggle>
       </div>`,
    )
  },
  (literal) => literal.text.includes('id="spacer"'),
)
