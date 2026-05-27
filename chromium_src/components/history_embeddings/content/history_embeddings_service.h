/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_HISTORY_EMBEDDINGS_CONTENT_HISTORY_EMBEDDINGS_SERVICE_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_HISTORY_EMBEDDINGS_CONTENT_HISTORY_EMBEDDINGS_SERVICE_H_

#include "base/containers/flat_set.h"

// Make OnPassageVisibilityCalculated virtual and protected so
// BraveHistoryEmbeddingsService can override it to skip the
// scored_url_rows.clear() when annotations are empty (Brave doesn't
// use PageContentAnnotationsService).
#define OnPassageVisibilityCalculated \
  NotUsed() {}                        \
                                      \
 protected:                           \
  virtual void OnPassageVisibilityCalculated

// Add a thin `SearchWithUrlIdFilter` entry point that stashes a URL-id filter
// set on the service before delegating to upstream `Search`. Upstream
// `Search` (patched) drains the member into `result.search_params`, which
// `VectorDatabase::FindNearest` consults to route through the SQL-level
// `WHERE url_id IN (...)` filter installed by our chromium_src `SqlDatabase`.
// The member-then-Search dance keeps the abstract
// `HistoryEmbeddingsSearch::Search` API untouched.
#define SetPassagesStoredCallbackForTesting                                   \
  SetPassagesStoredCallbackForTesting_Unused();                               \
                                                                              \
  SearchResult SearchWithUrlIdFilter(                                         \
      SearchResult* previous_search_result, std::string query,                \
      std::optional<base::Time> time_range_start, size_t count,               \
      bool skip_answering, base::flat_set<history::URLID> url_id_filter,      \
      SearchResultCallback callback) {                                        \
    brave_pending_url_id_filter_ = std::move(url_id_filter);                  \
    return Search(previous_search_result, std::move(query), time_range_start, \
                  count, skip_answering, std::move(callback));                \
  }                                                                           \
                                                                              \
 private:                                                                     \
  base::flat_set<history::URLID> brave_pending_url_id_filter_;                \
                                                                              \
 public:                                                                      \
  void SetPassagesStoredCallbackForTesting

#include <components/history_embeddings/content/history_embeddings_service.h>  // IWYU pragma: export

#undef SetPassagesStoredCallbackForTesting
#undef OnPassageVisibilityCalculated

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_HISTORY_EMBEDDINGS_CONTENT_HISTORY_EMBEDDINGS_SERVICE_H_
