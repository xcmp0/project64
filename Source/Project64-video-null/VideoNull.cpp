// Project64-video-null: a graphics plugin that renders nothing, for runs that
// need no picture (CPU and audio tests, the sync CPU check).
//
// ReadScreen returns what the video interface shows from RDRAM, so frames
// drawn by the CPU, or by an RSP and RDP that write RDRAM, can still be
// dumped. Display lists and RDP command lists sent here are dropped, but
// their end is signalled as an RDP full sync would, so the game goes on.

#include <Project64-plugin-spec/Video.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace
{
// The core maps RDRAM with room to spare; the VI never reads past 8 MB
const uint32_t RdramLimit = 0x800000;

const uint32_t MI_INTR_DP = 0x20;
const uint32_t DPC_STATUS_XBUS_DMEM_DMA = 0x0001;
const uint32_t RDP_FULL_SYNC = 0x29;

GFX_INFO g_GfxInfo;

void FullSync(void)
{
    *g_GfxInfo.MI_INTR_REG |= MI_INTR_DP;
    g_GfxInfo.CheckInterrupts();
}

// A pixel of the frame buffer as 0xRRGGBB. RDRAM is byte-swapped by 32-bit
// word: a 16-bit pixel at N64 address a lies at a ^ 2.
uint32_t Pixel(uint32_t Address, bool Is32Bit)
{
    if (Is32Bit)
    {
        uint32_t Value = *(const uint32_t *)(g_GfxInfo.RDRAM + Address);
        return Value >> 8;
    }
    uint32_t Value = *(const uint16_t *)(g_GfxInfo.RDRAM + (Address ^ 2));
    uint32_t Red = (Value >> 11) & 0x1F, Green = (Value >> 6) & 0x1F, Blue = (Value >> 1) & 0x1F;
    return ((Red << 3 | Red >> 2) << 16) | ((Green << 3 | Green >> 2) << 8) | (Blue << 3 | Blue >> 2);
}
} // namespace

EXPORT void CALL GetDllInfo(PLUGIN_INFO * PluginInfo)
{
    PluginInfo->Version = VIDEO_SPECS_VERSION;
    PluginInfo->Type = PLUGIN_TYPE_VIDEO;
    snprintf(PluginInfo->Name, sizeof(PluginInfo->Name), "Project64 null video");
    PluginInfo->Reserved1 = false;
    PluginInfo->Reserved2 = true;
}

EXPORT int CALL InitiateGFX(GFX_INFO Gfx_Info)
{
    g_GfxInfo = Gfx_Info;
    return true;
}

EXPORT void CALL PluginLoaded(void)
{
}

EXPORT void CALL RomOpen(void)
{
}

EXPORT void CALL RomClosed(void)
{
}

EXPORT void CALL CloseDLL(void)
{
}

EXPORT void CALL DllAbout(void * /*hParent*/)
{
}

EXPORT void CALL ChangeWindow(void)
{
}

EXPORT void CALL DrawScreen(void)
{
}

EXPORT void CALL MoveScreen(int /*xpos*/, int /*ypos*/)
{
}

// A display list ends with a full sync
EXPORT void CALL ProcessDList(void)
{
    FullSync();
}

// RDP commands from DPC_CURRENT to DPC_END, in RDRAM or DMEM: all taken; a
// full sync as the last command raises the DP interrupt
EXPORT void CALL ProcessRDPList(void)
{
    uint32_t End = *g_GfxInfo.DPC_END_REG;
    if (End >= *g_GfxInfo.DPC_CURRENT_REG + 8)
    {
        bool InDmem = (*g_GfxInfo.DPC_STATUS_REG & DPC_STATUS_XBUS_DMEM_DMA) != 0;
        uint32_t Address = InDmem ? ((End - 8) & 0xFF8) : ((End - 8) & 0xFFFFF8);
        const uint8_t * Memory = InDmem ? g_GfxInfo.DMEM : g_GfxInfo.RDRAM;
        uint32_t Command = *(const uint32_t *)(Memory + Address);
        if (Address < (InDmem ? 0x1000 : RdramLimit) && ((Command >> 24) & 0x3F) == RDP_FULL_SYNC)
        {
            FullSync();
        }
    }
    *g_GfxInfo.DPC_START_REG = End;
    *g_GfxInfo.DPC_CURRENT_REG = End;
}

EXPORT void CALL ShowCFB(void)
{
}

EXPORT void CALL UpdateScreen(void)
{
}

EXPORT void CALL ViStatusChanged(void)
{
}

EXPORT void CALL ViWidthChanged(void)
{
}

EXPORT void CALL CaptureScreen(const char * /*Directory*/)
{
}

// The frame as 24-bit BGR rows, bottom row first (as glReadPixels gives it);
// free it with DllCrtFree
EXPORT void CALL ReadScreen(void ** Dest, long * Width, long * Height)
{
    *Dest = nullptr;
    *Width = 0;
    *Height = 0;
    uint32_t Type = *g_GfxInfo.VI_STATUS_REG & 3;
    uint32_t Origin = *g_GfxInfo.VI_ORIGIN_REG & 0xFFFFFF;
    uint32_t Stride = *g_GfxInfo.VI_WIDTH_REG & 0xFFF;
    uint32_t HStart = (*g_GfxInfo.VI_H_START_REG >> 16) & 0x3FF, HEnd = *g_GfxInfo.VI_H_START_REG & 0x3FF;
    uint32_t VStart = (*g_GfxInfo.VI_V_START_REG >> 16) & 0x3FF, VEnd = *g_GfxInfo.VI_V_START_REG & 0x3FF;
    uint32_t XScale = *g_GfxInfo.VI_X_SCALE_REG & 0xFFF, YScale = *g_GfxInfo.VI_Y_SCALE_REG & 0xFFF;
    if (Type < 2 || Stride == 0 || HEnd <= HStart || VEnd <= VStart)
    {
        return;
    }
    // Visible size: the active area scaled by the VI's 2.10 fixed-point factors
    uint32_t OutWidth = ((HEnd - HStart) * XScale) >> 10;
    uint32_t OutHeight = (((VEnd - VStart) >> 1) * YScale) >> 10;
    if (OutWidth > Stride)
    {
        OutWidth = Stride;
    }
    bool Is32Bit = Type == 3;
    uint32_t PixelSize = Is32Bit ? 4 : 2;
    if (OutWidth == 0 || OutHeight == 0 || Origin + OutHeight * Stride * PixelSize > RdramLimit)
    {
        return;
    }
    uint8_t * Image = (uint8_t *)malloc(OutWidth * OutHeight * 3);
    if (Image == nullptr)
    {
        return;
    }
    for (uint32_t y = 0; y < OutHeight; y++)
    {
        uint8_t * Row = Image + (OutHeight - 1 - y) * OutWidth * 3;
        for (uint32_t x = 0; x < OutWidth; x++)
        {
            uint32_t Color = Pixel(Origin + (y * Stride + x) * PixelSize, Is32Bit);
            Row[x * 3] = (uint8_t)Color;
            Row[x * 3 + 1] = (uint8_t)(Color >> 8);
            Row[x * 3 + 2] = (uint8_t)(Color >> 16);
        }
    }
    *Dest = Image;
    *Width = OutWidth;
    *Height = OutHeight;
}

EXPORT void CALL DllCrtFree(void * Address)
{
    free(Address);
}
