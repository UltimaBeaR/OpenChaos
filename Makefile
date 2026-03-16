SHELL         := bash
MSBUILD       := "C:/Program Files/Microsoft Visual Studio/18/Community/MSBuild/Current/Bin/MSBuild.exe"
VCPKG_TARGETS := C:/Program Files/Microsoft Visual Studio/18/Community/VC/vcpkg/scripts/buildsystems/msbuild/vcpkg.targets
SLN           := new_game/fallen/Fallen.sln
MSBUILD_FLAGS := -t:Rebuild -p:Platform=Win32 "-p:CustomAfterMicrosoftCppTargets=$(VCPKG_TARGETS)"

.PHONY: build-release build-debug run-release run-debug

build-release:
	$(MSBUILD) $(SLN) $(MSBUILD_FLAGS) -p:Configuration=Release

build-debug:
	$(MSBUILD) $(SLN) $(MSBUILD_FLAGS) -p:Configuration=Debug

run-release:
	powershell -Command "Start-Process -FilePath 'new_game/fallen/Release/Fallen.exe' -WorkingDirectory (Resolve-Path 'new_game/fallen/Release')"

run-debug:
	powershell -Command "Start-Process -FilePath 'new_game/fallen/Debug/Fallen.exe' -WorkingDirectory (Resolve-Path 'new_game/fallen/Debug')"
