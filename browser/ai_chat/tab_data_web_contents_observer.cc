// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/tab_data_web_contents_observer.h"

#include <utility>

#include "base/functional/bind.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/browser/ai_chat/tab_tracker_service_factory.h"
#include "brave/components/ai_chat/core/browser/tab_tracker_service.h"
#include "brave/components/ai_chat/core/common/mojom/tab_tracker.mojom.h"
#include "chrome/browser/history/history_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "components/history/core/browser/history_service.h"
#include "components/history/core/browser/history_types.h"
#include "components/tabs/public/tab_interface.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"

namespace ai_chat {

namespace {

mojom::TabDataPtr CreateTabDataFromWebContents(
    content::WebContents* web_contents) {
  auto tab = mojom::TabData::New();
  tab->content_id =
      web_contents->GetController().GetLastCommittedEntry()->GetUniqueID();
  tab->title = base::UTF16ToUTF8(web_contents->GetTitle());
  tab->url = web_contents->GetLastCommittedURL();
  return tab;
}

}  // namespace

TabDataWebContentsObserver::TabDataWebContentsObserver(
    int32_t tab_handle,
    content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents),
      tab_handle_(tab_handle),
      service_(*TabTrackerServiceFactory::GetForBrowserContext(
          web_contents->GetBrowserContext())) {}

TabDataWebContentsObserver::~TabDataWebContentsObserver() {
  service_->UpdateTab(tab_handle_, nullptr);
}

void TabDataWebContentsObserver::TitleWasSet(content::NavigationEntry* entry) {
  UpdateTab();
}

void TabDataWebContentsObserver::PrimaryPageChanged(content::Page& page) {
  UpdateTab();
}

void TabDataWebContentsObserver::UpdateTab() {
  auto tab = CreateTabDataFromWebContents(web_contents());
  tab->id = tab_handle_;
  const GURL url = tab->url;
  const int32_t content_id = tab->content_id;
  service_->UpdateTab(tab_handle_, std::move(tab));

  // Kick off an async URL→URLID resolution so subsequent updates from this
  // observer (or other consumers) can read the indexed URLID off the tracked
  // TabData. The CancelableTaskTracker drops any prior in-flight resolution.
  auto* profile =
      Profile::FromBrowserContext(web_contents()->GetBrowserContext());
  if (!profile || !url.SchemeIsHTTPOrHTTPS()) {
    return;
  }
  auto* history_service = HistoryServiceFactory::GetForProfile(
      profile, ServiceAccessType::EXPLICIT_ACCESS);
  if (!history_service) {
    return;
  }
  url_id_task_tracker_.TryCancelAll();
  history_service->QueryURL(
      url,
      base::BindOnce(&TabDataWebContentsObserver::OnUrlIdResolved,
                     base::Unretained(this), tab_handle_, content_id),
      &url_id_task_tracker_);
}

void TabDataWebContentsObserver::OnUrlIdResolved(
    int32_t tab_handle,
    int64_t expected_content_id,
    history::QueryURLResult result) {
  if (!result.success || result.row.id() == 0) {
    return;
  }
  // The tab may have navigated since we issued the QueryURL; refetch the
  // current TabData and only update if content_id still matches. This avoids
  // attaching a stale URLID to a newer URL.
  const auto& tabs = service_->GetTabs();
  for (const auto& tab : tabs) {
    if (!tab || tab->id != tab_handle ||
        tab->content_id != expected_content_id) {
      continue;
    }
    auto updated = tab->Clone();
    updated->url_id = result.row.id();
    service_->UpdateTab(tab_handle, std::move(updated));
    return;
  }
}

}  // namespace ai_chat
