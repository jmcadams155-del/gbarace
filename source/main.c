name: GBA Build

on: [push]

jobs:
  build:
    runs-on: ubuntu-latest
    container: devkitpro/devkitarm  # This container has ALL the GBA tools pre-installed!
    
    steps:
    - name: Checkout code
      uses: actions/checkout@v4

    - name: Compile Game
      run: make

    - name: Upload GBA ROM
      uses: actions/upload-artifact@v4
      with:
        name: My-GBA-Game
        path: game.gba
