name: Build Geode Mod

on:
  push:
    branches:
      - main
  pull_request:
    branches:
      - main

jobs:
  build:
    strategy:
      fail-fast: false
      matrix:
        config:
          - name: Windows
            os: windows-2022
            platform: win
          - name: Android
            os: ubuntu-latest
            target: android
            platform: android
    name: ${{ matrix.config.name }}
    runs-on: ${{ matrix.config.os }}
    steps:
      - uses: actions/checkout@v4

      - name: Build Mod
        uses: geode-sdk/build-geode-mod@main
        with:
          target: ${{ matrix.config.target }}
          platform: ${{ matrix.config.platform }}


