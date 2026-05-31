// screen.cpp — УКНЦ planar→RGB32 renderer, lifted verbatim from upstream
// Emulator.cpp (Emulator_PrepareScreenRGB32) + qscreen.cpp palette. No Qt.
#include "stdafx.h"
#include "emubase/Board.h"
extern CMotherboard* g_pBoard;
extern bool g_okEmulatorInitialized;

extern const quint32 ScreenView_StandardRGBColors[16 * 8] =
{
    0xFF000000, 0xFF000080, 0xFF008000, 0xFF008080, 0xFF800000, 0xFF800080, 0xFF808000, 0xFF808080,
    0xFF000000, 0xFF0000FF, 0xFF00FF00, 0xFF00FFFF, 0xFFFF0000, 0xFFFF00FF, 0xFFFFFF00, 0xFFFFFFFF,
    0xFF000000, 0xFF000060, 0xFF008000, 0xFF008060, 0xFF800000, 0xFF800060, 0xFF808000, 0xFF808060,
    0xFF000000, 0xFF0000DF, 0xFF00FF00, 0xFF00FFDF, 0xFFFF0000, 0xFFFF00DF, 0xFFFFFF00, 0xFFFFFFDF,
    0xFF000000, 0xFF000080, 0xFF006000, 0xFF006080, 0xFF800000, 0xFF800080, 0xFF806000, 0xFF806080,
    0xFF000000, 0xFF0000FF, 0xFF00DF00, 0xFF00DFFF, 0xFFFF0000, 0xFFFF00FF, 0xFFFFDF00, 0xFFFFDFFF,
    0xFF000000, 0xFF000060, 0xFF006000, 0xFF006060, 0xFF800000, 0xFF800060, 0xFF806000, 0xFF806060,
    0xFF000000, 0xFF0000DF, 0xFF00DF00, 0xFF00DFDF, 0xFFFF0000, 0xFFFF00DF, 0xFFFFDF00, 0xFFFFDFDF,
    0xFF000000, 0xFF000080, 0xFF008000, 0xFF008080, 0xFF600000, 0xFF600080, 0xFF608000, 0xFF608080,
    0xFF000000, 0xFF0000FF, 0xFF00FF00, 0xFF00FFFF, 0xFFDF0000, 0xFFDF00FF, 0xFFDFFF00, 0xFFDFFFFF,
    0xFF000000, 0xFF000060, 0xFF008000, 0xFF008060, 0xFF600000, 0xFF600060, 0xFF608000, 0xFF608060,
    0xFF000000, 0xFF0000DF, 0xFF00FF00, 0xFF00FFDF, 0xFFDF0000, 0xFFDF00DF, 0xFFDFFF00, 0xFFDFFFDF,
    0xFF000000, 0xFF000080, 0xFF006000, 0xFF006080, 0xFF600000, 0xFF600080, 0xFF606000, 0xFF606080,
    0xFF000000, 0xFF0000FF, 0xFF00DF00, 0xFF00DFFF, 0xFFDF0000, 0xFFDF00FF, 0xFFDFDF00, 0xFFDFDFFF,
    0xFF000000, 0xFF000060, 0xFF006000, 0xFF006060, 0xFF600000, 0xFF600060, 0xFF606000, 0xFF606060,
    0xFF000000, 0xFF0000DF, 0xFF00DF00, 0xFF00DFDF, 0xFFDF0000, 0xFFDF00DF, 0xFFDFDF00, 0xFFDFDFDF,
};

