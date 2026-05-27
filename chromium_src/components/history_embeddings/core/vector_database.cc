/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <components/history_embeddings/core/vector_database.cc>

namespace history_embeddings {

// Default body for the `MakeUrlDataIterator` overload declared via macro
// injection in our chromium_src `vector_database.h`. Ignores the filter and
// delegates to the time-range-only overload; `SqlDatabase` overrides this to
// apply the filter at the storage layer.
std::unique_ptr<VectorDatabase::UrlDataIterator>
VectorDatabase::MakeUrlDataIterator(
    std::optional<base::Time> time_range_start,
    base::flat_set<history::URLID> /*url_id_filter*/) {
  return MakeUrlDataIterator(time_range_start);
}

}  // namespace history_embeddings
