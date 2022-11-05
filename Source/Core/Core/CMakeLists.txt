set(SRCS	ActionReplay.cpp
			Analytics.cpp
			ARDecrypt.cpp
			BootManager.cpp
			ConfigManager.cpp
			Core.cpp
			CoreTiming.cpp
			DSPEmulator.cpp
			ec_wii.cpp
			GeckoCodeConfig.cpp
			GeckoCode.cpp
			HotkeyManager.cpp
			MemTools.cpp
			Movie.cpp
			NetPlayClient.cpp
			NetPlayServer.cpp
			PatchEngine.cpp
			State.cpp
			Boot/Boot_BS2Emu.cpp
			Boot/Boot.cpp
			Boot/Boot_DOL.cpp
			Boot/Boot_ELF.cpp
			Boot/Boot_WiiWAD.cpp
			Boot/ElfReader.cpp
			Debugger/Debugger_SymbolMap.cpp
			Debugger/Dump.cpp
			Debugger/PPCDebugInterface.cpp
			DSP/DSPAssembler.cpp
			DSP/DSPDisassembler.cpp
			DSP/DSPAccelerator.cpp
			DSP/DSPCaptureLogger.cpp
			DSP/DSPHWInterface.cpp
			DSP/DSPMemoryMap.cpp
			DSP/DSPStacks.cpp
			DSP/DSPAnalyzer.cpp
			DSP/DSPCodeUtil.cpp
			DSP/LabelMap.cpp
			DSP/DSPCore.cpp
			DSP/DSPTables.cpp
			DSP/Interpreter/DSPIntArithmetic.cpp
			DSP/Interpreter/DSPIntBranch.cpp
			DSP/Interpreter/DSPIntCCUtil.cpp
			DSP/Interpreter/DSPInterpreter.cpp
			DSP/Interpreter/DSPIntExtOps.cpp
			DSP/Interpreter/DSPIntLoadStore.cpp
			DSP/Interpreter/DSPIntMisc.cpp
			DSP/Interpreter/DSPIntMultiplier.cpp
			DSP/Jit/DSPEmitter.cpp
			DSP/Jit/DSPJitRegCache.cpp
			DSP/Jit/DSPJitExtOps.cpp
			DSP/Jit/DSPJitBranch.cpp
			DSP/Jit/DSPJitCCUtil.cpp
			DSP/Jit/DSPJitArithmetic.cpp
			DSP/Jit/DSPJitLoadStore.cpp
			DSP/Jit/DSPJitMultiplier.cpp
			DSP/Jit/DSPJitUtil.cpp
			DSP/Jit/DSPJitMisc.cpp
			FifoPlayer/FifoAnalyzer.cpp
			FifoPlayer/FifoDataFile.cpp
			FifoPlayer/FifoPlaybackAnalyzer.cpp
			FifoPlayer/FifoPlayer.cpp
			FifoPlayer/FifoRecordAnalyzer.cpp
			FifoPlayer/FifoRecorder.cpp
			HLE/HLE.cpp
			HLE/HLE_Misc.cpp
			HLE/HLE_OS.cpp
			HW/AudioInterface.cpp
			HW/CPU.cpp
			HW/DSP.cpp
			HW/DSPHLE/UCodes/AX.cpp
			HW/DSPHLE/UCodes/AXWii.cpp
			HW/DSPHLE/UCodes/CARD.cpp
			HW/DSPHLE/UCodes/GBA.cpp
			HW/DSPHLE/UCodes/INIT.cpp
			HW/DSPHLE/UCodes/ROM.cpp
			HW/DSPHLE/UCodes/UCodes.cpp
			HW/DSPHLE/UCodes/Zelda.cpp
			HW/DSPHLE/MailHandler.cpp
			HW/DSPHLE/DSPHLE.cpp
			HW/DSPLLE/DSPDebugInterface.cpp
			HW/DSPLLE/DSPHost.cpp
			HW/DSPLLE/DSPSymbols.cpp
			HW/DSPLLE/DSPLLEGlobals.cpp
			HW/DSPLLE/DSPLLE.cpp
			HW/DSPLLE/DSPLLETools.cpp
			HW/DVDInterface.cpp
			HW/DVDThread.cpp
			HW/EXI_Channel.cpp
			HW/EXI.cpp
			HW/EXI_Device.cpp
			HW/EXI_DeviceAD16.cpp
			HW/EXI_DeviceAGP.cpp
			HW/EXI_DeviceEthernet.cpp
			HW/EXI_DeviceGecko.cpp
			HW/EXI_DeviceIPL.cpp
			HW/EXI_DeviceMemoryCard.cpp
			HW/EXI_DeviceSlippi.cpp
			HW/EXI_DeviceMic.cpp
			HW/GCKeyboard.cpp
			HW/GCKeyboardEmu.cpp
			HW/GCMemcard.cpp
			HW/GCMemcardDirectory.cpp
			HW/GCMemcardRaw.cpp
			HW/GCPad.cpp
			HW/GCPadEmu.cpp
			HW/GPFifo.cpp
			HW/HW.cpp
			HW/Memmap.cpp
			HW/MemoryInterface.cpp
			HW/MMIO.cpp
			HW/ProcessorInterface.cpp
			HW/SI.cpp
			HW/SI_Device.cpp
			HW/SI_DeviceDanceMat.cpp
			HW/SI_DeviceGBA.cpp
			HW/SI_DeviceGCAdapter.cpp
			HW/SI_DeviceGCController.cpp
			HW/SI_DeviceGCSteeringWheel.cpp
			HW/SI_DeviceKeyboard.cpp
			HW/Sram.cpp
			HW/StreamADPCM.cpp
			HW/SystemTimers.cpp
			HW/VideoInterface.cpp
			HW/WII_IPC.cpp
			HW/Wiimote.cpp
			HW/WiimoteEmu/WiimoteEmu.cpp
			HW/WiimoteEmu/Attachment/Classic.cpp
			HW/WiimoteEmu/Attachment/Attachment.cpp
			HW/WiimoteEmu/Attachment/Nunchuk.cpp
			HW/WiimoteEmu/Attachment/Drums.cpp
			HW/WiimoteEmu/Attachment/Guitar.cpp
			HW/WiimoteEmu/Attachment/Turntable.cpp
			HW/WiimoteEmu/EmuSubroutines.cpp
			HW/WiimoteEmu/Encryption.cpp
			HW/WiimoteEmu/Speaker.cpp
			HW/WiimoteReal/WiimoteReal.cpp
			HW/WiiSaveCrypted.cpp
			IPC_HLE/ICMPLin.cpp
			IPC_HLE/NWC24Config.cpp
			IPC_HLE/WII_IPC_HLE.cpp
			IPC_HLE/WII_IPC_HLE_Device.cpp
			IPC_HLE/WII_IPC_HLE_Device_stub.cpp
			IPC_HLE/WII_IPC_HLE_Device_DI.cpp
			IPC_HLE/WII_IPC_HLE_Device_es.cpp
			IPC_HLE/WII_IPC_HLE_Device_FileIO.cpp
			IPC_HLE/WII_IPC_HLE_Device_fs.cpp
			IPC_HLE/WII_Socket.cpp
			IPC_HLE/WII_IPC_HLE_Device_net.cpp
			IPC_HLE/WII_IPC_HLE_Device_net_ssl.cpp
			IPC_HLE/WII_IPC_HLE_Device_stm.cpp
			IPC_HLE/WII_IPC_HLE_Device_sdio_slot0.cpp
			IPC_HLE/WII_IPC_HLE_Device_usb_bt_base.cpp
			IPC_HLE/WII_IPC_HLE_Device_usb_bt_emu.cpp
			IPC_HLE/WII_IPC_HLE_Device_usb_bt_stub.cpp
			IPC_HLE/WII_IPC_HLE_Device_usb_kbd.cpp
			IPC_HLE/WII_IPC_HLE_Device_usb_ven.cpp
			IPC_HLE/WII_IPC_HLE_WiiMote.cpp
			IPC_HLE/WiiMote_HID_Attr.cpp
			IPC_HLE/WiiNetConfig.cpp
			Slippi/SlippiGameFileLoader.cpp
			Slippi/SlippiMatchmaking.cpp
			Slippi/SlippiNetplay.cpp
			Slippi/SlippiPad.cpp
			Slippi/SlippiPlayback.cpp
			Slippi/SlippiReplayComm.cpp
			Slippi/SlippiSavestate.cpp
			Slippi/SlippiSpectate.cpp
			Slippi/SlippiTimer.cpp
			Slippi/SlippiUser.cpp
			Slippi/SlippiGameReporter.cpp
			Slippi/SlippiDirectCodes.cpp
			)