void Emulator_PrepareScreenRGB32(void* pImageBits, const quint32* colors)
{
    if (pImageBits == nullptr) return;
    if (!g_okEmulatorInitialized) return;

    // Tag parsing loop
    quint8 cursorYRGB = 0;
    bool okCursorType = false;
    quint8 cursorPos = 128;
    bool cursorOn = false;
    quint8 cursorAddress = 0;  // Address of graphical cursor
    quint16 address = 0000270;  // Tag sequence start address
    bool okTagSize = false;  // Tag size: true - 4-word, false - 2-word (first tag is always 2-word)
    bool okTagType = false;  // Type of 4-word tag: true - set palette, false - set params
    int scale = 1;           // Horizontal scale: 1, 2, 4, or 8
    quint32 palette = 0;       // Palette
    qint32 palettecurrent[8]; // Current palette; update each time we change the "palette" variable
    for (int i = 0; i < 8; i++)
        palettecurrent[i] = 0xFF0000000;
    quint8 pbpgpr = 0;         // 3-bit Y-value modifier
    for (int yy = 0; yy < 307; yy++)
    {
        if (okTagSize)  // 4-word tag
        {
            quint16 tag1 = g_pBoard->GetRAMWord(0, address);
            address += 2;
            quint16 tag2 = g_pBoard->GetRAMWord(0, address);
            address += 2;

            if (okTagType)  // 4-word palette tag
            {
                palette = ((quint32)tag1) | ((quint32)tag2 << 16);
            }
            else  // 4-word params tag
            {
                scale = (tag2 >> 4) & 3;  // Bits 4-5 - new scale value
                pbpgpr = (quint8)((7 - (tag2 & 7)) << 4);  // Y-value modifier
                cursorYRGB = (quint8)(tag1 & 15);  // Cursor color
                okCursorType = ((tag1 & 16) != 0);  // true - graphical cursor, false - symbolic cursor
                //ASSERT(okCursorType==0);  //DEBUG
                cursorPos = (quint8)(((tag1 >> 8) >> scale) & 0x7f);  // Cursor position in the line
                cursorAddress = (quint8)((tag1 >> 5) & 7);
                scale = 1 << scale;
            }
            for (uint8_t c = 0; c < 8; c++)  // Update palettecurrent
            {
                quint8 valueYRGB = (uint8_t) (palette >> (c << 2)) & 15;
                palettecurrent[c] = colors[pbpgpr | valueYRGB];
                //if (pbpgpr != 0) DebugLogFormat("pbpgpr %02x\r\n", pbpgpr | valueYRGB);
            }
        }

        quint16 addressBits = g_pBoard->GetRAMWord(0, address);  // The word before the last word - is address of bits from all three memory planes
        address += 2;

        // Calculate size, type and address of the next tag
        quint16 tagB = g_pBoard->GetRAMWord(0, address);  // Last word of the tag - is address and type of the next tag
        okTagSize = (tagB & 2) != 0;  // Bit 1 shows size of the next tag
        if (okTagSize)
        {
            address = tagB & ~7;
            okTagType = (tagB & 4) != 0;  // Bit 2 shows type of the next tag
        }
        else
            address = tagB & ~3;
        if ((tagB & 1) != 0)
            cursorOn = !cursorOn;

        // Draw bits into the bitmap, from line 20 to line 307
        if (yy < 19 /*|| yy > 306*/)
            continue;

        // Loop thru bits from addressBits, planes 0,1,2
        // For each pixel:
        //   Get bit from planes 0,1,2 and make value
        //   Map value to palette; result is 4-bit value YRGB
        //   Translate value to 24-bit RGB
        //   Put value to m_bits; repeat using scale value

        int xr = 640;
        int y = yy - 19;
        quint32* pBits = (static_cast<quint32*>(pImageBits)) + y * 640;
        int pos = 0;
        for (;;)
        {
            // Get bit from planes 0,1,2
            quint8 src0 = g_pBoard->GetRAMByte(0, addressBits);
            quint8 src1 = g_pBoard->GetRAMByte(1, addressBits);
            quint8 src2 = g_pBoard->GetRAMByte(2, addressBits);
            // Loop through the bits of the byte
            int bit = 0;
            for (;;)
            {
                quint32 valueRGB;
                if (cursorOn && (pos == cursorPos) && (!okCursorType || (okCursorType && bit == cursorAddress)))
                    valueRGB = colors[cursorYRGB];  // 4-bit to 32-bit color
                else
                {
                    // Make 3-bit value from the bits
                    quint8 value012 = (src0 & 1) | ((src1 & 1) << 1) | ((src2 & 1) << 2);
                    valueRGB = palettecurrent[value012];  // 3-bit to 32-bit color
                }

                // Put value to m_bits; repeat using scale value
                //WAS: for (int s = 0; s < scale; s++) *pBits++ = valueRGB;
                switch (scale)
                {
                case 8:
                    *pBits++ = valueRGB;
                    *pBits++ = valueRGB;
                    *pBits++ = valueRGB;
                    *pBits++ = valueRGB;
                    /* FALLTHRU */
                case 4:
                    *pBits++ = valueRGB;
                    *pBits++ = valueRGB;
                    /* FALLTHRU */
                case 2:
                    *pBits++ = valueRGB;
                    /* FALLTHRU */
                case 1:
                    *pBits++ = valueRGB;
                    /* FALLTHRU */
                default:
                    break;
                }

                xr -= scale;

                if (bit == 7)
                    break;
                bit++;

                // Shift to the next bit
                src0 >>= 1;
                src1 >>= 1;
                src2 >>= 1;
            }
            if (xr <= 0)
                break;  // End of line
            addressBits++;  // Go to the next byte
            pos++;
        }
    }
}
