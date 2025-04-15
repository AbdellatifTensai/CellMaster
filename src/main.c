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

        u32 CellsHorizontalCount = 6;
        u32 CellsVerticalCount = CellsHorizontalCount * 4;

        frame_buffer Frame = { .Width = 1600, .Height = 800, .Stride = 0 };
        u32 FrameBufferSize = Frame.Width * Frame.Height * sizeof(*Frame.Pixels);
        u32 InputSize = Kilobytes(1);
        u32 DebugBufferSize = Kilobytes(1);
        u32 PageFieldsWidth = CellsHorizontalCount + 1;
        u32 PageFieldsHeight = CellsVerticalCount + 1;
        u32 PageFieldsSize = PageFieldsWidth * PageFieldsHeight * sizeof(u32);
        u32 MemSize = FrameBufferSize + InputSize + DebugBufferSize + PageFieldsSize;
        u8 *Mem = (u8 *) mmap(0, MemSize, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, 0, 0);

        Frame.Pixels = (u32 *)Mem;
        char *Input = (char *)(Mem + FrameBufferSize); (void) Input;
        char *DebugBuffer = (char *)(Mem + FrameBufferSize + InputSize);
        u32 *PageFields = (u32 *)(Mem + FrameBufferSize + InputSize + DebugBufferSize);

        u32 InputLength = 0; (void) InputLength;
        u32 DebugBufferLength = 0;

        /* for(u32 Idx=0, Column=2, Row=1; Idx<FileSize; Idx++){ */
        /*         if(File[Idx] == '\n'){ */
        /*                 if(Column < PageFieldsWidth){ */
        /*                         PageFields[Row * PageFieldsWidth + Column] = Idx; */
        /*                         Column++; */
        /*                 } */
        /*                 Row++; */
        /*                 if(Row >= PageFieldsHeight) */
        /*                         break; */
        /*                 Column = 1; */
        /*         } */
        /*         else if(File[Idx] == ','){ */
        /*                 if(Column >= PageFieldsWidth) */
        /*                         continue; */
        /*                 PageFields[Row * PageFieldsWidth + Column] = Idx; */
        /*                 Column++; */
        /*         } */
        /* } */

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
                u32 CellWidth = ScreenWidth / CellsHorizontalCount ;
                u32 CellHeight = ScreenHeight / CellsVerticalCount ;
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
                                        if(SelectedCell.Column > (OffsetCell.Column + CellsHorizontalCount - 1)){
                                                OffsetCell.Column++;
#if 0
                                                for(u32 Row=1; Row<PageFieldsHeight; Row++){
                                                        b32 IsLastColumn = false;
                                                        for(u32 Column=1; Column<PageFieldsWidth-1; Column++){
                                                                u32 CurrentFieldIdx = Row * PageFieldsWidth + Column;
                                                                IsLastColumn = PageFields[CurrentFieldIdx + 1] == 0;
                                                                if(IsLastColumn){
                                                                        PageFields[CurrentFieldIdx] = 0; 
                                                                        break;
                                                                }
                                                                PageFields[CurrentFieldIdx] = PageFields[CurrentFieldIdx + 1];
                                                        }

                                                        if(!IsLastColumn){
                                                                u32 LastFieldIdx = Row * PageFieldsWidth + PageFieldsWidth - 1;
                                                                u32 LastField = PageFields[LastFieldIdx];
                                                                if(File[LastField] == '\n')
                                                                        PageFields[LastFieldIdx] = 0;
                                                                else for(u32 Idx=LastField+1; Idx<FileSize; Idx++)
                                                                             if(File[Idx] == ',' || File[Idx] == '\n'){
                                                                                     PageFields[LastFieldIdx] = Idx;
                                                                                     break;
                                                                             }
                                                        }
                                                }
#endif
                                        }

                                } break;

                                case XK_Left: {
                                        SelectedCell.Column -= SelectedCell.Column > 0? 1: 0;
                                        if(SelectedCell.Column < OffsetCell.Column){
                                                OffsetCell.Column--;
#if 0
                                                for(u32 Row=1; Row<PageFieldsHeight; Row++){
                                                        for(u32 Column=PageFieldsWidth-1; Column>1; Column--){
                                                                u32 CurrentFieldIdx = Row * PageFieldsWidth + Column;
                                                                PageFields[CurrentFieldIdx] = PageFields[CurrentFieldIdx - 1];
                                                        }

                                                        u32 LastNonEmptyFieldIdx = FirstFieldIdx;
                                                        u32 FirstFieldIdx = LastNonEmptyFieldIdx + 1;
                                                        u32 FirstField = PageFields[FirstFieldIdx];
                                                        if(!FirstField)
                                                                PageFields[FirstFieldIdx] = PageFields[LastNonEmptyFieldIdx];
                                                        
                                                        else for(u32 Idx=FirstField-1; Idx<FileSize; Idx--)
                                                                     if(File[Idx] == ',' || File[Idx] == '\n' || Idx == 0){
                                                                             PageFields[FirstFieldIdx] = Idx;
                                                                             break;
                                                                     }
                                                }

#endif
                                        }
                                } break;

                                case XK_Down: {
                                        SelectedCell.Row += 1;
                                        if(SelectedCell.Row > (OffsetCell.Row + CellsVerticalCount - 1))
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

                for(u32 Row=0; Row<CellsVerticalCount; Row++)
                        RenderRectangle(Frame, V2u(0, Row*CellHeight), V2u(ScreenWidth, Row*CellHeight + LineThickness), 0x00555555);

                for(u32 Column=0; Column<CellsHorizontalCount; Column++)
                        RenderRectangle(Frame, V2u(Column*CellWidth, 0), V2u(Column*CellWidth + LineThickness, ScreenHeight), 0x00555555);

                v2u CellPos = V2u((SelectedCell.Column - OffsetCell.Column)*CellWidth, (SelectedCell.Row - OffsetCell.Row)*CellHeight);
                RenderRectangle(Frame, CellPos, V2u(CellPos.x + CellWidth, CellPos.y + LineThickness + 1), 0x00FFFFFF);
                RenderRectangle(Frame, V2u(CellPos.x, CellPos.y + CellHeight), V2u(CellPos.x + CellWidth, CellPos.y + CellHeight + LineThickness + 1), 0x00FFFFFF);
                RenderRectangle(Frame, CellPos, V2u(CellPos.x + LineThickness + 1, CellPos.y + CellHeight), 0x00FFFFFF);
                RenderRectangle(Frame, V2u(CellPos.x + CellWidth, CellPos.y), V2u(CellPos.x + CellWidth + LineThickness + 1, CellPos.y + CellHeight), 0x00FFFFFF);

                u64 PerfBegin = __rdtsc();
                for(u32 Idx=1, Lines=0, Commas=0; Idx<FileSize; Idx++){
                        u32 BeginIdx = Idx - 1;
                        b32 IsEnd = false;
                        b32 IsLine = false;
                        for(; Idx<FileSize && !IsEnd; Idx++){
                                if(File[Idx] == ','){
                                        Commas++;
                                        IsEnd = true;
                                }
                                else if(File[Idx] == '\n'){
                                        Commas++;
                                        IsLine = true;
                                        IsEnd = true;
                                }
                        }
                        u32 Length = Idx - BeginIdx - 1;
                        RenderText(Frame, &File[BeginIdx], Length, V2u(CellWidth*(Commas - 1 - OffsetCell.Column) + TextPaddingWidth, CellHeight*(Lines - OffsetCell.Row) + TextPaddingHeight), LineThickness, 0x00FFFFFF);
                        if(IsLine){
                                Lines++;
                                Commas = 0;
                        }

                        if((Lines > (CellsVerticalCount + OffsetCell.Row)) && (Commas > (CellsHorizontalCount + OffsetCell.Column)))
                                break;
                }
                u64 PageRenderTime = __rdtsc() - PerfBegin;

#if DEBUG
                char DebugFormatStr[] =
                        "%luus\n"
                        "%.2fms %.2ffps\n"
                        /* "Sleep Time = %d\n" */
                        /* "PageRenderTime = %luc %.2fMc [%.2fms %.2fms]\n" */
                        /* "PageRenderTime = %luc %.2fMc\n" */
                        "OffsetCell = "V2U_FMT"\n"
                        "MousePos = "V2U_FMT"\n";

                /* PerformanceCounter = __rdtsc() - PerformanceCounter; */

                DebugBufferLength = stbsp_snprintf(DebugBuffer, DebugBufferSize, DebugFormatStr,
                                                   ElapsedTime,
                                                   ElapsedTime*1e-3, 1e6/ElapsedTime,
                                                   /* ElapsedTime - FrameTime, */
                                                   /* PerformanceCounter, PerformanceCounter*1e-6, */
                                                   /* PageRenderTime, PageRenderTime*1e-6, PageRenderTime/3.5*1e-6, PageRenderTime/2.6*1e-6, */
                                                   /* PageRenderTime, PageRenderTime*1e-6, */
                                                   V2U_ARG(OffsetCell),
                                                   V2U_ARG(MousePos)
                                                   );

                RenderText(Frame, DebugBuffer, DebugBufferLength, V2u(0, 16*GLYPH_HEIGHT*LineThickness), LineThickness, 0x00FF00FF);
                
                /* static char DebugPageFieldsBuffer[1024]; */
                /* u32 Max = 0; */
                /* for(u32 Idx=0; Idx<(CellsHorizontalCount*CellsVerticalCount); Idx++) */
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