# PowerPC is branched for Apple as of 11/04/2022 due to it becoming exceedingly
# difficult to allocate memory below the 2GB boundary on those platforms. Ishiiruka
# needs to do that, but for macOS we've just backported the JIT from an older version
# of mainline (revision: 3443454ba279811a6e10a840d8c21af3e24dd614) in order to handle
# allocating above the boundary.
#
# Note that we're explicitly only doing this on Apple platforms as we want to avoid
# unnecessary performance degrading on non-Apple platforms due to differences in 
# mainline and Ishiiruka.
if(APPLE)
    add_subdirectory(PowerPC PowerPC)
else()
    add_subdirectory(PowerPC PowerPC)
endif()

set(LIBS
	audiocommon
	bdisasm
	common
	cubeb
	discio
	enet
	inputcommon
	${LZO}
	semver
	sfml-network
	sfml-system
	SlippiLib
	vcdcom
	vcddec
	vcdenc
	videoogl
	videosoftware
	z
	)

if(LIBUSB_FOUND)
	# Using shared LibUSB
	set(LIBS	${LIBS}	${LIBUSB_LIBRARIES})
	set(SRCS	${SRCS}	IPC_HLE/WII_IPC_HLE_Device_hid.cpp
	        	       	IPC_HLE/WII_IPC_HLE_Device_usb_bt_real.cpp)
