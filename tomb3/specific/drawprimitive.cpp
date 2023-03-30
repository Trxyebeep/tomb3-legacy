#include "../tomb3/pch.h"
#include "drawprimitive.h"
#include "winmain.h"
#include "hwrender.h"

static LPDIRECT3DDEVICEX D3DDev;

HRESULT HWDrawPrimitive(D3DPRIMITIVETYPE dptPrimitiveType, D3DVERTEXTYPE dwVertexTypeDesc, LPVOID lpvVertices, ulong dwVertexCount, ulong dwFlags)
{
	return D3DDev->DrawPrimitive(dptPrimitiveType, dwVertexTypeDesc, lpvVertices, dwVertexCount, dwFlags);
}

HRESULT HWBeginScene()
{
	WinFrameRate();
	return D3DDev->BeginScene();
}

HRESULT HWEndScene()
{
	return D3DDev->EndScene();
}

HRESULT HWSetRenderState(D3DRENDERSTATETYPE dwRenderStateType, ulong dwRenderState)
{
	return D3DDev->SetRenderState(dwRenderStateType, dwRenderState);
}

void InitDrawPrimitive(LPDIRECT3DDEVICEX lpD3DDev, LPDIRECTDRAWSURFACEX surf, bool hw)
{
	D3DDev = lpD3DDev;

	if (hw)
	{
		DrawPrimitive = HWDrawPrimitive;
		BeginScene = HWBeginScene;
		EndScene = HWEndScene;
		SetRenderState = HWSetRenderState;
	}
	else	//obv software isn't empty in the original codebase.
	{
		if (App.DXConfig.MMX)
		{

		}
		else
		{

		}
	}
}

void CloseDrawPrimitive()
{
	//does nothing for HWR.
}
