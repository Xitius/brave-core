#!/usr/bin/env vpython3
# Copyright (c) 2023 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
r"""A tool run telemetry perftests and report the results to dashboard

Use npm run perf_tests to call this script.

The tool:
1. Downloads browser binaries.
2. Runs the set of telemetry perftests from the provided config.
3. Reports the results to brave-perf-dashboard.appspot.com or stores them as
   local .html files.

The tool gives more stable results on prepared hardware/OS to minimize jitter.

"""
import argparse
import logging
import shutil
import subprocess
import sys
import os
import platform
import tempfile
from typing import List, Optional

import components.android_tools as android_tools
import components.perf_config as perf_config
import components.perf_test_runner as perf_test_runner
import components.perf_test_utils as perf_test_utils
import components.path_util as path_util
import components.profile_tools as profile_tools
import components.wpr_utils as wpr_utils

from components.common_options import CommonOptions, PerfMode


with path_util.SysPath(path_util.GetPyJson5Dir()):
  # pylint: disable=import-error # pytype: disable=import-error
  import json5
  # pylint: enable=import-error # pytype: enable=import-error


# Override GOPROXY/GOROOT for the WPR `go build` invoked by telemetry: the
# Windows perf workers persist bad values in their Go env file (empty GOPROXY,
# and GOROOT=C:\Go which doesn't match the bundled toolchain). Without these
# overrides module fetches fail with "GOPROXY list is not the empty string, but
# contains no entries" and stdlib lookups fail with "package log/slog is not in
# std (C:\Go\src\log\slog)".
if sys.platform == 'win32':
  os.environ['GOPROXY'] = 'https://proxy.golang.org,direct'
  os.environ['GOROOT'] = os.path.join(path_util.GetSrcDir(), 'third_party',
                                      'webpagereplay', 'third_party', 'golang',
                                      'win', 'x64')


def _log_go_env() -> None:
  for var in ('GOPROXY', 'GOFLAGS', 'GOSUMDB', 'GONOSUMCHECK', 'GOPATH',
              'GOMODCACHE'):
    logging.info('os.environ[%s] = %r', var, os.environ.get(var))

  if sys.platform == 'win32':
    os_name, exe = 'win', 'go.exe'
  elif sys.platform == 'darwin':
    os_name, exe = 'mac', 'go'
  else:
    os_name, exe = 'linux', 'go'

  arch = platform.machine().lower()
  if arch in ("arm64", "aarch64"):
    arch = 'arm64'
  else:
    arch = 'x64'

  go_exe = os.path.join(path_util.GetSrcDir(), 'third_party', 'webpagereplay',
                        'third_party', 'golang', os_name, arch, 'bin', exe)
  try:
    out = subprocess.check_output([
        go_exe, 'env', 'GOPROXY', 'GOFLAGS', 'GOSUMDB', 'GONOSUMCHECK',
        'GOPATH', 'GOMODCACHE'
    ],
                                  text=True,
                                  stderr=subprocess.STDOUT)
    logging.info('go env:\n%s', out)
  except (OSError, subprocess.CalledProcessError) as e:
    logging.info('go env failed: %s', e)


def load_config(options: CommonOptions) -> dict:
  config = options.config
  if config.startswith('https://'):  # URL to download the config
    _, config_path = tempfile.mkstemp(dir=options.working_directory,
                                      prefix='config-')
    perf_test_utils.DownloadFile(config, config_path)

  elif os.path.isfile(config):  # Full config path
    config_path = config
  else:  # config is a relative path
    config_path = os.path.join(path_util.GetBravePerfConfigDir(), config)
    if not os.path.isfile(config_path):
      raise RuntimeError(f'Can\'t find matching config {config}')

  with open(config_path, 'r', encoding='utf-8') as config_file:
    return json5.load(config_file)


def main():
  parser = argparse.ArgumentParser(
      formatter_class=argparse.RawTextHelpFormatter,
      description='A tool to run perf tests and report the results.'
      'Use npm run perf_tests to launch it',
      epilog=R'''
To some launch tests locally:
npm run perf_tests -- compare/compare_with_on_off_feature.json5

On CI:
npm run perf_tests -- smoke-brave.json5 v1.58.45
     --working-directory=e:\work\brave\src\out\100
     --ci-mode
''')
  CommonOptions.add_parser_args(parser)

  args = parser.parse_args()
  options = CommonOptions.from_args(args)

  log_level = logging.DEBUG if options.verbose else logging.INFO
  log_format = '%(asctime)s: %(message)s'
  logging.basicConfig(level=log_level, format=log_format)

  os.environ['GOPROXY'] = 'https://proxy.golang.org,direct'
  _log_go_env()

  logging.info('Use working directory %s', options.working_directory)
  if options.ci_mode and os.path.exists(options.working_directory):
    logging.info('Cleaning working directory %s', options.working_directory)
    shutil.rmtree(options.working_directory)

  os.makedirs(options.working_directory, exist_ok=True)

  json_config = load_config(options)
  config = perf_config.PerfConfig(json_config)

  if options.is_android:
    if options.reboot_android:
      android_tools.RebootAndroid()
    android_tools.SetupAndroidDevice()

  if options.mode == PerfMode.RUN:
    if len(config.runners) != 1:
      raise RuntimeError('Only one configuration should be specified.')

    configurations = perf_test_runner.SpawnConfigurationsFromTargetList(
        options.targets, config.runners[0])
    return 0 if perf_test_runner.RunConfigurations(
        configurations, config.benchmarks, options) else 1

  if options.mode == PerfMode.COMPARE:
    return 0 if perf_test_runner.RunConfigurations(
        config.runners, config.benchmarks, options) else 1

  if options.mode == PerfMode.UPDATE_PROFILE:
    if options.chromium:
      return 0  # A build with !options.chromium will update the both profiles
    args.chromium = False
    options = CommonOptions.from_args(args)
    brave_config = perf_config.PerfConfig(load_config(options))
    brave_config.runners[0].label = 'brave-rebase'

    args.chromium = True
    options = CommonOptions.from_args(args)
    chromium_config = perf_config.PerfConfig(load_config(options))
    chromium_config.runners[0].label = 'chromium-rebase'

    return 0 if profile_tools.RunUpdateProfile(brave_config, chromium_config,
                                               options) else 1

  if options.mode == PerfMode.RECORD_WPR:
    return 0 if wpr_utils.record_wpr(config, options) else 1

  raise RuntimeError('Unknown mode')

if __name__ == '__main__':
  sys.exit(main())
