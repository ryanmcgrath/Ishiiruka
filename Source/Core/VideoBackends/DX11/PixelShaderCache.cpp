// Copyright 2013 Dolphin Emulator Project
// Licensed under GPLv2
// Refer to the license.txt file included.

#include "Common/FileUtil.h"
#include "Common/LinearDiskCache.h"

#include "VideoCommon/Debugger.h"
#include "VideoCommon/Statistics.h"
#include "VideoCommon/VideoConfig.h"
#include "VideoCommon/HLSLCompiler.h"

#include "D3DBase.h"
#include "D3DShader.h"
#include "Globals.h"
#include "VideoCommon/PixelShaderGen.h"
#include "PixelShaderCache.h"

#include "Core/ConfigManager.h"

extern int frameCount;

// See comment near the bottom of this file.
GC_ALIGNED16(float psconstants[C_PENVCONST_END*4]);
bool pscbufchanged = true;

namespace DX11
{

PixelShaderCache::PSCache PixelShaderCache::PixelShaders;
const PixelShaderCache::PSCacheEntry* PixelShaderCache::last_entry;
PixelShaderUid PixelShaderCache::last_uid;
PixelShaderUid PixelShaderCache::external_last_uid;
UidChecker<PixelShaderUid,ShaderCode> PixelShaderCache::pixel_uid_checker;
static HLSLAsyncCompiler *Compiler;
static Common::SpinLock<true> PixelShadersLock;

LinearDiskCache<PixelShaderUid, u8> g_ps_disk_cache;

ID3D11PixelShader* s_ColorMatrixProgram[2] = {nullptr};
ID3D11PixelShader* s_ColorCopyProgram[2] = {nullptr};
ID3D11PixelShader* s_DepthMatrixProgram[2] = {nullptr};
ID3D11PixelShader* s_ClearProgram = nullptr;
ID3D11PixelShader* s_rgba6_to_rgb8[2] = {nullptr};
ID3D11PixelShader* s_rgb8_to_rgba6[2] = {nullptr};
ID3D11Buffer* pscbuf = nullptr;

const char clear_program_code[] = {
	"void main(\n"
	"out float4 ocol0 : SV_Target,\n"
	"in float4 pos : SV_Position,\n"
	"in float4 incol0 : COLOR0){\n"
	"ocol0 = incol0;\n"
	"}\n"
};

// TODO: Find some way to avoid having separate shaders for non-MSAA and MSAA...
const char color_copy_program_code[] = {
	"sampler samp0 : register(s0);\n"
	"Texture2D Tex0 : register(t0);\n"
	"void main(\n"
	"out float4 ocol0 : SV_Target,\n"
	"in float4 pos : SV_Position,\n"
	"in float2 uv0 : TEXCOORD0){\n"
	"ocol0 = Tex0.Sample(samp0,uv0);\n"
	"}\n"
};

// TODO: Improve sampling algorithm!
const char color_copy_program_code_msaa[] = {
	"sampler samp0 : register(s0);\n"
	"Texture2DMS<float4, %d> Tex0 : register(t0);\n"
	"void main(\n"
	"out float4 ocol0 : SV_Target,\n"
	"in float4 pos : SV_Position,\n"
	"in float2 uv0 : TEXCOORD0){\n"
	"int width, height, samples;\n"
	"Tex0.GetDimensions(width, height, samples);\n"
	"ocol0 = 0;\n"
	"for(int i = 0; i < samples; ++i)\n"
	"	ocol0 += Tex0.Load(int2(uv0.x*(width), uv0.y*(height)), i);\n"
	"ocol0 /= samples;\n"
	"}\n"
};

const char color_matrix_program_code[] = {
	"sampler samp0 : register(s0);\n"
	"Texture2D Tex0 : register(t0);\n"
	"uniform float4 cColMatrix[7] : register(c0);\n"
	"void main(\n" 
	"out float4 ocol0 : SV_Target,\n"
	"in float4 pos : SV_Position,\n"
	" in float2 uv0 : TEXCOORD0){\n"
	"float4 texcol = Tex0.Sample(samp0,uv0);\n"
	"texcol = round(texcol * cColMatrix[5])*cColMatrix[6];\n"
	"ocol0 = float4(dot(texcol,cColMatrix[0]),dot(texcol,cColMatrix[1]),dot(texcol,cColMatrix[2]),dot(texcol,cColMatrix[3])) + cColMatrix[4];\n"
	"}\n"
};

const char color_matrix_program_code_msaa[] = {
	"sampler samp0 : register(s0);\n"
	"Texture2DMS<float4, %d> Tex0 : register(t0);\n"
	"uniform float4 cColMatrix[7] : register(c0);\n"
	"void main(\n" 
	"out float4 ocol0 : SV_Target,\n"
	"in float4 pos : SV_Position,\n"
	" in float2 uv0 : TEXCOORD0){\n"
	"int width, height, samples;\n"
	"Tex0.GetDimensions(width, height, samples);\n"
	"float4 texcol = 0;\n"
	"for(int i = 0; i < samples; ++i)\n"
	"	texcol += Tex0.Load(int2(uv0.x*(width), uv0.y*(height)), i);\n"
	"texcol /= samples;\n"
	"texcol = round(texcol * cColMatrix[5])*cColMatrix[6];\n"
	"ocol0 = float4(dot(texcol,cColMatrix[0]),dot(texcol,cColMatrix[1]),dot(texcol,cColMatrix[2]),dot(texcol,cColMatrix[3])) + cColMatrix[4];\n"
	"}\n"
};

const char depth_matrix_program[] = {
	"sampler samp0 : register(s0);\n"
	"Texture2D Tex0 : register(t0);\n"
	"uniform float4 cColMatrix[7] : register(c0);\n"
	"void main(\n"
	"out float4 ocol0 : SV_Target,\n"
	" in float4 pos : SV_Position,\n"
	" in float2 uv0 : TEXCOORD0){\n"
	"	float4 texcol = Tex0.Sample(samp0,uv0);\n"

	// 255.99998474121 = 16777215/16777216*256
	"	float workspace = texcol.x * 255.99998474121;\n"

	"	texcol.x = floor(workspace);\n"         // x component

	"	workspace = workspace - texcol.x;\n"    // subtract x component out
	"	workspace = workspace * 256.0;\n"       // shift left 8 bits
	"	texcol.y = floor(workspace);\n"         // y component

	"	workspace = workspace - texcol.y;\n"    // subtract y component out
	"	workspace = workspace * 256.0;\n"       // shift left 8 bits
	"	texcol.z = floor(workspace);\n"         // z component

	"	texcol.w = texcol.x;\n"                 // duplicate x into w

	"	texcol = texcol / 255.0;\n"             // normalize components to [0.0..1.0]

	"	texcol.w = texcol.w * 15.0;\n"
	"	texcol.w = floor(texcol.w);\n"
	"	texcol.w = texcol.w / 15.0;\n"          // w component

	"	ocol0 = float4(dot(texcol,cColMatrix[0]),dot(texcol,cColMatrix[1]),dot(texcol,cColMatrix[2]),dot(texcol,cColMatrix[3])) + cColMatrix[4];\n"
	"}\n"
};

const char depth_matrix_program_msaa[] = {
	"sampler samp0 : register(s0);\n"
	"Texture2DMS<float4, %d> Tex0 : register(t0);\n"
	"uniform float4 cColMatrix[7] : register(c0);\n"
	"void main(\n"
	"out float4 ocol0 : SV_Target,\n"
	" in float4 pos : SV_Position,\n"
	" in float2 uv0 : TEXCOORD0){\n"
	"	int width, height, samples;\n"
	"	Tex0.GetDimensions(width, height, samples);\n"
	"	float4 texcol = 0;\n"
	"	for(int i = 0; i < samples; ++i)\n"
	"		texcol += Tex0.Load(int2(uv0.x*(width), uv0.y*(height)), i);\n"
	"	texcol /= samples;\n"

	// 255.99998474121 = 16777215/16777216*256
	"	float workspace = texcol.x * 255.99998474121;\n"

	"	texcol.x = floor(workspace);\n"         // x component

	"	workspace = workspace - texcol.x;\n"    // subtract x component out
	"	workspace = workspace * 256.0;\n"       // shift left 8 bits
	"	texcol.y = floor(workspace);\n"         // y component

	"	workspace = workspace - texcol.y;\n"    // subtract y component out
	"	workspace = workspace * 256.0;\n"       // shift left 8 bits
	"	texcol.z = floor(workspace);\n"         // z component

	"	texcol.w = texcol.x;\n"                 // duplicate x into w

	"	texcol = texcol / 255.0;\n"             // normalize components to [0.0..1.0]

	"	texcol.w = texcol.w * 15.0;\n"
	"	texcol.w = floor(texcol.w);\n"
	"	texcol.w = texcol.w / 15.0;\n"          // w component

	"	ocol0 = float4(dot(texcol,cColMatrix[0]),dot(texcol,cColMatrix[1]),dot(texcol,cColMatrix[2]),dot(texcol,cColMatrix[3])) + cColMatrix[4];\n"
	"}\n"
};

const char reint_rgba6_to_rgb8[] = {
	"sampler samp0 : register(s0);\n"
	"Texture2D Tex0 : register(t0);\n"
	"void main(\n"
	"	out float4 ocol0 : SV_Target,\n"
	"	in float4 pos : SV_Position,\n"
	"	in float2 uv0 : TEXCOORD0)\n"
	"{\n"
	"	int4 src6 = round(Tex0.Sample(samp0,uv0) * 63.f);\n"
	"	int4 dst8;\n"
	"	dst8.r = (src6.r << 2) | (src6.g >> 4);\n"
	"	dst8.g = ((src6.g & 0xF) << 4) | (src6.b >> 2);\n"
	"	dst8.b = ((src6.b & 0x3) << 6) | src6.a;\n"
	"	dst8.a = 255;\n"
	"	ocol0 = (float4)dst8 / 255.f;\n"
	"}"
};

const char reint_rgba6_to_rgb8_msaa[] = {
	"sampler samp0 : register(s0);\n"
	"Texture2DMS<float4, %d> Tex0 : register(t0);\n"
	"void main(\n"
	"	out float4 ocol0 : SV_Target,\n"
	"	in float4 pos : SV_Position,\n"
	"	in float2 uv0 : TEXCOORD0)\n"
	"{\n"
	"	int width, height, samples;\n"
	"	Tex0.GetDimensions(width, height, samples);\n"
	"	float4 texcol = 0;\n"
	"	for(int i = 0; i < samples; ++i)\n"
	"		texcol += Tex0.Load(int2(uv0.x*(width), uv0.y*(height)), i);\n"
	"	texcol /= samples;\n"
	"	int4 src6 = round(texcol * 63.f);\n"
	"	int4 dst8;\n"
	"	dst8.r = (src6.r << 2) | (src6.g >> 4);\n"
	"	dst8.g = ((src6.g & 0xF) << 4) | (src6.b >> 2);\n"
	"	dst8.b = ((src6.b & 0x3) << 6) | src6.a;\n"
	"	dst8.a = 255;\n"
	"	ocol0 = (float4)dst8 / 255.f;\n"
	"}"
};

const char reint_rgb8_to_rgba6[] = {
	"sampler samp0 : register(s0);\n"
	"Texture2D Tex0 : register(t0);\n"
	"void main(\n"
	"	out float4 ocol0 : SV_Target,\n"
	"	in float4 pos : SV_Position,\n"
	"	in float2 uv0 : TEXCOORD0)\n"
	"{\n"
	"	int4 src8 = round(Tex0.Sample(samp0,uv0) * 255.f);\n"
	"	int4 dst6;\n"
	"	dst6.r = src8.r >> 2;\n"
	"	dst6.g = ((src8.r & 0x3) << 4) | (src8.g >> 4);\n"
	"	dst6.b = ((src8.g & 0xF) << 2) | (src8.b >> 6);\n"
	"	dst6.a = src8.b & 0x3F;\n"
	"	ocol0 = (float4)dst6 / 63.f;\n"
	"}\n"
};

const char reint_rgb8_to_rgba6_msaa[] = {
	"sampler samp0 : register(s0);\n"
	"Texture2DMS<float4, %d> Tex0 : register(t0);\n"
	"void main(\n"
	"	out float4 ocol0 : SV_Target,\n"
	"	in float4 pos : SV_Position,\n"
	"	in float2 uv0 : TEXCOORD0)\n"
	"{\n"
	"	int width, height, samples;\n"
	"	Tex0.GetDimensions(width, height, samples);\n"
	"	float4 texcol = 0;\n"
	"	for(int i = 0; i < samples; ++i)\n"
	"		texcol += Tex0.Load(int2(uv0.x*(width), uv0.y*(height)), i);\n"
	"	texcol /= samples;\n"
	"	int4 src8 = round(texcol * 255.f);\n"
	"	int4 dst6;\n"
	"	dst6.r = src8.r >> 2;\n"
	"	dst6.g = ((src8.r & 0x3) << 4) | (src8.g >> 4);\n"
	"	dst6.b = ((src8.g & 0xF) << 2) | (src8.b >> 6);\n"
	"	dst6.a = src8.b & 0x3F;\n"
	"	ocol0 = (float4)dst6 / 63.f;\n"
	"}\n"
};

ID3D11PixelShader* PixelShaderCache::ReinterpRGBA6ToRGB8(bool multisampled)
{
	if (!multisampled || D3D::GetAAMode(g_ActiveConfig.iMultisampleMode).Count == 1)
	{
		if (!s_rgba6_to_rgb8[0])
		{
			s_rgba6_to_rgb8[0] = D3D::CompileAndCreatePixelShader(reint_rgba6_to_rgb8);
			CHECK(s_rgba6_to_rgb8[0], "Create RGBA6 to RGB8 pixel shader");
			D3D::SetDebugObjectName(s_rgba6_to_rgb8[0], "RGBA6 to RGB8 pixel shader");
		}
		return s_rgba6_to_rgb8[0];
	}
	else if (!s_rgba6_to_rgb8[1])
	{
		// create MSAA shader for current AA mode
		char buf[1024];
		const int l = sprintf_s(buf, 1024, reint_rgba6_to_rgb8_msaa, D3D::GetAAMode(g_ActiveConfig.iMultisampleMode).Count);

		s_rgba6_to_rgb8[1] = D3D::CompileAndCreatePixelShader(buf);

		CHECK(s_rgba6_to_rgb8[1], "Create RGBA6 to RGB8 MSAA pixel shader");
		D3D::SetDebugObjectName(s_rgba6_to_rgb8[1], "RGBA6 to RGB8 MSAA pixel shader");
	}
	return s_rgba6_to_rgb8[1];
}

ID3D11PixelShader* PixelShaderCache::ReinterpRGB8ToRGBA6(bool multisampled)
{
	if (!multisampled || D3D::GetAAMode(g_ActiveConfig.iMultisampleMode).Count == 1)
	{
		if (!s_rgb8_to_rgba6[0])
		{
			s_rgb8_to_rgba6[0] = D3D::CompileAndCreatePixelShader(reint_rgb8_to_rgba6);
			CHECK(s_rgb8_to_rgba6[0], "Create RGB8 to RGBA6 pixel shader");
			D3D::SetDebugObjectName(s_rgb8_to_rgba6[0], "RGB8 to RGBA6 pixel shader");
		}
		return s_rgb8_to_rgba6[0];
	}
	else if (!s_rgb8_to_rgba6[1])
	{
		// create MSAA shader for current AA mode
		char buf[1024];
		const int l = sprintf_s(buf, 1024, reint_rgb8_to_rgba6_msaa, D3D::GetAAMode(g_ActiveConfig.iMultisampleMode).Count);

		s_rgb8_to_rgba6[1] = D3D::CompileAndCreatePixelShader(buf);

		CHECK(s_rgb8_to_rgba6[1], "Create RGB8 to RGBA6 MSAA pixel shader");
		D3D::SetDebugObjectName(s_rgb8_to_rgba6[1], "RGB8 to RGBA6 MSAA pixel shader");
	}
	return s_rgb8_to_rgba6[1];
}

ID3D11PixelShader* PixelShaderCache::GetColorCopyProgram(bool multisampled)
{
	if (!multisampled || D3D::GetAAMode(g_ActiveConfig.iMultisampleMode).Count == 1) return s_ColorCopyProgram[0];
	else if (s_ColorCopyProgram[1]) return s_ColorCopyProgram[1];
	else
	{
		// create MSAA shader for current AA mode
		char buf[1024];
		int l = sprintf_s(buf, 1024, color_copy_program_code_msaa, D3D::GetAAMode(g_ActiveConfig.iMultisampleMode).Count);
		s_ColorCopyProgram[1] = D3D::CompileAndCreatePixelShader(buf);
		CHECK(s_ColorCopyProgram[1]!=nullptr, "Create color copy MSAA pixel shader");
		D3D::SetDebugObjectName((ID3D11DeviceChild*)s_ColorCopyProgram[1], "color copy MSAA pixel shader");
		return s_ColorCopyProgram[1];
	}
}

ID3D11PixelShader* PixelShaderCache::GetColorMatrixProgram(bool multisampled)
{
	if (!multisampled || D3D::GetAAMode(g_ActiveConfig.iMultisampleMode).Count == 1) return s_ColorMatrixProgram[0];
	else if (s_ColorMatrixProgram[1]) return s_ColorMatrixProgram[1];
	else
	{
		// create MSAA shader for current AA mode
		char buf[1024];
		int l = sprintf_s(buf, 1024, color_matrix_program_code_msaa, D3D::GetAAMode(g_ActiveConfig.iMultisampleMode).Count);
		s_ColorMatrixProgram[1] = D3D::CompileAndCreatePixelShader(buf);
		CHECK(s_ColorMatrixProgram[1]!=nullptr, "Create color matrix MSAA pixel shader");
		D3D::SetDebugObjectName((ID3D11DeviceChild*)s_ColorMatrixProgram[1], "color matrix MSAA pixel shader");
		return s_ColorMatrixProgram[1];
	}
}

ID3D11PixelShader* PixelShaderCache::GetDepthMatrixProgram(bool multisampled)
{
	if (!multisampled || D3D::GetAAMode(g_ActiveConfig.iMultisampleMode).Count == 1) return s_DepthMatrixProgram[0];
	else if (s_DepthMatrixProgram[1]) return s_DepthMatrixProgram[1];
	else
	{
		// create MSAA shader for current AA mode
		char buf[1024];
		int l = sprintf_s(buf, 1024, depth_matrix_program_msaa, D3D::GetAAMode(g_ActiveConfig.iMultisampleMode).Count);
		s_DepthMatrixProgram[1] = D3D::CompileAndCreatePixelShader(buf);
		CHECK(s_DepthMatrixProgram[1]!=nullptr, "Create depth matrix MSAA pixel shader");
		D3D::SetDebugObjectName((ID3D11DeviceChild*)s_DepthMatrixProgram[1], "depth matrix MSAA pixel shader");
		return s_DepthMatrixProgram[1];
	}
}

ID3D11PixelShader* PixelShaderCache::GetClearProgram()
{
	return s_ClearProgram;
}

ID3D11Buffer* &PixelShaderCache::GetConstantBuffer()
{
	// TODO: divide the global variables of the generated shaders into about 5 constant buffers to speed this up
	if (pscbufchanged)
	{
		bool lightingEnabled = xfregs.numChan.numColorChans > 0;
		bool enable_pl = g_ActiveConfig.bEnablePixelLighting && g_ActiveConfig.backend_info.bSupportsPixelLighting && lightingEnabled;
		D3D11_MAPPED_SUBRESOURCE map;
		int sz = enable_pl ? sizeof(psconstants) : C_PLIGHTS * 4 * sizeof(float);
		D3D::context->Map(pscbuf, 0, D3D11_MAP_WRITE_DISCARD, 0, &map);
		memcpy(map.pData, psconstants, sz);
		D3D::context->Unmap(pscbuf, 0);
		pscbufchanged = false;
		
		ADDSTAT(stats.thisFrame.bytesUniformStreamed, sz);
	}
	return pscbuf;
}

// this class will load the precompiled shaders into our cache
class PixelShaderCacheInserter : public LinearDiskCacheReader<PixelShaderUid, u8>
{
public:
	void Read(const PixelShaderUid &key, const u8 *value, u32 value_size)
	{
		PixelShaderCache::InsertByteCode(key, value, value_size);
	}
};

void PixelShaderCache::Init()
{
	Compiler = &HLSLAsyncCompiler::getInstance();
	PixelShadersLock.unlock();
	unsigned int cbsize = sizeof(psconstants); // is always a multiple of 16
	D3D11_BUFFER_DESC cbdesc = CD3D11_BUFFER_DESC(cbsize, D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);	
	D3D::device->CreateBuffer(&cbdesc, nullptr, &pscbuf);
	CHECK(pscbuf!=nullptr, "Create pixel shader constant buffer");
	D3D::SetDebugObjectName((ID3D11DeviceChild*)pscbuf, "pixel shader constant buffer used to emulate the GX pipeline");

	// used when drawing clear quads
	s_ClearProgram = D3D::CompileAndCreatePixelShader(clear_program_code);	
	CHECK(s_ClearProgram!=nullptr, "Create clear pixel shader");
	D3D::SetDebugObjectName((ID3D11DeviceChild*)s_ClearProgram, "clear pixel shader");

	// used when copying/resolving the color buffer
	s_ColorCopyProgram[0] = D3D::CompileAndCreatePixelShader(color_copy_program_code);
	CHECK(s_ColorCopyProgram[0]!=nullptr, "Create color copy pixel shader");
	D3D::SetDebugObjectName((ID3D11DeviceChild*)s_ColorCopyProgram[0], "color copy pixel shader");

	// used for color conversion
	s_ColorMatrixProgram[0] = D3D::CompileAndCreatePixelShader(color_matrix_program_code);
	CHECK(s_ColorMatrixProgram[0]!=nullptr, "Create color matrix pixel shader");
	D3D::SetDebugObjectName((ID3D11DeviceChild*)s_ColorMatrixProgram[0], "color matrix pixel shader");

	// used for depth copy
	s_DepthMatrixProgram[0] = D3D::CompileAndCreatePixelShader(depth_matrix_program);
	CHECK(s_DepthMatrixProgram[0]!=nullptr, "Create depth matrix pixel shader");
	D3D::SetDebugObjectName((ID3D11DeviceChild*)s_DepthMatrixProgram[0], "depth matrix pixel shader");

	Clear();

	if (!File::Exists(File::GetUserPath(D_SHADERCACHE_IDX)))
		File::CreateDir(File::GetUserPath(D_SHADERCACHE_IDX).c_str());

	SETSTAT(stats.numPixelShadersCreated, 0);
	SETSTAT(stats.numPixelShadersAlive, 0);

	char cache_filename[MAX_PATH];
	sprintf(cache_filename, "%sdx11-%s-ps.cache", File::GetUserPath(D_SHADERCACHE_IDX).c_str(),
			SConfig::GetInstance().m_LocalCoreStartupParameter.m_strUniqueID.c_str());
	PixelShaderCacheInserter inserter;
	PixelShadersLock.lock();
	g_ps_disk_cache.OpenAndRead(cache_filename, inserter);
	PixelShadersLock.unlock();

	if (g_Config.bEnableShaderDebugging)
		Clear();

	last_entry = nullptr;
}

// ONLY to be used during shutdown.
void PixelShaderCache::Clear()
{
	PixelShadersLock.lock();
	for (PSCache::iterator iter = PixelShaders.begin(); iter != PixelShaders.end(); iter++)
		iter->second.Destroy();
	PixelShaders.clear();
	PixelShadersLock.unlock();
	pixel_uid_checker.Invalidate();

	last_entry = nullptr;
}

// Used in Swap() when AA mode has changed
void PixelShaderCache::InvalidateMSAAShaders()
{
	SAFE_RELEASE(s_ColorCopyProgram[1]);
	SAFE_RELEASE(s_ColorMatrixProgram[1]);
	SAFE_RELEASE(s_DepthMatrixProgram[1]);
	SAFE_RELEASE(s_rgb8_to_rgba6[1]);
	SAFE_RELEASE(s_rgba6_to_rgb8[1]);
}

void PixelShaderCache::Shutdown()
{
	Compiler->WaitForFinish();
	SAFE_RELEASE(pscbuf);

	SAFE_RELEASE(s_ClearProgram);
	for (int i = 0; i < 2; ++i)
	{
		SAFE_RELEASE(s_ColorCopyProgram[i]);
		SAFE_RELEASE(s_ColorMatrixProgram[i]);
		SAFE_RELEASE(s_DepthMatrixProgram[i]);
		SAFE_RELEASE(s_rgba6_to_rgb8[i]);
		SAFE_RELEASE(s_rgb8_to_rgba6[i]);
	}
	
	Clear();
	g_ps_disk_cache.Sync();
	g_ps_disk_cache.Close();
}

void PixelShaderCache::PrepareShader(DSTALPHA_MODE dstAlphaMode,
	u32 components,
	const XFRegisters &xfr,
	const BPMemory &bpm, bool ongputhread)
{
	PixelShaderUid uid;
	GetPixelShaderUidD3D11(uid, dstAlphaMode, components, xfr, bpm);
	if (ongputhread)
	{
		Compiler->ProcCompilationResults();
#if defined(_DEBUG) || defined(DEBUGFAST)
		if (g_ActiveConfig.bEnableShaderDebugging)
		{
			ShaderCode code;
			GeneratePixelShaderCodeD3D11(code, dstAlphaMode, components, xfr, bpm);
			pixel_uid_checker.AddToIndexAndCheck(code, uid, "Pixel", "p");
		}
#endif
		// Check if the shader is already set
		if (last_entry)
		{
			if (uid == last_uid)
			{
				return;
			}
		}
		last_uid = uid;
		GFX_DEBUGGER_PAUSE_AT(NEXT_PIXEL_SHADER_CHANGE, true);
	}
	else
	{
		if (external_last_uid == uid)
		{
			return;
		}
		external_last_uid = uid;
	}
	PixelShadersLock.lock();
	PSCacheEntry* entry = &PixelShaders[uid];
	PixelShadersLock.unlock();
	if (ongputhread)
	{
		last_entry = entry;
	}
	// Compile only when we have a new instance
	if (entry->initialized.test_and_set())
	{
		return;
	}
	// Need to compile a new shader
	ShaderCode code;
	ShaderCompilerWorkUnit *wunit = Compiler->NewUnit(PIXELSHADERGEN_BUFFERSIZE);
	code.SetBuffer(wunit->code.data());
	GeneratePixelShaderCodeD3D11(code, dstAlphaMode, components, xfr, bpm);
	wunit->codesize = (u32)code.BufferSize();
	wunit->entrypoint = "main";
	wunit->flags = D3DCOMPILE_SKIP_VALIDATION | D3DCOMPILE_OPTIMIZATION_LEVEL3;
	wunit->target = D3D::PixelShaderVersionString();
	wunit->ResultHandler = [uid, entry](ShaderCompilerWorkUnit* wunit)
	{
		if (SUCCEEDED(wunit->cresult))
		{
			ID3DBlob* shaderBuffer = wunit->shaderbytecode;
			const u8* bytecode = (const u8*)shaderBuffer->GetBufferPointer();
			u32 bytecodelen = (u32)shaderBuffer->GetBufferSize();
			g_ps_disk_cache.Append(uid, bytecode, bytecodelen);
			PushByteCode(uid, bytecode, bytecodelen, entry);
#if defined(_DEBUG) || defined(DEBUGFAST)
			if (g_ActiveConfig.bEnableShaderDebugging)
			{
				entry->code = wunit->code.data();
			}
#endif
		}
		else
		{
			static int num_failures = 0;
			char szTemp[MAX_PATH];
			sprintf(szTemp, "%sbad_ps_%04i.txt", File::GetUserPath(D_DUMP_IDX).c_str(), num_failures++);
			std::ofstream file;
			OpenFStream(file, szTemp, std::ios_base::out);
			file << ((const char *)wunit->code.data());
			file.close();

			PanicAlert("Failed to compile pixel shader!\nThis usually happens when trying to use Dolphin with an outdated GPU or integrated GPU like the Intel GMA series.\n\nIf you're sure this is Dolphin's error anyway, post the contents of %s along with this error message at the forums.\n\nDebug info (%s):\n%s",
				szTemp,
				D3D::PixelShaderVersionString(),
				(char*)wunit->error->GetBufferPointer());
		}
	};
	Compiler->CompileShaderAsync(wunit);
}

bool PixelShaderCache::TestShader()
{
	int count = 0;
	while (!last_entry->compiled)
	{
		Compiler->ProcCompilationResults();
		if (g_ActiveConfig.bFullAsyncShaderCompilation)
		{
			break;
		}
		Common::cYield(count++);
	}
	return last_entry->shader != nullptr && last_entry->compiled;
}

void PixelShaderCache::PushByteCode(const PixelShaderUid &uid, const void* bytecode, unsigned int bytecodelen, PixelShaderCache::PSCacheEntry* entry)
{
	entry->shader = D3D::CreatePixelShaderFromByteCode(bytecode, bytecodelen);
	entry->compiled = true;
	if (entry->shader != nullptr)
	{
		// TODO: Somehow make the debug name a bit more specific
		D3D::SetDebugObjectName((ID3D11DeviceChild*)entry->shader, "a pixel shader of PixelShaderCache");
		INCSTAT(stats.numPixelShadersCreated);
		SETSTAT(stats.numPixelShadersAlive, PixelShaders.size());
	}
}

void PixelShaderCache::InsertByteCode(const PixelShaderUid &uid, const void* bytecode, unsigned int bytecodelen)
{
	PSCacheEntry* entry = &PixelShaders[uid];
	entry->initialized.test_and_set();
	PushByteCode(uid, bytecode, bytecodelen, entry);
}

// These are "callbacks" from VideoCommon and thus must be outside namespace DX11.
// This will have to be changed when we merge.

void Renderer::SetPSConstant4f(unsigned int const_number, float f1, float f2, float f3, float f4)
{
	u32 idx = const_number * 4;
	psconstants[idx++] = f1;
	psconstants[idx++] = f2;
	psconstants[idx++] = f3;
	psconstants[idx] = f4;
	pscbufchanged = true;
}

void Renderer::SetPSConstant4fv(unsigned int const_number, const float* f)
{
	u32 idx = const_number * 4;
	memcpy(&psconstants[idx], f, sizeof(float) * 4);
	pscbufchanged = true;
}

void Renderer::SetMultiPSConstant4fv(unsigned int const_number, unsigned int count, const float* f)
{
	u32 idx = const_number * 4;
	memcpy(&psconstants[idx], f, sizeof(float) * 4 * count);
	pscbufchanged = true;
}

}  // DX11
