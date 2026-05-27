/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_HISTORY_EMBEDDINGS_CORE_SQL_DATABASE_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_HISTORY_EMBEDDINGS_CORE_SQL_DATABASE_H_

#include "base/containers/flat_set.h"

// Inject a `MakeUrlDataIterator` override on `SqlDatabase` that takes a
// `url_id_filter` (defined in our chromium_src `sql_database.cc`).
// `DeleteDataForUrlId_Unused` absorbs the leading `bool` return type of the
// original declaration.
#define DeleteDataForUrlId                                    \
  DeleteDataForUrlId_Unused();                                \
  std::unique_ptr<UrlDataIterator> MakeUrlDataIterator(       \
      std::optional<base::Time> time_range_start,             \
      base::flat_set<history::URLID> url_id_filter) override; \
  bool DeleteDataForUrlId

#include <components/history_embeddings/core/sql_database.h>  // IWYU pragma: export

#undef DeleteDataForUrlId

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_HISTORY_EMBEDDINGS_CORE_SQL_DATABASE_H_
