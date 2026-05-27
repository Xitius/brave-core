/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/polkadot/polkadot_chain_metadata.h"

#include <vector>

#include "base/base_paths.h"
#include "base/path_service.h"
#include "base/test/values_test_util.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

namespace {

constexpr char kResult[] = "result";

}  // namespace

TEST(PolkadotChainMetadataUnitTest, FromFields) {
  auto metadata = PolkadotChainMetadata::FromFields(
      /*system_pallet_index=*/0, /*balances_pallet_index=*/7,
      /*transaction_payment_pallet_index=*/0x20,
      /*transfer_allow_death_call_index=*/2,
      /*transfer_keep_alive_call_index=*/4,
      /*transfer_all_call_index=*/5,
      /*ss58_prefix=*/42, /*spec_version=*/1'234'567,
      /*asset_tx_payment=*/true);

  EXPECT_EQ(metadata.GetSystemPalletIndex(), 0u);
  EXPECT_EQ(metadata.GetBalancesPalletIndex(), 7u);
  EXPECT_EQ(metadata.GetTransactionPaymentPalletIndex(), 0x20u);
  EXPECT_EQ(metadata.GetTransferAllowDeathCallIndex(), 2u);
  EXPECT_EQ(metadata.GetTransferKeepAliveCallIndex(), 4u);
  EXPECT_EQ(metadata.GetTransferAllCallIndex(), 5u);
  EXPECT_EQ(metadata.GetSs58Prefix(), 42u);
  EXPECT_EQ(metadata.GetSpecVersion(), 1'234'567u);
  EXPECT_TRUE(metadata.UsesAssetTxPayment());
}

TEST(PolkadotChainMetadataUnitTest, EqualityOperator) {
  auto metadata_a = PolkadotChainMetadata::FromFields(
      /*system_pallet_index=*/0, /*balances_pallet_index=*/7,
      /*transaction_payment_pallet_index=*/0x20,
      /*transfer_allow_death_call_index=*/2,
      /*transfer_keep_alive_call_index=*/4,
      /*transfer_all_call_index=*/5,
      /*ss58_prefix=*/42, /*spec_version=*/1'234'567,
      /*asset_tx_payment=*/false);
  auto metadata_b = PolkadotChainMetadata::FromFields(
      /*system_pallet_index=*/0, /*balances_pallet_index=*/7,
      /*transaction_payment_pallet_index=*/0x20,
      /*transfer_allow_death_call_index=*/2,
      /*transfer_keep_alive_call_index=*/4,
      /*transfer_all_call_index=*/5,
      /*ss58_prefix=*/42, /*spec_version=*/1'234'567,
      /*asset_tx_payment=*/false);
  auto metadata_c = PolkadotChainMetadata::FromFields(
      /*system_pallet_index=*/0, /*balances_pallet_index=*/7,
      /*transaction_payment_pallet_index=*/0x20,
      /*transfer_allow_death_call_index=*/2,
      /*transfer_keep_alive_call_index=*/4,
      /*transfer_all_call_index=*/5,
      /*ss58_prefix=*/42, /*spec_version=*/1'234'568,
      /*asset_tx_payment=*/false);
  auto metadata_d = PolkadotChainMetadata::FromFields(
      /*system_pallet_index=*/0, /*balances_pallet_index=*/7,
      /*transaction_payment_pallet_index=*/0x20,
      /*transfer_allow_death_call_index=*/2,
      /*transfer_keep_alive_call_index=*/4,
      /*transfer_all_call_index=*/5,
      /*ss58_prefix=*/42, /*spec_version=*/1'234'567,
      /*asset_tx_payment=*/true);

  EXPECT_EQ(metadata_a, metadata_b);
  EXPECT_NE(metadata_a, metadata_c);
  EXPECT_NE(metadata_a, metadata_d);
}

