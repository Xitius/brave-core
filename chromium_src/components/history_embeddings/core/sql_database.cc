/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <components/history_embeddings/core/sql_database.cc>

namespace history_embeddings {

// Iterates `url_id_filter` via per-id `GetUrlData` point lookups. The set is
// bounded by the caller's open-tab count, so N indexed lookups on the
// `url_id` primary key are cheap.
std::unique_ptr<VectorDatabase::UrlDataIterator>
SqlDatabase::MakeUrlDataIterator(std::optional<base::Time> time_range_start,
                                 base::flat_set<history::URLID> url_id_filter) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (url_id_filter.empty()) {
    return nullptr;
  }

  struct FilteredIterator : public UrlDataIterator {
    FilteredIterator(base::WeakPtr<SqlDatabase> db,
                     std::vector<history::URLID> ids,
                     std::optional<base::Time> time_range_start)
        : db(std::move(db)),
          ids(std::move(ids)),
          time_range_start(time_range_start) {}

    const UrlData* Next() override {
      while (db && index < ids.size()) {
        auto data = db->GetUrlData(ids[index++]);
        if (!data.has_value() || data->passage_embeddings.empty()) {
          continue;
        }
        if (time_range_start.has_value() &&
            data->visit_time < *time_range_start) {
          continue;
        }
        current = std::move(*data);
        return &current;
      }
      return nullptr;
    }

    base::WeakPtr<SqlDatabase> db;
    std::vector<history::URLID> ids;
    std::optional<base::Time> time_range_start;
    size_t index = 0;
    UrlData current{0, 0, base::Time()};
  };

  return std::make_unique<FilteredIterator>(
      weak_ptr_factory_.GetWeakPtr(),
      std::vector<history::URLID>(url_id_filter.begin(), url_id_filter.end()),
      time_range_start);
}

}  // namespace history_embeddings
