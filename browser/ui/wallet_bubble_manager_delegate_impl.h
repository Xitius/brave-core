/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WALLET_BUBBLE_MANAGER_DELEGATE_IMPL_H_
#define BRAVE_BROWSER_UI_WALLET_BUBBLE_MANAGER_DELEGATE_IMPL_H_

#include <memory>

#include "base/memory/raw_ptr.h"
#include "brave/browser/ui/brave_wallet/wallet_bubble_manager_delegate.h"
#include "url/gurl.h"

namespace brave_wallet {

class WalletWebUIBubbleManager;

class WalletBubbleManagerDelegateImpl : public WalletBubbleManagerDelegate {
 public:
  explicit WalletBubbleManagerDelegateImpl(content::WebContents* web_contents,
                                           const GURL& webui_url);

  // Returns true if the constructor routed the request to the wallet side
  // panel instead of creating a popup bubble.
  bool redirected_to_side_panel() const { return redirected_to_side_panel_; }
  WalletBubbleManagerDelegateImpl(const WalletBubbleManagerDelegateImpl&) =
      delete;
  WalletBubbleManagerDelegateImpl& operator=(
      const WalletBubbleManagerDelegateImpl&) = delete;
  ~WalletBubbleManagerDelegateImpl() override;

  void ShowBubble() override;
  void CloseBubble() override;
  bool IsShowingBubble() override;
  bool IsBubbleClosedForTesting() override;
  content::WebContents* GetWebContentsForTesting() override;

 private:
  // `this` never outlives `web_contents_` instance.
  raw_ptr<content::WebContents> web_contents_ = nullptr;
  bool redirected_to_side_panel_ = false;
  std::unique_ptr<WalletWebUIBubbleManager> webui_bubble_manager_;
};

}  // namespace brave_wallet

#endif  // BRAVE_BROWSER_UI_WALLET_BUBBLE_MANAGER_DELEGATE_IMPL_H_
