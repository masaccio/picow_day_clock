#! /bin/bash
if [ $(uname -s) == "Darwin" ]; then
  brew update
  brew install picotool llvm
  brew install --cask gcc-arm-embedded

  export PICO_SDK_PATH="${HOME}/.pico-sdk/sdk/2.3.0"
  export PICO_TOOLCHAIN_PATH="${HOME}/.pico-sdk/toolchain/15_2_Rel1"
  export PATH="${HOME}/.pico-sdk/toolchain/15_2_Rel1/bin:${PATH}"
  export PATH="${HOME}/.pico-sdk/picotool/2.3.0/picotool:${PATH}"
  export PATH="${HOME}/.pico-sdk/cmake/v4.3.4/bin:${PATH}"
  export PATH="${HOME}/.pico-sdk/ninja/v1.13.2:${PATH}"
  export PATH="/opt/homebrew/opt/llvm/bin:${PATH}"
  export PATH="$(brew --prefix gcc-arm-embedded)/bin:${PATH}"
  export PATH="$(brew --prefix llvm)/bin:${PATH}"
else
  sudo apt-get update
  sudo apt-get install -y cmake ninja-build lcov llvm gcc 
  sudo apt-get install -y gcc-arm-none-eabi libnewlib-arm-none-eabi

  export PICO_SDK_PATH="${HOME}/.pico-sdk/sdk/2.3.0"
  export PICO_TOOLCHAIN_PATH="${HOME}/.pico-sdk/toolchain/15_2_Rel1"
  export PATH="${HOME}/.pico-sdk/toolchain/15_2_Rel1/bin:${PATH}"
  export PATH="${HOME}/.pico-sdk/picotool/2.3.0/picotool:${PATH}"
  export PATH="${HOME}/.pico-sdk/cmake/v4.3.4/bin:${PATH}"
  export PATH="${HOME}/.pico-sdk/ninja/v1.13.2:${PATH}"
fi

rm -rf build
mkdir -p build

cmake -DCMAKE_BUILD_TYPE=Debug -B build -G Ninja
ninja -v -C build test_picow_day_clock
cmake --build build --target run_tests
cmake --build build --target coverage_reports
