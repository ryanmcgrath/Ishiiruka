set(SRCS ${SRCS}
    # In mainline, this is used - but to fit pre-existing Ishiiruka conventions,
    # the PowerPC code has been updated to use Common/BreakPoints instead (which
    # Ishiiruka used before).
    # PowerPC/BreakPoints.cpp

  PowerPC/MMU.cpp
  PowerPC/PowerPC.cpp
  PowerPC/PPCAnalyst.cpp
  PowerPC/PPCCache.cpp
  PowerPC/PPCSymbolDB.cpp
  PowerPC/PPCTables.cpp
  PowerPC/Profiler.cpp
  PowerPC/SignatureDB/CSVSignatureDB.cpp
  PowerPC/SignatureDB/DSYSignatureDB.cpp
  PowerPC/SignatureDB/MEGASignatureDB.cpp
  
  # In mainline, this is PowerPC/SignatureDB/SignatureDB.cpp.
  # It's been modified here to fit pre-existing Ishiiruka conventions.
  PowerPC/SignatureDB.cpp
  
  PowerPC/JitInterface.cpp
  PowerPC/CachedInterpreter/CachedInterpreter.cpp
  PowerPC/CachedInterpreter/InterpreterBlockCache.cpp
  PowerPC/Interpreter/Interpreter_Branch.cpp
  PowerPC/Interpreter/Interpreter.cpp
  PowerPC/Interpreter/Interpreter_FloatingPoint.cpp
  PowerPC/Interpreter/Interpreter_Integer.cpp
  PowerPC/Interpreter/Interpreter_LoadStore.cpp
  PowerPC/Interpreter/Interpreter_LoadStorePaired.cpp
  PowerPC/Interpreter/Interpreter_Paired.cpp
  PowerPC/Interpreter/Interpreter_SystemRegisters.cpp
  PowerPC/Interpreter/Interpreter_Tables.cpp
  PowerPC/JitCommon/JitAsmCommon.cpp
  PowerPC/JitCommon/JitBase.cpp
  PowerPC/JitCommon/JitCache.cpp
)

if(_M_X86)
  set(SRCS ${SRCS}
    PowerPC/Jit64/FPURegCache.cpp
    PowerPC/Jit64/GPRRegCache.cpp
    PowerPC/Jit64/Jit64_Tables.cpp
    PowerPC/Jit64/JitAsm.cpp
    PowerPC/Jit64/Jit_Branch.cpp
    PowerPC/Jit64/Jit.cpp
    PowerPC/Jit64/Jit_FloatingPoint.cpp
    PowerPC/Jit64/Jit_Integer.cpp
    PowerPC/Jit64/Jit_LoadStore.cpp
    PowerPC/Jit64/Jit_LoadStoreFloating.cpp
    PowerPC/Jit64/Jit_LoadStorePaired.cpp
    PowerPC/Jit64/Jit_Paired.cpp
    PowerPC/Jit64/JitRegCache.cpp
    PowerPC/Jit64/Jit_SystemRegisters.cpp
    PowerPC/Jit64Common/BlockCache.cpp
    PowerPC/Jit64Common/ConstantPool.cpp
    PowerPC/Jit64Common/EmuCodeBlock.cpp
    PowerPC/Jit64Common/FarCodeCache.cpp
    PowerPC/Jit64Common/Jit64AsmCommon.cpp
    PowerPC/Jit64Common/Jit64Base.cpp
    PowerPC/Jit64Common/TrampolineCache.cpp
  )
elseif(_M_ARM_64)
  set(SRCS ${SRCS}
    PowerPC/JitArm64/Jit.cpp
    PowerPC/JitArm64/JitAsm.cpp
    PowerPC/JitArm64/JitArm64Cache.cpp
    PowerPC/JitArm64/JitArm64_RegCache.cpp
    PowerPC/JitArm64/JitArm64_BackPatch.cpp
    PowerPC/JitArm64/JitArm64_Branch.cpp
    PowerPC/JitArm64/JitArm64_FloatingPoint.cpp
    PowerPC/JitArm64/JitArm64_Integer.cpp
    PowerPC/JitArm64/JitArm64_LoadStore.cpp
    PowerPC/JitArm64/JitArm64_LoadStoreFloating.cpp
    PowerPC/JitArm64/JitArm64_Paired.cpp
    PowerPC/JitArm64/JitArm64_LoadStorePaired.cpp
    PowerPC/JitArm64/JitArm64_SystemRegisters.cpp
    PowerPC/JitArm64/Jit_Util.cpp
    PowerPC/JitArm64/JitArm64_Tables.cpp
  )
endif()

if(GDBSTUB)
  set(SRCS ${SRCS} PowerPC/GDBStub.cpp)
endif()

# This is necessary to ensure that SRCS is hoisted upwards.
set(SRCS ${SRCS} PARENT_SCOPE)
