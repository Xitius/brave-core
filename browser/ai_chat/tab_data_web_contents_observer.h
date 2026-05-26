// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_AI_CHAT_TAB_DATA_WEB_CONTENTS_OBSERVER_H_
#define BRAVE_BROWSER_AI_CHAT_TAB_DATA_WEB_CONTENTS_OBSERVER_H_

#include "base/task/cancelable_task_tracker.h"
#include "brave/components/ai_chat/core/browser/tab_tracker_service.h"
#include "components/history/core/browser/history_types.h"
#include "content/public/browser/web_contents_observer.h"

namespace content {
class NavigationEntry;
class Page;
class WebContents;
}  // namespace content

namespace history {
struct QueryURLResult;
}

namespace ai_chat {

class TabTrackerService;

// This class informs the TabTrackerService about changes to tabs (i.e.
// creation, deletion, title/url updates). Each instance of this class is
// associated with a single tab.
class TabDataWebContentsObserver : public content::WebContentsObserver {
 public:
  TabDataWebContentsObserver(int32_t tab_handle,
                             content::WebContents* contents);
  ~TabDataWebContentsObserver() override;

  TabDataWebContentsObserver(const TabDataWebContentsObserver&) = delete;
  TabDataWebContentsObserver& operator=(const TabDataWebContentsObserver&) =
      delete;

  // content::WebContentsObserver:
  void PrimaryPageChanged(content::Page& page) override;
  void TitleWasSet(content::NavigationEntry* entry) override;

 private:
  void UpdateTab();
  void OnUrlIdResolved(int32_t tab_handle,
                       int64_t expected_content_id,
                       history::QueryURLResult result);

  int32_t tab_handle_ = 0;

  raw_ref<TabTrackerService> service_;
  // Tracks the most recent QueryURL call so stale callbacks (e.g. for a URL
  // the tab has since navigated away from) are dropped on arrival.
  base::CancelableTaskTracker url_id_task_tracker_;
};

}  // namespace ai_chat

#endif  // BRAVE_BROWSER_AI_CHAT_TAB_DATA_WEB_CONTENTS_OBSERVER_H_