TEST(PolkadotChainMetadataUnitTest, FromChainName) {
  auto expected_westend = PolkadotChainMetadata::FromFields(
      /*system_pallet_index=*/0, /*balances_pallet_index=*/4,
      /*transaction_payment_pallet_index=*/0x1a,
      /*transfer_allow_death_call_index=*/0,
      /*transfer_keep_alive_call_index=*/3,
      /*transfer_all_call_index=*/4,
      /*ss58_prefix=*/42, /*spec_version=*/0,
      /*asset_tx_payment=*/false);
  auto westend = PolkadotChainMetadata::FromChainName("Westend");
  ASSERT_TRUE(westend);

  EXPECT_EQ(*westend, expected_westend);

  auto expected_westend_asset_hub = PolkadotChainMetadata::FromFields(
      /*system_pallet_index=*/0, /*balances_pallet_index=*/10,
      /*transaction_payment_pallet_index=*/0x0b,
      /*transfer_allow_death_call_index=*/0,
      /*transfer_keep_alive_call_index=*/3,
      /*transfer_all_call_index=*/4,
      /*ss58_prefix=*/42, /*spec_version=*/0,
      /*asset_tx_payment=*/true);
  auto westend_asset_hub =
      PolkadotChainMetadata::FromChainName("Westend Asset Hub");
  ASSERT_TRUE(westend_asset_hub);
  EXPECT_EQ(*westend_asset_hub, expected_westend_asset_hub);

  auto expected_polkadot = PolkadotChainMetadata::FromFields(
      /*system_pallet_index=*/0, /*balances_pallet_index=*/5,
      /*transaction_payment_pallet_index=*/0x20,
      /*transfer_allow_death_call_index=*/0,
      /*transfer_keep_alive_call_index=*/3,
      /*transfer_all_call_index=*/4,
      /*ss58_prefix=*/0, /*spec_version=*/0,
      /*asset_tx_payment=*/false);
  auto polkadot = PolkadotChainMetadata::FromChainName("Polkadot");
  ASSERT_TRUE(polkadot);
  EXPECT_EQ(*polkadot, expected_polkadot);

  auto expected_polkadot_asset_hub = PolkadotChainMetadata::FromFields(
      /*system_pallet_index=*/0, /*balances_pallet_index=*/10,
      /*transaction_payment_pallet_index=*/0x0b,
      /*transfer_allow_death_call_index=*/0,
      /*transfer_keep_alive_call_index=*/3,
      /*transfer_all_call_index=*/4,
      /*ss58_prefix=*/0, /*spec_version=*/0,
      /*asset_tx_payment=*/true);
  auto polkadot_asset_hub =
      PolkadotChainMetadata::FromChainName("Polkadot Asset Hub");
  ASSERT_TRUE(polkadot_asset_hub);
  EXPECT_EQ(*polkadot_asset_hub, expected_polkadot_asset_hub);

  EXPECT_FALSE(PolkadotChainMetadata::FromChainName(""));
  EXPECT_FALSE(PolkadotChainMetadata::FromChainName("Unknown Chain"));
}

TEST(PolkadotChainMetadataUnitTest, FromBytesInvalid) {
  // Invalid metadata magic/version payload.
  std::vector<uint8_t> invalid_magic;
  ASSERT_TRUE(PrefixedHexStringToBytes("0x6d65746164617461", &invalid_magic));
  EXPECT_FALSE(PolkadotChainMetadata::FromBytes(invalid_magic));

  std::vector<uint8_t> invalid_short;
  ASSERT_TRUE(PrefixedHexStringToBytes("0xdeadbeef", &invalid_short));
  EXPECT_FALSE(PolkadotChainMetadata::FromBytes(invalid_short));
}

TEST(PolkadotChainMetadataUnitTest, ParseRealStateGetMetadataResponsePolkadot) {
  // Refreshed with:
  // curl -sS -H 'Content-Type: application/json' \
  //   -d '{"id":1,"jsonrpc":"2.0","method":"state_getMetadata","params":[]}' \
  //   https://rpc.polkadot.io/
  const auto fixture_path =
      base::PathService::CheckedGet(base::DIR_SRC_TEST_DATA_ROOT)
          .AppendASCII("brave")
          .AppendASCII("components")
          .AppendASCII("test")
          .AppendASCII("data")
          .AppendASCII("brave_wallet")
          .AppendASCII("polkadot")
          .AppendASCII("chain_metadata")
          .AppendASCII("state_getMetadata_polkadot.json");
  const base::DictValue json = base::test::ParseJsonDictFromFile(fixture_path);

  const std::string* metadata_hex = json.FindString(kResult);
  ASSERT_TRUE(metadata_hex);
  ASSERT_FALSE(metadata_hex->empty());

  std::vector<uint8_t> metadata_bytes;
  ASSERT_TRUE(PrefixedHexStringToBytes(*metadata_hex, &metadata_bytes));

  auto metadata = PolkadotChainMetadata::FromBytes(metadata_bytes);
  ASSERT_TRUE(metadata);
  auto expected = PolkadotChainMetadata::FromFields(
      /*system_pallet_index=*/0, /*balances_pallet_index=*/5,
      /*transaction_payment_pallet_index=*/0x20,
      /*transfer_allow_death_call_index=*/0,
      /*transfer_keep_alive_call_index=*/3,
      /*transfer_all_call_index=*/4,
      /*ss58_prefix=*/0, /*spec_version=*/2'000'007,
      /*asset_tx_payment=*/false);
  EXPECT_EQ(*metadata, expected);

  auto metadata2 = PolkadotChainMetadata::FromChainName("Polkadot");
  ASSERT_TRUE(metadata2);
  auto expected_from_name = PolkadotChainMetadata::FromFields(
      /*system_pallet_index=*/0, /*balances_pallet_index=*/5,
      /*transaction_payment_pallet_index=*/0x20,
      /*transfer_allow_death_call_index=*/0,
      /*transfer_keep_alive_call_index=*/3,
      /*transfer_all_call_index=*/4,
      /*ss58_prefix=*/0, /*spec_version=*/0,
      /*asset_tx_payment=*/false);
  EXPECT_EQ(*metadata2, expected_from_name);
}

