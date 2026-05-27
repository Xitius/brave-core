/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <components/history_embeddings/core/sql_database_unittest.cc>

namespace history_embeddings {

TEST_F(HistoryEmbeddingsSqlDatabaseTest,
       MakeUrlDataIteratorWithUrlIdFilterReturnsOnlyMatching) {
  auto sql_database = MakeDatabase();
  sql_database->SetEmbedderMetadata({kEmbeddingsVersion, kEmbeddingsSize},
                                    GetEncryptorInstance());
  AddBasicMockData(sql_database.get());

  base::flat_set<history::URLID> filter = {2};
  auto iterator = sql_database->MakeUrlDataIterator({}, std::move(filter));
  ASSERT_TRUE(iterator);

  std::vector<history::URLID> yielded;
  while (const UrlData* data = iterator->Next()) {
    yielded.push_back(data->url_id);
  }
  EXPECT_THAT(yielded, testing::ElementsAre(2));
}

TEST_F(HistoryEmbeddingsSqlDatabaseTest,
       MakeUrlDataIteratorWithUrlIdFilterSkipsUnknownIds) {
  auto sql_database = MakeDatabase();
  sql_database->SetEmbedderMetadata({kEmbeddingsVersion, kEmbeddingsSize},
                                    GetEncryptorInstance());
  AddBasicMockData(sql_database.get());

  base::flat_set<history::URLID> filter = {1, 999};
  auto iterator = sql_database->MakeUrlDataIterator({}, std::move(filter));
  ASSERT_TRUE(iterator);

  std::vector<history::URLID> yielded;
  while (const UrlData* data = iterator->Next()) {
    yielded.push_back(data->url_id);
  }
  EXPECT_THAT(yielded, testing::ElementsAre(1));
}

TEST_F(HistoryEmbeddingsSqlDatabaseTest,
       MakeUrlDataIteratorWithEmptyUrlIdFilterReturnsNull) {
  auto sql_database = MakeDatabase();
  sql_database->SetEmbedderMetadata({kEmbeddingsVersion, kEmbeddingsSize},
                                    GetEncryptorInstance());
  AddBasicMockData(sql_database.get());

  EXPECT_FALSE(sql_database->MakeUrlDataIterator({}, {}));
}

TEST_F(HistoryEmbeddingsSqlDatabaseTest,
       MakeUrlDataIteratorWithUrlIdFilterAppliesTimeRange) {
  auto sql_database = MakeDatabase();
  sql_database->SetEmbedderMetadata({kEmbeddingsVersion, kEmbeddingsSize},
                                    GetEncryptorInstance());

  const base::Time t_old = base::Time::Now() - base::Days(2);
  const base::Time t_new = base::Time::Now();
  {
    UrlData url_data_old(1, 10, t_old);
    url_data_old.passages.add_passages("old");
    url_data_old.passage_embeddings.push_back(FakeEmbedding());
    ASSERT_TRUE(sql_database->AddUrlData(url_data_old));
  }
  {
    UrlData url_data_new(2, 11, t_new);
    url_data_new.passages.add_passages("new");
    url_data_new.passage_embeddings.push_back(FakeEmbedding());
    ASSERT_TRUE(sql_database->AddUrlData(url_data_new));
  }

  base::flat_set<history::URLID> filter = {1, 2};
  auto iterator =
      sql_database->MakeUrlDataIterator(t_new - base::Days(1), filter);
  ASSERT_TRUE(iterator);

  std::vector<history::URLID> yielded;
  while (const UrlData* data = iterator->Next()) {
    yielded.push_back(data->url_id);
  }
  EXPECT_THAT(yielded, testing::ElementsAre(2));
}

}  // namespace history_embeddings
