/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// Inject a Brave-only message handler and two loadTimeData booleans into the
// upstream chrome://history WebUI so the side bar can render a Semantic
// History Search toggle. Hooked via macro substitution of the one-and-only
// ManagedUIHandler::Initialize() call in the upstream constructor.

#include <memory>

#include "base/functional/bind.h"
#include "base/memory/raw_ptr.h"
#include "base/values.h"
#include "brave/components/local_ai/core/pref_names.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/history_embeddings/history_embeddings_utils.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/webui/managed_ui_handler.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/browser/web_ui_message_handler.h"
#include "ui/base/webui/web_ui_util.h"

namespace {

class BraveHistoryEmbeddingsToggleHandler
    : public content::WebUIMessageHandler {
 public:
  explicit BraveHistoryEmbeddingsToggleHandler(Profile* profile)
      : profile_(profile) {}

  BraveHistoryEmbeddingsToggleHandler(
      const BraveHistoryEmbeddingsToggleHandler&) = delete;
  BraveHistoryEmbeddingsToggleHandler& operator=(
      const BraveHistoryEmbeddingsToggleHandler&) = delete;

  ~BraveHistoryEmbeddingsToggleHandler() override = default;

  void RegisterMessages() override {
    web_ui()->RegisterMessageCallback(
        "setHistoryEmbeddingsEnabled",
        base::BindRepeating(&BraveHistoryEmbeddingsToggleHandler::
                                HandleSetHistoryEmbeddingsEnabled,
                            base::Unretained(this)));
  }

 private:
  void HandleSetHistoryEmbeddingsEnabled(const base::ListValue& args) {
    if (args.size() != 1 || !args[0].is_bool()) {
      return;
    }
    profile_->GetPrefs()->SetBoolean(
        local_ai::prefs::kBraveHistoryEmbeddingsEnabled, args[0].GetBool());
  }

  raw_ptr<Profile> profile_;
};

class BraveHistoryUIInitializer {
 public:
  static void Initialize(content::WebUI* web_ui,
                         content::WebUIDataSource* source) {
    ManagedUIHandler::Initialize(web_ui, source);

    Profile* profile = Profile::FromWebUI(web_ui);
    source->AddBoolean("isHistoryEmbeddingsFeatureEnabled",
                       history_embeddings::IsHistoryEmbeddingsFeatureEnabled());
    source->AddBoolean("isHistoryEmbeddingsEnabled",
                       profile->GetPrefs()->GetBoolean(
                           local_ai::prefs::kBraveHistoryEmbeddingsEnabled));

    static constexpr webui::LocalizedString kBraveHistoryEmbeddingsStrings[] = {
        {"braveHistoryEmbeddingsToggleLabel",
         IDS_BRAVE_HISTORY_EMBEDDINGS_TOGGLE_LABEL},
        {"braveHistoryEmbeddingsToggleDescription",
         IDS_BRAVE_HISTORY_EMBEDDINGS_TOGGLE_DESCRIPTION},
    };
    source->AddLocalizedStrings(kBraveHistoryEmbeddingsStrings);

    web_ui->AddMessageHandler(
        std::make_unique<BraveHistoryEmbeddingsToggleHandler>(profile));
  }
};

}  // namespace

#define ManagedUIHandler BraveHistoryUIInitializer

#include <chrome/browser/ui/webui/history/history_ui.cc>

#undef ManagedUIHandler