endif(LIBUSB_FOUND)

set(LIBS ${LIBS} videovulkan)

set(LIBS ${LIBS} ${MBEDTLS_LIBRARIES})

if(WIN32)
	set(SRCS ${SRCS} HW/BBA-TAP/TAP_Win32.cpp HW/WiimoteReal/IOWin.cpp)
elseif(APPLE)
	set(SRCS ${SRCS} HW/BBA-TAP/TAP_Apple.cpp HW/WiimoteReal/IOdarwin.mm)
	set(LIBS ${LIBS}
		${IOB_LIBRARY})
elseif(UNIX)
	set(SRCS ${SRCS} HW/BBA-TAP/TAP_Unix.cpp)
	if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux" AND BLUEZ_FOUND)
		set(SRCS ${SRCS} HW/WiimoteReal/IOLinux.cpp)
		set(LIBS ${LIBS} bluetooth)
	elseif(ANDROID)
		set(SRCS ${SRCS} HW/WiimoteReal/IOAndroid.cpp)
	endif()
endif()
if(HIDAPI_FOUND)
	set(SRCS ${SRCS} HW/WiimoteReal/IOhidapi.cpp)
endif(HIDAPI_FOUND)

if(PORTAUDIO_FOUND)
	set(LIBS ${LIBS} ${PORTAUDIO_LIBRARIES})
endif(PORTAUDIO_FOUND)

if(OPROFILE_FOUND)
	set(LIBS ${LIBS} ${OPROFILE_LIBRARIES})
endif(OPROFILE_FOUND)

if(UNIX)
	set(SRCS ${SRCS} MemoryWatcher.cpp)
endif(UNIX)

add_dolphin_library(core "${SRCS}" "${LIBS}")
