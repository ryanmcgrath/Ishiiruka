// Copyright 2008 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#include <cstring>

#include "Common/ChunkFile.h"
#include "Common/CommonFuncs.h"
#include "Common/CommonTypes.h"
#include "Core/HW/GPFifo.h"
#include "Core/HW/Memmap.h"
#include "Core/HW/ProcessorInterface.h"
#include "Core/PowerPC/JitInterface.h"
#include "VideoCommon/CommandProcessor.h"

namespace GPFifo
{
// 32 Byte gather pipe with extra space
// Overfilling is no problem (up to the real limit), CheckGatherPipe will blast the
// contents in nicely sized chunks
//
// Other optimizations to think about:
// - If the GP is NOT linked to the FIFO, just blast to memory byte by word
// - If the GP IS linked to the FIFO, use a fast wrapping buffer and skip writing to memory
//
// Both of these should actually work! Only problem is that we have to decide at run time,
// the same function could use both methods. Compile 2 different versions of each such block?

// More room for the fastmodes
#ifdef __APPLE__
alignas(32) static u8 s_gather_pipe[GATHER_PIPE_SIZE * 16];

// pipe pointer
u8* g_gather_pipe_ptr = s_gather_pipe;

static size_t GetGatherPipeCount() { return g_gather_pipe_ptr - s_gather_pipe; }
static void SetGatherPipeCount(size_t size) { g_gather_pipe_ptr = s_gather_pipe + size; }
#else
alignas(32) u8 m_gatherPipe[GATHER_PIPE_SIZE * 16];

// pipe counter
u32 m_gatherPipeCount = 0;
#endif

void DoState(PointerWrap& p)
{
#ifdef __APPLE__
    p.Do(s_gather_pipe);
    u32 pipe_count = static_cast<u32>(GetGatherPipeCount());
    p.Do(pipe_count);
    SetGatherPipeCount(pipe_count);
#else
	p.Do(m_gatherPipe);
	p.Do(m_gatherPipeCount);
#endif
}

void Init()
{
	ResetGatherPipe();

#ifdef __APPLE__
    memset(s_gather_pipe, 0, sizeof(s_gather_pipe));
#else
	memset(m_gatherPipe, 0, sizeof(m_gatherPipe));
#endif
}

bool IsEmpty()
{
#ifdef __APPLE__
    return GetGatherPipeCount() == 0;
#else
	return m_gatherPipeCount == 0;
#endif
}

void ResetGatherPipe()
{
#ifdef __APPLE__
    SetGatherPipeCount(0);
#else
	m_gatherPipeCount = 0;
#endif
}

static void UpdateGatherPipe()
{
#ifdef __APPLE__
    size_t pipe_count = GetGatherPipeCount();
    size_t cnt;
#else
	u32 cnt;
#endif

    u8* curMem = Memory::GetPointer(ProcessorInterface::Fifo_CPUWritePointer);
		// copy the GatherPipe
#ifdef __APPLE__
    for (cnt = 0; pipe_count >= GATHER_PIPE_SIZE; cnt += GATHER_PIPE_SIZE)
    {
        memcpy(curMem, s_gather_pipe + cnt, GATHER_PIPE_SIZE);
        pipe_count -= GATHER_PIPE_SIZE;
#else
    for (cnt = 0; m_gatherPipeCount >= GATHER_PIPE_SIZE; cnt += GATHER_PIPE_SIZE)
    {
		memcpy(curMem, m_gatherPipe + cnt, GATHER_PIPE_SIZE);
		m_gatherPipeCount -= GATHER_PIPE_SIZE;
#endif

		// increase the CPUWritePointer
		if (ProcessorInterface::Fifo_CPUWritePointer == ProcessorInterface::Fifo_CPUEnd)
		{
			ProcessorInterface::Fifo_CPUWritePointer = ProcessorInterface::Fifo_CPUBase;
			curMem = Memory::GetPointer(ProcessorInterface::Fifo_CPUWritePointer);
		}
		else
		{
			curMem += GATHER_PIPE_SIZE;
			ProcessorInterface::Fifo_CPUWritePointer += GATHER_PIPE_SIZE;
		}

		CommandProcessor::GatherPipeBursted();
    }

	// move back the spill bytes
#ifdef __APPLE__
    memmove(s_gather_pipe, s_gather_pipe + cnt, pipe_count);
    SetGatherPipeCount(pipe_count);
#else
	memmove(m_gatherPipe, m_gatherPipe + cnt, m_gatherPipeCount);
#endif
}

void FastCheckGatherPipe()
{
#ifdef __APPLE__
    if (GetGatherPipeCount() >= GATHER_PIPE_SIZE)
#else
	if (m_gatherPipeCount >= GATHER_PIPE_SIZE)
#endif
	{
		UpdateGatherPipe();
	}
}

void CheckGatherPipe()
{
#ifdef __APPLE__
    if (GetGatherPipeCount() >= GATHER_PIPE_SIZE)
#else
	if (m_gatherPipeCount >= GATHER_PIPE_SIZE)
#endif
	{
		UpdateGatherPipe();

		// Profile where slow FIFO writes are occurring.
		JitInterface::CompileExceptionCheck(JitInterface::ExceptionType::EXCEPTIONS_FIFO_WRITE);
	}
}

void Write8(const u8 value)
{
	FastWrite8(value);
	CheckGatherPipe();
}

void Write16(const u16 value)
{
	FastWrite16(value);
	CheckGatherPipe();
}

void Write32(const u32 value)
{
	FastWrite32(value);
	CheckGatherPipe();
}

void Write64(const u64 value)
{
	FastWrite64(value);
	CheckGatherPipe();
}

void FastWrite8(const u8 value)
{
#ifdef __APPLE__
    *g_gather_pipe_ptr = value;
    g_gather_pipe_ptr += sizeof(u8);
#else
	m_gatherPipe[m_gatherPipeCount] = value;
	++m_gatherPipeCount;
#endif
}

void FastWrite16(u16 value)
{
	value = Common::swap16(value);

#ifdef __APPLE__
    std::memcpy(g_gather_pipe_ptr, &value, sizeof(u16));
    g_gather_pipe_ptr += sizeof(u16);
#else
	std::memcpy(&m_gatherPipe[m_gatherPipeCount], &value, sizeof(u16));
	m_gatherPipeCount += sizeof(u16);
#endif
}

void FastWrite32(u32 value)
{
	value = Common::swap32(value);

#ifdef __APPLE__
    std::memcpy(g_gather_pipe_ptr, &value, sizeof(u32));
    g_gather_pipe_ptr += sizeof(u32);
#else
	std::memcpy(&m_gatherPipe[m_gatherPipeCount], &value, sizeof(u32));
	m_gatherPipeCount += sizeof(u32);
#endif
}

void FastWrite64(u64 value)
{
	value = Common::swap64(value);

#ifdef __APPLE__
    std::memcpy(g_gather_pipe_ptr, &value, sizeof(u64));
    g_gather_pipe_ptr += sizeof(u64);
#else
	std::memcpy(&m_gatherPipe[m_gatherPipeCount], &value, sizeof(u64));
	m_gatherPipeCount += sizeof(u64);
#endif
}

}  // end of namespace GPFifo
