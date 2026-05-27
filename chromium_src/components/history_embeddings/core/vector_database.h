/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_HISTORY_EMBEDDINGS_CORE_VECTOR_DATABASE_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_HISTORY_EMBEDDINGS_CORE_VECTOR_DATABASE_H_

#include "base/containers/flat_set.h"

// Inject a `url_id_filter` field into the upstream `SearchParams` struct so
// `HistoryEmbeddingsService::SearchWithUrlIdFilter` can carry an open-tab
// URL-id allowlist into `VectorDatabase::FindNearest`. The patched
// `FindNearest` forwards the field to `MakeUrlDataIterator`, where overrides
// (`SqlDatabase`, `VectorDatabaseInMemory`) apply the filter.
#define skip_answering                          \
  brave_unused_skip_answering = false;          \
  base::flat_set<history::URLID> url_id_filter; \
  bool skip_answering

// Inject a new `MakeUrlDataIterator` overload on `VectorDatabase` that takes
// a `url_id_filter`. Default body (defined in our chromium_src
// `vector_database.cc`) delegates to the unfiltered overload; `SqlDatabase`
// overrides this to apply the filter at the storage layer.
#define FindNearest                                             \
  FindNearest_Unused();                                         \
                                                                \
 public:                                                        \
  virtual std::unique_ptr<UrlDataIterator> MakeUrlDataIterator( \
      std::optional<base::Time> time_range_start,               \
      base::flat_set<history::URLID> url_id_filter);            \
  SearchInfo FindNearest

#include <components/history_embeddings/core/vector_database.h>  // IWYU pragma: export

#undef FindNearest
#undef skip_answering

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_HISTORY_EMBEDDINGS_CORE_VECTOR_DATABASE_H_
