#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <error.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/time.h>
#include <x86intrin.h>

#include "platform.h"

u32 Kilobytes(i32 x){ return x * 1024; }

b32 CheckFileExtension(char *FileName, char *Extension, i32 Length){
        char *P = FileName;
        while(*++P != '\0');
        P -= Length;

        for(i32 Idx=0; Idx<Length; Idx++){
                if(P[Idx] != Extension[Idx])
                        return false;
        }
        return true;
}

i32 main(i32 ArgCount, char **Args){
        if(ArgCount < 2){
                printf("USAGE: ./main <file.csv>\n");
                return 1;
        }

        char *FileName = Args[1]; if(!CheckFileExtension(FileName, "csv", 3)){printf("The file '%s' is not a CSV file\n", FileName);
                return 1;
        }

        i32 Fd = open(FileName, O_RDONLY);
        if(Fd < 0){
                error(1, errno, "Could not load file '%s'", FileName);
                /* return 1; */
        }

        struct stat Stats;
        fstat(Fd, &Stats);
        u32 FileSize = Stats.st_size;
        char *File = (char *) mmap(0, FileSize, PROT_READ, MAP_PRIVATE, Fd, 0);
        close(Fd);

        // TODO: find a better way other than two passes
        u32 LinesCount = 0; 
        for(u32 Idx=0; Idx<FileSize; Idx++)
                if(File[Idx] == '\n')
                        LinesCount++;

        v2u CellsCount = V2u(6, 24);

        frame_buffer Frame = { .Width = 1600, .Height = 800, .Stride = 0 };
        str_slice Input = EMPTY_STR_SLICE;
        str_slice DebugBuffer = EMPTY_STR_SLICE;
        u32_slice LinesIdx = EMPTY_U32_SLICE;

        u32 FrameBufferSize = Frame.Width * Frame.Height * sizeof(*Frame.Pixels);
        u32 InputSize = Kilobytes(1);
        u32 DebugBufferSize = Kilobytes(1);
        u32 LinesIdxSize = (LinesCount + 1) * sizeof(u32);
        u32 MemSize = FrameBufferSize + InputSize + DebugBufferSize + LinesIdxSize;
        u8 *Mem = (u8 *) mmap(0, MemSize, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, 0, 0);

        Frame.Pixels = (u32 *)Mem;
        Input.Data = (char *)(Mem + FrameBufferSize); (void) Input;
        DebugBuffer.Data = (char *)(Mem + FrameBufferSize + InputSize);
        LinesIdx.Data = (u32 *)(Mem + FrameBufferSize + InputSize + DebugBufferSize);

        Input.Count = 0; (void) Input;
        DebugBuffer.Count = 0;
        LinesIdx.Count = LinesCount + 1;

        LinesIdx.Data[0] = 0;
        for(u32 Idx=0, LineIdx=1; Idx<FileSize; Idx++)
                if(File[Idx] == '\n')
                        LinesIdx.Data[LineIdx++] = Idx;

        InitWindow(Frame, "Minicel");

        /* f32 Factor = 1.0f; */
        v2u OffsetCell = V2u(0, 0);
        v2u SelectedCell = V2u(0, 0);
        v2u MousePos = V2u(0, 0);

        struct timeval Timer;
        gettimeofday(&Timer, 0);
        /* u64 PerformanceCounter = __rdtsc(); */
        u32 TargetFPS = 60;
        u32 FrameTime = (u32)(1e6/TargetFPS);

        x11_keypress Key = {0};
        x11_event Event = {0};
        b32 IsQuit = false;
        while(!IsQuit){

                u32 ScreenWidth = Frame.Width;
                u32 ScreenHeight = Frame.Height;
                u32 CellWidth = ScreenWidth / CellsCount.Width ;
                u32 CellHeight = ScreenHeight / CellsCount.Height ;
                u32 TextPaddingWidth = CellWidth / 16;
                u32 TextPaddingHeight = CellHeight / 4;
                f32 LineThickness = 3.0f; // 1/Factor >= 1.0? 1/Factor: 1.0f;

                u64 ElapsedTime = (u64)(Timer.tv_sec + Timer.tv_usec);

                while(IsEventPending()){
                        EventNext(&Event);
                        switch(Event.type){

                        case ClientMessage:{
                                IsQuit = IsEventCloseWindow(&Event);
                        } break;

                        case ButtonPress:{
                                MousePos = V2u(CLAMP(Event.xbutton.x, 0, Frame.Width), CLAMP(Event.xbutton.y, 0, Frame.Height));
                                if(Event.xbutton.button == Button1){
                                        SelectedCell.Column = Floor((f32)MousePos.x/CellWidth);
                                        SelectedCell.Column += OffsetCell.Column;
                                        SelectedCell.Row = Floor((f32)MousePos.y/CellHeight);
                                        SelectedCell.Row += OffsetCell.Row;
                                }
                        } break;

                        case KeyPress:{

                                Key = GetKeyPress(Event);

                                switch(Key.Symbol){
                                case XK_plus:{
                                        /* if(Key.State & ControlMask) */
                                        /*         Factor += 1.0F / 16.0F; */
                                } break;

                                case XK_minus:{
                                        /* if(Key.State & ControlMask){ */
                                        /*         Factor -= 1.0F / 16.0F; */
                                        /*         Factor = Factor <= 0.125f? 0.125f: Factor; */
                                        /* } */
                                } break;

                                case XK_BackSpace:{
                                } break;

                                case XK_Right: {
                                        SelectedCell.Column += 1;
                                        if(SelectedCell.Column > (OffsetCell.Column + CellsCount.Width - 1))
                                                OffsetCell.Column++;
                                } break;

                                case XK_Left: {
                                        SelectedCell.Column -= SelectedCell.Column > 0? 1: 0;
                                        if(SelectedCell.Column < OffsetCell.Column)
                                                OffsetCell.Column--;
                                } break;

                                case XK_Down: {
                                        SelectedCell.Row += 1;
                                        if(SelectedCell.Row > (OffsetCell.Row + CellsCount.Height - 1))
                                                OffsetCell.Row++;

                                } break;
                                        
                                case XK_Up: {
                                        SelectedCell.Row -= SelectedCell.Row > 0? 1: 0;
                                        if(SelectedCell.Row < OffsetCell.Row)
                                                OffsetCell.Row--;

                                } break;
                               
                                default:{
                                        /* if(Key.Symbol > 128) break; */
                                        /* if(InputLength < InputSize) */
                                        /*         Input[InputLength++] = (char) Key.Symbol; */
                                } break;
                                }

                        } break;

                        case Expose: DrawImage(); break;
                        }
                }

                gettimeofday(&Timer, 0);
                ElapsedTime = (u64)(Timer.tv_sec + Timer.tv_usec) - ElapsedTime;
        
                if(ElapsedTime < FrameTime)
                        usleep(FrameTime - ElapsedTime);

                RenderBackground(Frame, 0x00181818);

                for(u32 Row=0; Row<CellsCount.Height; Row++)
                        RenderRectangle(Frame, V2u(0, Row*CellHeight), V2u(ScreenWidth, Row*CellHeight + LineThickness), 0x00555555);

                for(u32 Column=0; Column<CellsCount.Width; Column++)
                        RenderRectangle(Frame, V2u(Column*CellWidth, 0), V2u(Column*CellWidth + LineThickness, ScreenHeight), 0x00555555);

                v2u CellPos = V2u((SelectedCell.Column - OffsetCell.Column)*CellWidth, (SelectedCell.Row - OffsetCell.Row)*CellHeight);
                RenderRectangle(Frame, CellPos, V2u(CellPos.x + CellWidth, CellPos.y + LineThickness + 1), 0x00FFFFFF);
                RenderRectangle(Frame, V2u(CellPos.x, CellPos.y + CellHeight), V2u(CellPos.x + CellWidth, CellPos.y + CellHeight + LineThickness + 1), 0x00FFFFFF);
                RenderRectangle(Frame, CellPos, V2u(CellPos.x + LineThickness + 1, CellPos.y + CellHeight), 0x00FFFFFF);
                RenderRectangle(Frame, V2u(CellPos.x + CellWidth, CellPos.y), V2u(CellPos.x + CellWidth + LineThickness + 1, CellPos.y + CellHeight), 0x00FFFFFF);

#if DEBUG
                u64 PageRenderTime = __rdtsc();
#endif

                for(u32 LineIdx=OffsetCell.Row, CellIdx=0; CellIdx<CellsCount.Height && LineIdx<LinesIdx.Count; LineIdx++, CellIdx++){
                        u32 BeginLineIdx = LinesIdx.Data[LineIdx];
                        u32 BeginFieldIdx = BeginLineIdx == 0? 0: BeginLineIdx+1;
                        u32 EndFieldIdx = BeginFieldIdx + 1;
                        u32 Columns = 0;
                        for(; EndFieldIdx<FileSize; EndFieldIdx++){
                                if(File[EndFieldIdx] == ',' || File[EndFieldIdx] == '\n'){
                                        Columns++;
                                        if(Columns >= OffsetCell.Column){
                                                u32 Length = EndFieldIdx - BeginFieldIdx;
                                                RenderText(Frame, &File[BeginFieldIdx], Length, V2u(CellWidth*(Columns - 1 - OffsetCell.Column) + TextPaddingWidth, CellHeight*(LineIdx - OffsetCell.Row) + TextPaddingHeight), 1, 0x00FFFFFF);
                                        }
                                        if(Columns >= CellsCount.Width)
                                                break;
                                        BeginFieldIdx = EndFieldIdx + 1;
                                }
                                if(File[EndFieldIdx] == '\n')
                                        break;
                        }

                }
#if DEBUG
                PageRenderTime = __rdtsc() - PageRenderTime;

                char DebugFormatStr[] =
                        "%luus\n"
                        "%.2fms %.2ffps\n"
                        /* "Sleep Time = %d\n" */
                        /* "PageRenderTime = %luc %.2fMc [%.2fms %.2fms]\n" */
                        "PageRenderTime = %luc %.2fMc\n"
                        "OffsetCell = "V2U_FMT"\n"
                        "MousePos = "V2U_FMT"\n";

                /* PerformanceCounter = __rdtsc() - PerformanceCounter; */

                DebugBuffer.Count = stbsp_snprintf(DebugBuffer.Data, DebugBufferSize, DebugFormatStr,
                                                   ElapsedTime,
                                                   ElapsedTime*1e-3, 1e6/ElapsedTime,
                                                   /* ElapsedTime - FrameTime, */
                                                   /* PerformanceCounter, PerformanceCounter*1e-6, */
                                                   /* PageRenderTime, PageRenderTime*1e-6, PageRenderTime/3.5*1e-6, PageRenderTime/2.6*1e-6, */
                                                   PageRenderTime, PageRenderTime*1e-6,
                                                   V2U_ARG(OffsetCell),
                                                   V2U_ARG(MousePos)
                                                   );

                RenderText(Frame, DebugBuffer.Data, DebugBuffer.Count, V2u(0, 16*GLYPH_HEIGHT*LineThickness), LineThickness, 0x00FF00FF);
                
                /* static char DebugPageFieldsBuffer[1024]; */
                /* u32 Max = 0; */
                /* for(u32 Idx=0; Idx<(CellsCount.Width*CellsCount.Height); Idx++) */
                /*         Max = MAX(Max, PageFields[Idx]); */
                /* i32 Pad = stbsp_snprintf(DebugPageFieldsBuffer, 1024, "%u", Max); */
                /* i32 DebugPageFieldsBufferLength = 0; */
                /* for(u32 Row=0; Row<PageFieldsHeight; Row++){ */
                /*         for(u32 Col=0; Col<PageFieldsWidth; Col++) */
                /*                 DebugPageFieldsBufferLength += stbsp_snprintf(&DebugPageFieldsBuffer[DebugPageFieldsBufferLength], 1024, "%*u ", Pad, PageFields[Row * PageFieldsWidth + Col]); */
                /*         DebugPageFieldsBufferLength += stbsp_snprintf(&DebugPageFieldsBuffer[DebugPageFieldsBufferLength], 1024, "\n"); */
                /* } */

                /* RenderText(Frame, DebugPageFieldsBuffer, DebugPageFieldsBufferLength, V2u(800, 300), 2, 0x00FF00FF); */
#endif

                DrawImage(Frame);

        }
        
        /* CloseWindow(); */
        /* munmap(File, FileSize); */
        /* munmap(Mem, MemSize); */

        return 0;
}
