# Build M1 junk cycle easier.

DATA_SYS_PATH="./Data/Sys/"
BINARY_PATH="./build/Binaries/Slippi Dolphin.app/Contents/Resources/"

mkdir -p build
pushd build
cmake -DMACOS_CODE_SIGNING="ON" -DMACOS_CODE_SIGNING_IDENTITY="8JNDBG9U79" ..
make -j7
popd

echo "Copying Sys files into the bundle"
cp -Rfn "${DATA_SYS_PATH}" "${BINARY_PATH}"

echo "Signing the build..."
/usr/bin/codesign -f -s "8JNDBG9U79" --deep --options runtime --entitlements Source/Core/DolphinWX/Entitlements.plist build/Binaries/Slippi\ Dolphin.app

echo "Attempting to open..."
lldb -d -f build/Binaries/Slippi\ Dolphin.app/Contents/MacOS/Slippi\ Dolphin -o 'run'