TEST(PolkadotChainMetadataUnitTest,
     ParseRealStateGetMetadataResponseAssetHubPolkadot) {
  // Refreshed with:
  // curl -sS -H 'Content-Type: application/json' \
  //   -d '{"id":1,"jsonrpc":"2.0","method":"state_getMetadata","params":[]}' \
  //   https://polkadot-asset-hub-rpc.polkadot.io
  const auto fixture_path =
      base::PathService::CheckedGet(base::DIR_SRC_TEST_DATA_ROOT)
          .AppendASCII("brave")
          .AppendASCII("components")
          .AppendASCII("test")
          .AppendASCII("data")
          .AppendASCII("brave_wallet")
          .AppendASCII("polkadot")
          .AppendASCII("chain_metadata")
          .AppendASCII("state_getMetadata_assethub_polkadot.json");
  const base::DictValue json = base::test::ParseJsonDictFromFile(fixture_path);

  const std::string* metadata_hex = json.FindString(kResult);
  ASSERT_TRUE(metadata_hex);
  ASSERT_FALSE(metadata_hex->empty());

  std::vector<uint8_t> metadata_bytes;
  ASSERT_TRUE(PrefixedHexStringToBytes(*metadata_hex, &metadata_bytes));

  auto metadata = PolkadotChainMetadata::FromBytes(metadata_bytes);
  ASSERT_TRUE(metadata);
  auto expected = PolkadotChainMetadata::FromFields(
      /*system_pallet_index=*/0, /*balances_pallet_index=*/0x0a,
      /*transaction_payment_pallet_index=*/0x0b,
      /*transfer_allow_death_call_index=*/0,
      /*transfer_keep_alive_call_index=*/3,
      /*transfer_all_call_index=*/4,
      /*ss58_prefix=*/0, /*spec_version=*/2'002'001,
      /*asset_tx_payment=*/true);
  EXPECT_EQ(*metadata, expected);
}

TEST(PolkadotChainMetadataUnitTest, ParseRealStateGetMetadataResponseWestend) {
  // Refreshed with:
  // curl -sS -H 'Content-Type: application/json' \
  //   -d '{"id":1,"jsonrpc":"2.0","method":"state_getMetadata","params":[]}' \
  //   https://westend-rpc.polkadot.io/
  const auto fixture_path =
      base::PathService::CheckedGet(base::DIR_SRC_TEST_DATA_ROOT)
          .AppendASCII("brave")
          .AppendASCII("components")
          .AppendASCII("test")
          .AppendASCII("data")
          .AppendASCII("brave_wallet")
          .AppendASCII("polkadot")
          .AppendASCII("chain_metadata")
          .AppendASCII("state_getMetadata_westend.json");
  const base::DictValue json = base::test::ParseJsonDictFromFile(fixture_path);

  const std::string* metadata_hex = json.FindString(kResult);
  ASSERT_TRUE(metadata_hex);
  ASSERT_FALSE(metadata_hex->empty());

  std::vector<uint8_t> metadata_bytes;
  ASSERT_TRUE(PrefixedHexStringToBytes(*metadata_hex, &metadata_bytes));

  auto metadata = PolkadotChainMetadata::FromBytes(metadata_bytes);
  ASSERT_TRUE(metadata);
  auto expected = PolkadotChainMetadata::FromFields(
      /*system_pallet_index=*/0, /*balances_pallet_index=*/4,
      /*transaction_payment_pallet_index=*/0x1a,
      /*transfer_allow_death_call_index=*/0,
      /*transfer_keep_alive_call_index=*/3,
      /*transfer_all_call_index=*/4,
      /*ss58_prefix=*/42, /*spec_version=*/1'022'000,
      /*asset_tx_payment=*/false);
  EXPECT_EQ(*metadata, expected);
}

