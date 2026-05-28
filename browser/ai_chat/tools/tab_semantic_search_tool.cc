// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/tools/tab_semantic_search_tool.h"

#include <algorithm>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/containers/flat_set.h"
#include "base/functional/bind.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/strings/string_number_conversions.h"
#include "base/values.h"
#include "brave/browser/ai_chat/tab_tracker_service_factory.h"
#include "brave/components/ai_chat/core/browser/tab_tracker_service.h"
#include "brave/components/ai_chat/core/browser/tools/tool_input_properties.h"
#include "brave/components/ai_chat/core/browser/tools/tool_utils.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/tab_tracker.mojom.h"
#include "chrome/browser/history_embeddings/history_embeddings_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "components/history_embeddings/content/history_embeddings_service.h"
#include "components/history_embeddings/core/history_embeddings_search.h"
#include "url/gurl.h"

namespace ai_chat {

namespace {

constexpr size_t kDefaultResultCount = 5;
constexpr size_t kMaxResultCount = 20;

// Cap on how many rows we ask HistoryEmbeddingsService::Search() to return so
// there is headroom for the open-tab URL filter to find matches without
// missing them due to a low Search() `count`.
constexpr size_t kSearchCount = 100;

using OpenTab = internal::SemanticSearchTabInfo;

std::vector<OpenTab> SnapshotOpenTabs(Profile* profile) {
  std::vector<OpenTab> tabs;
  if (!profile) {
    return tabs;
  }
  auto* tracker = TabTrackerServiceFactory::GetForBrowserContext(profile);
  if (!tracker) {
    return tabs;
  }
  for (const auto& tab : tracker->GetTabs()) {
    if (!tab || !tab->url.SchemeIsHTTPOrHTTPS()) {
      continue;
    }
    tabs.push_back({tab->id, tab->title, tab->url, tab->url_id});
  }
  return tabs;
}

// Per-call state passed to HistoryEmbeddingsService::Search's
// RepeatingCallback. With `skip_answering=true` Search() only fires the
// callback once; `responded` guards against any unexpected re-entry.
struct PendingCall {
  size_t count = kDefaultResultCount;
  std::vector<OpenTab> tabs;
  Tool::UseToolCallback callback;
  bool responded = false;
};

std::string EmptyResultsJson() {
  base::DictValue root;
  root.Set("results", base::ListValue());
  std::string serialized;
  base::JSONWriter::Write(root, &serialized);
  return serialized;
}

void OnSearchResult(PendingCall* pending,
                    history_embeddings::SearchResult result) {
  if (pending->responded) {
    return;
  }
  pending->responded = true;
  std::string serialized = internal::BuildSemanticSearchResultsJson(
      pending->tabs, result, pending->count);
  std::vector<mojom::ToolArtifactPtr> artifacts;
  std::string sources_json = internal::BuildSemanticSearchTabSourcesJson(
      pending->tabs, result, pending->count);
  if (!sources_json.empty()) {
    auto artifact = mojom::ToolArtifact::New();
    artifact->type = mojom::kTabSourcesArtifactType;
    artifact->content_json = std::move(sources_json);
    artifacts.push_back(std::move(artifact));
  }
  std::move(pending->callback)
      .Run(CreateContentBlocksForText(serialized), std::move(artifacts));
}

}  // namespace

namespace internal {

std::string BuildSemanticSearchResultsJson(
    const std::vector<SemanticSearchTabInfo>& tabs,
    const history_embeddings::SearchResult& result,
    size_t count) {
  // Map open tabs by `url_id` so we can attach `tab_id`/`title`/`url` to
  // ranked results. We use `url_id` (not GURL) because Search's
  // `row.row.url()` comes from history's canonical form, which may differ
  // from the open tab's URL (trailing slashes, fragments, etc.). The
  // URL-id filter already guarantees every row corresponds to an open tab,
  // so the lookup just needs a stable identifier.
  base::flat_map<history::URLID, const SemanticSearchTabInfo*> by_url_id;
  for (const auto& tab : tabs) {
    if (tab.url_id != 0) {
      by_url_id.emplace(tab.url_id, &tab);
    }
  }

  base::ListValue results_list;
  for (const auto& row : result.scored_url_rows) {
    if (results_list.size() >= count) {
      break;
    }
    auto it = by_url_id.find(row.scored_url.url_id);
    if (it == by_url_id.end()) {
      continue;
    }
    base::DictValue entry;
    entry.Set("tab_id", base::NumberToString(it->second->tab_id));
    entry.Set("title", it->second->title);
    entry.Set("url", it->second->url.spec());
    results_list.Append(std::move(entry));
  }
  base::DictValue root;
  root.Set("results", std::move(results_list));
  std::string serialized;
  base::JSONWriter::Write(root, &serialized);
  return serialized;
}

std::string BuildSemanticSearchTabSourcesJson(
    const std::vector<SemanticSearchTabInfo>& tabs,
    const history_embeddings::SearchResult& result,
    size_t count) {
  base::flat_map<history::URLID, const SemanticSearchTabInfo*> by_url_id;
  for (const auto& tab : tabs) {
    if (tab.url_id != 0) {
      by_url_id.emplace(tab.url_id, &tab);
    }
  }

  base::ListValue sources;
  for (const auto& row : result.scored_url_rows) {
    if (sources.size() >= count) {
      break;
    }
    auto it = by_url_id.find(row.scored_url.url_id);
    if (it == by_url_id.end()) {
      continue;
    }
    base::DictValue entry;
    entry.Set("tab_id", it->second->tab_id);
    entry.Set("title", it->second->title);
    entry.Set("url", it->second->url.spec());
    sources.Append(std::move(entry));
  }
  if (sources.empty()) {
    return std::string();
  }
  base::DictValue root;
  root.Set("sources", std::move(sources));
  std::string serialized;
  base::JSONWriter::Write(root, &serialized);
  return serialized;
}

}  // namespace internal

TabSemanticSearchTool::TabSemanticSearchTool(Profile* profile)
    : profile_(profile) {}

TabSemanticSearchTool::~TabSemanticSearchTool() = default;

std::string_view TabSemanticSearchTool::Name() const {
  return mojom::kSemanticTabSearchToolName;
}

std::string_view TabSemanticSearchTool::Description() const {
  return "Semantically search the user's currently-open browser tabs by page "
         "content. Use this when the user asks to find one of their open tabs "
         "by what it contains (e.g. \"the tab about react hooks\"), not just "
         "by title or URL. The query and full page content stay on device; "
         "only matched tabs' titles and URLs are returned. Provide a "
         "natural-language query.";
}

std::optional<base::DictValue> TabSemanticSearchTool::InputProperties() const {
  return CreateInputProperties(
      {{"query",
        StringProperty(
            "Natural-language query describing the tab to find by content.")},
       {"count",
        IntegerProperty("Maximum number of matching tabs to return (default "
                        "5, max 20).")}});
}

std::optional<std::vector<std::string>>
TabSemanticSearchTool::RequiredProperties() const {
  return std::vector<std::string>{"query"};
}

std::variant<bool, mojom::PermissionChallengePtr>
TabSemanticSearchTool::RequiresUserInteractionBeforeHandling(
    const mojom::ToolUseEvent& tool_use) const {
  if (user_has_granted_permission_) {
    return false;
  }
  return mojom::PermissionChallenge::New(
      std::nullopt,
      std::string(
          "Allow Leo to semantically search your currently-open tabs? The "
          "search query and the full content of your tabs stay on this "
          "device. Only matched tabs' titles and URLs are sent to Brave AI "
          "as tool output. Permission applies to this conversation."));
}

void TabSemanticSearchTool::UserPermissionGranted(
    const std::string& tool_use_id) {
  user_has_granted_permission_ = true;
}

void TabSemanticSearchTool::UseTool(const std::string& input_json,
                                    UseToolCallback callback) {
  if (!user_has_granted_permission_) {
    std::move(callback).Run(
        CreateContentBlocksForText(
            "Permission to search open tabs has not been granted."),
        {});
    return;
  }
  auto input = base::JSONReader::ReadDict(input_json,
                                          base::JSON_PARSE_CHROMIUM_EXTENSIONS);
  if (!input.has_value()) {
    std::move(callback).Run(
        CreateContentBlocksForText(
            "Failed to parse input JSON. Provide a 'query' field."),
        {});
    return;
  }
  const auto* query = input->FindString("query");
  if (!query || query->empty()) {
    std::move(callback).Run(
        CreateContentBlocksForText("Missing required 'query' field."), {});
    return;
  }
  size_t count = kDefaultResultCount;
  if (auto count_opt = input->FindInt("count")) {
    count = std::clamp<size_t>(static_cast<size_t>(*count_opt), 1u,
                               kMaxResultCount);
  }

  auto tabs = SnapshotOpenTabs(profile_);
  auto* embeddings_service =
      HistoryEmbeddingsServiceFactory::GetForProfile(profile_);

  // Build the URL-id filter set from open tabs that have an indexed url_id
  // cached by TabDataWebContentsObserver. Tabs with url_id == 0 are either
  // un-indexed yet or not in history; they can't be content-matched anyway,
  // so skipping them costs nothing.
  base::flat_set<history::URLID> url_id_filter;
  for (const auto& tab : tabs) {
    if (tab.url_id != 0) {
      url_id_filter.insert(tab.url_id);
    }
  }

  if (tabs.empty() || !embeddings_service || url_id_filter.empty()) {
    std::move(callback).Run(CreateContentBlocksForText(EmptyResultsJson()), {});
    return;
  }

  auto pending = std::make_unique<PendingCall>();
  pending->count = count;
  pending->tabs = std::move(tabs);
  pending->callback = std::move(callback);
  // SearchWithUrlIdFilter stashes the filter set on the service and delegates
  // to upstream Search; SqlDatabase filters at SQL level via
  // `WHERE url_id IN (...)`. base::Owned keeps `pending` alive for the
  // lifetime of the RepeatingCallback bound state.
  embeddings_service->SearchWithUrlIdFilter(
      /*previous_search_result=*/nullptr, *query,
      /*time_range_start=*/std::nullopt, kSearchCount,
      /*skip_answering=*/true, std::move(url_id_filter),
      base::BindRepeating(&OnSearchResult, base::Owned(std::move(pending))));
}

}  // namespace ai_chat
