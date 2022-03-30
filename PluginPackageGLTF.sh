#!/bin/bash


UE_ROOT="/Users/Shared/UnrealEngine/UE_4.27/Engine"
UE_BASH_DIR="${UE_ROOT}/Build/BatchFiles"
UE_EXECUTABLE="${UE_ROOT}/Binaries/Mac/UE4Editor.app/Contents/MacOS/UE4Editor"

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

PLUGIN_FOLDER=`echo $DIR`

cd $UE_BASH_DIR

#./RunUAT.sh BuildCookRun -project=$DEMO_PROJECT_FILE  -installed -nop4 -cook -stage -package -ue4exe=$UE_EXECUTABLE -ddc=InstalledDerivedDataBackendGraph -pak -prereqs -targetplatform=IOS -build -target=AukiUnrealDemo -clientconfig=DebugGame -utf8output
#-Package="$UE_ROOT/Plugins/glTFRuntime"

./RunUAT.sh BuildPlugin -Plugin="$PLUGIN_FOLDER/glTFRuntime.uplugin" -TargetPlatforms=Mac+IOS -Package="$PLUGIN_FOLDER/Packaged"