TEST(PolkadotChainMetadataUnitTest,
     ParseRealStateGetMetadataResponseAssetHubWestend) {
  // Refreshed with:
  // curl -sS -H 'Content-Type: application/json' \
  //   -d '{"id":1,"jsonrpc":"2.0","method":"state_getMetadata","params":[]}' \
  //   https://westend-asset-hub-rpc.polkadot.io/
  const auto fixture_path =
      base::PathService::CheckedGet(base::DIR_SRC_TEST_DATA_ROOT)
          .AppendASCII("brave")
          .AppendASCII("components")
          .AppendASCII("test")
          .AppendASCII("data")
          .AppendASCII("brave_wallet")
          .AppendASCII("polkadot")
          .AppendASCII("chain_metadata")
          .AppendASCII("state_getMetadata_assethub_westend.json");
  const base::DictValue json = base::test::ParseJsonDictFromFile(fixture_path);

  const std::string* metadata_hex = json.FindString(kResult);
  ASSERT_TRUE(metadata_hex);
  ASSERT_FALSE(metadata_hex->empty());

  std::vector<uint8_t> metadata_bytes;
  ASSERT_TRUE(PrefixedHexStringToBytes(*metadata_hex, &metadata_bytes));

  auto metadata = PolkadotChainMetadata::FromBytes(metadata_bytes);
  ASSERT_TRUE(metadata);

  auto expected = PolkadotChainMetadata::FromFields(
      /*system_pallet_index=*/0, /*balances_pallet_index=*/0x0a,
      /*transaction_payment_pallet_index=*/0x0b,
      /*transfer_allow_death_call_index=*/0,
      /*transfer_keep_alive_call_index=*/3,
      /*transfer_all_call_index=*/4,
      /*ss58_prefix=*/42, /*spec_version=*/1'022'005,
      /*asset_tx_payment=*/true);
  EXPECT_EQ(*metadata, expected);
}

TEST(PolkadotChainMetadataUnitTest, Security_V14NoStorage_ParsesCorrectly) {
  std::vector<uint8_t> bytes;
  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0x6d6574610e080000000000000400000104507472616e736665725f616c6c6f"
      "775f6465617468000000000c1853797374656d00000008285353353850726566"
      "697800082a00001c56657273696f6e005820706f6c6b61646f74106e6f646501"
      "000000640000000000002042616c616e63657300010400000005485472616e73"
      "616374696f6e5061796d656e74000000000003",
      &bytes));
  auto metadata = PolkadotChainMetadata::FromBytes(bytes);
  ASSERT_TRUE(metadata);
  EXPECT_EQ(metadata->GetSs58Prefix(), 42u);
  EXPECT_EQ(metadata->GetBalancesPalletIndex(), 5u);
  EXPECT_EQ(metadata->GetSpecVersion(), 100u);
}

TEST(PolkadotChainMetadataUnitTest,
     Security_V14MapStorageHasher0_ParsesCorrectly) {
  std::vector<uint8_t> bytes;
  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0x6d6574610e080000000000000400000104507472616e736665725f616c6c6f"
      "775f6465617468000000000c1853797374656d011853797374656d041c416363"
      "6f756e7400010000000000000008285353353850726566697800082a00001c56"
      "657273696f6e005820706f6c6b61646f74106e6f646501000000640000000000"
      "002042616c616e63657300010400000005485472616e73616374696f6e506179"
      "6d656e74000000000003",
      &bytes));
  auto metadata = PolkadotChainMetadata::FromBytes(bytes);
  ASSERT_TRUE(metadata);
  EXPECT_EQ(metadata->GetSs58Prefix(), 42u);
  EXPECT_EQ(metadata->GetBalancesPalletIndex(), 5u);
}

TEST(PolkadotChainMetadataUnitTest,
     Security_V14MapStorageHasher1_ParsesCorrectly) {
  std::vector<uint8_t> bytes;
  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0x6d6574610e080000000000000400000104507472616e736665725f616c6c6f"
      "775f6465617468000000000c1853797374656d011853797374656d041c416363"
      "6f756e7400010100000000000008285353353850726566697800082a00001c56"
      "657273696f6e005820706f6c6b61646f74106e6f646501000000640000000000"
      "002042616c616e63657300010400000005485472616e73616374696f6e506179"
      "6d656e74000000000003",
      &bytes));
  auto metadata = PolkadotChainMetadata::FromBytes(bytes);
  ASSERT_TRUE(metadata);
  EXPECT_EQ(metadata->GetSs58Prefix(), 42u);
  EXPECT_EQ(metadata->GetBalancesPalletIndex(), 5u);
}

TEST(PolkadotChainMetadataUnitTest,
     Security_V15MapStorageHasher1_ParsesCorrectly) {
  std::vector<uint8_t> bytes;
  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0x6d6574610f080000000000000400000104507472616e736665725f616c6c6f"
      "775f6465617468000000000c1853797374656d011853797374656d041c416363"
      "6f756e740001040100000000000008285353353850726566697800082a00001c"
      "56657273696f6e005820706f6c6b61646f74106e6f6465010000006400000000"
      "0000002042616c616e6365730001040000000500485472616e73616374696f6e"
      "5061796d656e7400000000000300",
      &bytes));
  auto metadata = PolkadotChainMetadata::FromBytes(bytes);
  ASSERT_TRUE(metadata);
  EXPECT_EQ(metadata->GetSs58Prefix(), 42u);
  EXPECT_EQ(metadata->GetBalancesPalletIndex(), 5u);
}

TEST(PolkadotChainMetadataUnitTest,
     Security_SS58PrefixU16_ReturnsCorrectValue) {
  std::vector<uint8_t> bytes;
  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0x6d6574610f080000000000000400000104507472616e736665725f616c6c6f"
      "775f6465617468000000000c1853797374656d00000008285353353850726566"
      "697800082a00001c56657273696f6e005820706f6c6b61646f74106e6f646501"
      "00000064000000000000002042616c616e636573000104000000050048547261"
      "6e73616374696f6e5061796d656e7400000000000300",
      &bytes));
  auto metadata = PolkadotChainMetadata::FromBytes(bytes);
  ASSERT_TRUE(metadata);
  EXPECT_EQ(metadata->GetSs58Prefix(), 42u);
}

TEST(PolkadotChainMetadataUnitTest,
     Security_SS58PrefixCompact_ReturnsCorrectValue) {
  std::vector<uint8_t> bytes;
  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0x6d6574610f080000000000000400000104507472616e736665725f616c6c6f"
      "775f6465617468000000000c1853797374656d00000008285353353850726566"
      "69780004a8001c56657273696f6e005820706f6c6b61646f74106e6f64650100"
      "000064000000000000002042616c616e6365730001040000000500485472616e"
      "73616374696f6e5061796d656e7400000000000300",
      &bytes));
  auto metadata = PolkadotChainMetadata::FromBytes(bytes);
  ASSERT_TRUE(metadata);
  EXPECT_EQ(metadata->GetSs58Prefix(), 42u);
}

TEST(PolkadotChainMetadataUnitTest,
     Security_TrailingBytesNotRejected_KnownLimitation) {
  std::vector<uint8_t> bytes;
  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0x6d6574610f080000000000000400000104507472616e736665725f616c6c6f"
      "775f6465617468000000000c1853797374656d00000008285353353850726566"
      "697800082a00001c56657273696f6e005820706f6c6b61646f74106e6f646501"
      "00000064000000000000002042616c616e636573000104000000050048547261"
      "6e73616374696f6e5061796d656e7400000000000300deadbeef",
      &bytes));
  auto metadata = PolkadotChainMetadata::FromBytes(bytes);
  // Known limitation: trailing bytes are silently accepted. Cannot add a
  // trailing-bytes check because real metadata has fields after the pallets
  // section that this partial parser doesn't consume.
  ASSERT_TRUE(metadata);
  EXPECT_EQ(metadata->GetSs58Prefix(), 42u);
  EXPECT_EQ(metadata->GetBalancesPalletIndex(), 5u);
}

TEST(PolkadotChainMetadataUnitTest,
     Security_HugeVecLength_RejectedByBoundsCheck) {
  std::vector<uint8_t> bytes;
  ASSERT_TRUE(PrefixedHexStringToBytes("0x6d6574610f420d0300", &bytes));
  EXPECT_FALSE(PolkadotChainMetadata::FromBytes(bytes));
}

}  // namespace brave_wallet
