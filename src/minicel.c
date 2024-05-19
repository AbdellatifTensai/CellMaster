#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <raylib.h>

typedef size_t usize;
typedef ssize_t ssize;
typedef float f32;
typedef double f64;
typedef uint32_t u32;
typedef int32_t s32;

typedef struct{
	char *Data;
	usize Length;
} string;

#define STR(str) (string){(str), sizeof((str))-1}

static const string EMPTY_STRING = {"", 0};

typedef enum { NODE_TYPE_EMPTY, NODE_TYPE_NUM, NODE_TYPE_EXPR, NODE_TYPE_UNI_OP, NODE_TYPE_BIN_OP, NODE_TYPE_COUNT } node_type;
typedef struct node{
	struct node *Left, *Right;
	node_type NodeType;
	union{ string Op; f64 Num; };
} node;

static node EMPTY_NODE = { .Left =  &EMPTY_NODE, .Right = &EMPTY_NODE, .NodeType = NODE_TYPE_EMPTY };

typedef struct{
	char *Data;
	usize *Length;   //ajoint to Data, for performance
	usize RowsCount;
	usize ColumnsCount;
	usize DataCapacity;
	usize CellSize;
	usize RowsCapacity;
	usize ColumnsCapacity;
} cells;

void PrintString(const string Input){
	for(usize CharIndex=0; CharIndex<Input.Length; CharIndex++)
		putchar(Input.Data[CharIndex]);
}

void CopyString(string From, string To){
	assert(From.Length <= To.Length);
	memcpy(From.Data, To.Data, From.Length);
	To.Length = From.Length;
}

void Split(string Input, string Delims, string *OutStrings, usize *OutStringsCount){
	OutStrings[0].Data = &Input.Data[0];

	usize StringsIndex = 1;
	usize OldInputIndex = 0;
	for(usize InputIndex=0; InputIndex<Input.Length; InputIndex++)
		for(usize DelimIndex=0; DelimIndex<Delims.Length; DelimIndex++)
			if(Input.Data[InputIndex] == Delims.Data[DelimIndex]){
				if(InputIndex == 0) //if Delim is first char
					OutStrings[StringsIndex - 1].Length = 1;
				
				else{
					OutStrings[StringsIndex - 1].Length = InputIndex - OldInputIndex; //close the prev string

					//if include delim true
					OutStrings[StringsIndex].Data = &Input.Data[InputIndex]; //start the delim string
					OutStrings[StringsIndex].Length = 1; //close the delim string
					StringsIndex++;
				}

				OutStrings[StringsIndex].Data = &Input.Data[InputIndex + 1]; //start the next string
				StringsIndex++;

				InputIndex++;
				OldInputIndex = InputIndex;
				break;
			}

	OutStrings[StringsIndex - 1].Length = Input.Length - OldInputIndex;
	*OutStringsCount = StringsIndex;
}

void Trim(string *Input){
	string Temp;
	char *CurrentCharacter = Input->Data;
	while(*CurrentCharacter++ == ' ' && CurrentCharacter < (Input->Data + Input->Length));

	Temp.Data = CurrentCharacter - 1;

	CurrentCharacter = Input->Data + Input->Length - 1;
	while(*CurrentCharacter == ' ' && CurrentCharacter >= Temp.Data)
		CurrentCharacter--;
	
	Temp.Length = CurrentCharacter - Temp.Data + 1;
	*Input = Temp;
}

void CountRowsColumns(cells *Cells, const string Input, char delim){
	usize NewLineCount = 0;
	usize MaxDelimCount = 0;
	usize CurrentDelimCount = 0;
	for(usize CharIndex=0; CharIndex<Input.Length; CharIndex++){
		if(Input.Data[CharIndex] == '\n'){
			NewLineCount++;
			if(CurrentDelimCount > MaxDelimCount){
				MaxDelimCount = CurrentDelimCount;
				CurrentDelimCount = 0;
			}
		}
		else if(Input.Data[CharIndex] == delim)
			CurrentDelimCount++;
	}

	Cells->RowsCount = NewLineCount;
	Cells->ColumnsCount = MaxDelimCount + 1;
}

void CellSet(cells Cells, string Cell, usize Row, usize Column){
	usize CellIndex = (Row * Cells.ColumnsCount + Column) * Cells.CellSize;
	usize LengthIndex = (Row * Cells.ColumnsCount + Column) * (Cells.CellSize / sizeof(*Cells.Length));
	memcpy(&Cells.Data[CellIndex], Cell.Data, Cell.Length);
	Cells.Length[LengthIndex] = Cell.Length;
}

string CellGet(cells Cells, usize Row, usize Column){
	usize CellIndex = (Row * Cells.ColumnsCount + Column) * Cells.CellSize;
	usize LengthIndex = (Row * Cells.ColumnsCount + Column) * (Cells.CellSize / sizeof(*Cells.Length));
	return (string){ .Data = &Cells.Data[CellIndex], .Length = Cells.Length[LengthIndex] };
}

void CellErase(cells Cells, usize Row, usize Column){
	usize CellIndex = (Row * Cells.ColumnsCount + Column) * Cells.CellSize;
	usize LengthIndex = (Row * Cells.ColumnsCount + Column) * (Cells.CellSize / sizeof(*Cells.Length));
	memset(&Cells.Data[CellIndex], 0, Cells.Length[LengthIndex] * sizeof(*Cells.Data));
	Cells.Length[LengthIndex] = 0;
}

void FillCells(cells Cells, const string Input, char Delim){
	for(usize InputIndex = 0, FieldLength = 0, Row = 0, Column = 0; InputIndex<Input.Length; InputIndex++, FieldLength++){
		if(Input.Data[InputIndex] == Delim){
			string Field = (string){ .Data = &Input.Data[InputIndex - FieldLength], .Length = FieldLength }; 
			CellSet(Cells, Field, Row, Column);

			FieldLength = 0;
			InputIndex++;
			Column++;
		}

		else if(Input.Data[InputIndex] == '\n'){
			string Field = (string){ .Data = &Input.Data[InputIndex - FieldLength], .Length = FieldLength }; 
			CellSet(Cells, Field, Row, Column);

			FieldLength = 0;
			InputIndex++;
			Row++;
			Column = 0;
		}
	}
}

f64 StrToNum(const string Expr){
	f64 Num = 0;
	u32 Frac = 1;
	bool Point = false;
	for(usize CharIndex=0; CharIndex<Expr.Length; CharIndex++){
		char CurrentChar = Expr.Data[CharIndex];
		if(CurrentChar == '.'){ Point = true; continue; }
		if(Point) Frac *= 10.0f;
		Num = Num * 10.f + (f64)(CurrentChar - '0');
	}
	return Num / Frac;
}

bool IsNumber(const string Expr){
	if(Expr.Length == 0) return false;
	for(usize CharIndex=0; CharIndex<Expr.Length; CharIndex++)
		if(!(('0' <= Expr.Data[CharIndex] && Expr.Data[CharIndex] <= '9') || Expr.Data[CharIndex] == '.'))
			return false;
	return true;
}

bool IsPrimaryOp(const string Expr){
	if(Expr.Length == 0) return false;
	for(usize CharIndex=0; CharIndex<Expr.Length; CharIndex++)
		if(Expr.Data[CharIndex] == '*' || Expr.Data[CharIndex] == '/')
			return true;
	return false;
}

bool IsSecondaryOp(const string Expr){
	if(Expr.Length == 0) return false;
	for(usize CharIndex=0; CharIndex<Expr.Length; CharIndex++)
		if(Expr.Data[CharIndex] == '-' || Expr.Data[CharIndex] == '+')
			return true;
	return false;
}

void Factor(const string *Tokens, usize *TokenIndex, node *Nodes, usize *NodeIndex, usize CurrentNodeIndex){
	string CurrentToken = Tokens[*TokenIndex];
	if(IsNumber(CurrentToken)){
		(*TokenIndex)++;
		Nodes[CurrentNodeIndex] = (node){
			.Left = &EMPTY_NODE,
			.Right = &EMPTY_NODE,
			.NodeType = NODE_TYPE_NUM,
			.Num = StrToNum(CurrentToken)
		};
	}
	else if(IsSecondaryOp(CurrentToken)){
		(*TokenIndex)++;

		usize RightNodeIndex = (*NodeIndex)++;
		Factor(Tokens, TokenIndex, Nodes, NodeIndex, RightNodeIndex);

		Nodes[CurrentNodeIndex].Right = &Nodes[RightNodeIndex];
		Nodes[CurrentNodeIndex].NodeType = NODE_TYPE_UNI_OP;
		Nodes[CurrentNodeIndex].Op = CurrentToken;
	}
	else
		Nodes[CurrentNodeIndex] = EMPTY_NODE;
}

void Term(const string *Tokens, usize *TokenIndex, node *Nodes, usize *NodeIndex, usize CurrentNodeIndex){
	usize LeftNodeIndex = (*NodeIndex)++;
	Factor(Tokens, TokenIndex, Nodes, NodeIndex, LeftNodeIndex);

	while(IsPrimaryOp(Tokens[*TokenIndex])){
		string CurrentToken = Tokens[*TokenIndex];
		(*TokenIndex)++;
		usize RightNodeIndex = (*NodeIndex)++;
		Factor(Tokens, TokenIndex, Nodes, NodeIndex, RightNodeIndex);

		usize OldLeftIndex = (*NodeIndex)++;
		Nodes[OldLeftIndex] = Nodes[LeftNodeIndex];

		Nodes[LeftNodeIndex] = (node){
			.Left = &Nodes[OldLeftIndex],
			.Right = &Nodes[RightNodeIndex],
			.NodeType = NODE_TYPE_BIN_OP
		};
		Nodes[LeftNodeIndex].Op = CurrentToken;
	}

	Nodes[CurrentNodeIndex] = Nodes[LeftNodeIndex];
}

void Expression(const string *Tokens, usize *TokenIndex, node *Nodes, usize *NodeIndex, usize CurrentNodeIndex){
	usize LeftNodeIndex = (*NodeIndex)++;
	Term(Tokens, TokenIndex, Nodes, NodeIndex, LeftNodeIndex);

	while(IsSecondaryOp(Tokens[*TokenIndex])){
		string CurrentToken = Tokens[*TokenIndex];
		(*TokenIndex)++;
		usize RightNodeIndex = (*NodeIndex)++;
		Term(Tokens, TokenIndex, Nodes, NodeIndex, RightNodeIndex);

		usize OldLeftIndex = (*NodeIndex)++;
		Nodes[OldLeftIndex] = Nodes[LeftNodeIndex];

		Nodes[LeftNodeIndex] = (node){
			.Left = &Nodes[OldLeftIndex],
			.Right = &Nodes[RightNodeIndex],
			.NodeType = NODE_TYPE_BIN_OP
		};
		Nodes[LeftNodeIndex].Op = CurrentToken;
	}

	Nodes[CurrentNodeIndex] = Nodes[LeftNodeIndex];
}

f64 ParseAST(const node *AST){
	switch(AST->NodeType){
	case NODE_TYPE_EMPTY: return 0;

	case NODE_TYPE_NUM: return AST->Num;

	case NODE_TYPE_UNI_OP:{
		if(AST->Op.Data[0] == '+') return ParseAST(AST->Right); 
		else if(AST->Op.Data[0] == '-') return -ParseAST(AST->Right);
	} break; 

	case NODE_TYPE_BIN_OP:{
		f64 LeftNum = ParseAST(AST->Left);
		f64 RightNum = ParseAST(AST->Right);
		
		switch(AST->Op.Data[0]){
		case '+': return LeftNum + RightNum;
		case '-': return LeftNum - RightNum;
		case '*': return LeftNum * RightNum;
		case '/': return LeftNum / RightNum;
		}
	} break;

	default: printf("this should not be here!");
	}
	
	return 0;
}

void ParseCells(cells Cells){
	string Tokens[32];
	usize TokensCount = 0;
	node Nodes[32];
	usize NodesCount = 0;

	for(usize x=0;x<Cells.DataCapacity;x++){ Tokens[x] = EMPTY_STRING; Nodes[x] = EMPTY_NODE; } //look up to this

	for(usize Row=0; Row<Cells.RowsCount; Row++)
		for(usize Column=0; Column<Cells.ColumnsCount; Column++){
			string Cell = CellGet(Cells, Row, Column);
			if(Cell.Data[0] == '='){
				string Field = (string){ .Data = &Cell.Data[1], .Length = Cell.Length - 1 };
				Split(Field, STR("+-*/()"), Tokens, &TokensCount);
				for(usize TokenIndex=0; TokenIndex<TokensCount; TokenIndex++)
					Trim(&Tokens[TokenIndex]);

				usize CurrentTokenIndex = 0;
				Expression(Tokens, &CurrentTokenIndex, Nodes, &NodesCount, 0);
				f64 ParsedCell = ParseAST(&Nodes[0]);
				
				CellErase(Cells, Row, Column);
				char Buff[10];
				usize BuffLength = snprintf(Buff, 10, "%.2f", ParsedCell);
				CellSet(Cells, (string){Buff, BuffLength}, Row, Column);
				for(usize x=0;x<Cells.DataCapacity;x++){ Tokens[x] = EMPTY_STRING; Nodes[x] = EMPTY_NODE; } //look up to this
			}
		}
}

void PrintCells(cells Cells){
	for(usize Row=0; Row<Cells.RowsCount; Row++){
		for(usize Column=0; Column<Cells.ColumnsCount; Column++){
			string Cell = CellGet(Cells, Row, Column);
			PrintString(Cell);
			putchar('|');
		}
		putchar('\n');
	}
}

f32 Absf(f32 x){ return x>0? x: -x; }
f32 Floor(f32 x){ return (f32)(s32)x; }

int main(){ 

	const string Input = STR(
		"1      |2\n"
		"3      |4   |   5\n"
		"=12-13*9    |=19*10\n"
	);

	cells Cells = (cells){
		.RowsCount = 0,
		.ColumnsCount = 0,
		.DataCapacity = 32,
		.RowsCapacity = 5,
		.ColumnsCapacity = 5
	};
	
	Cells.CellSize = Cells.DataCapacity * sizeof(*Cells.Data) + sizeof(*Cells.Length);

	char Buff[Cells.ColumnsCapacity * Cells.RowsCapacity * Cells.CellSize];
	memset(Buff, 0, sizeof(Buff));

	Cells.Data = Buff;
	Cells.Length = (usize *)(Cells.Data + Cells.DataCapacity);

	CountRowsColumns(&Cells, Input, '|');
	FillCells(Cells, Input, '|');
	PrintCells(Cells);
	ParseCells(Cells);
	PrintCells(Cells);

	InitWindow(800, 600, "Minicel");
	SetTargetFPS(60);
	SetWindowState(FLAG_WINDOW_RESIZABLE);

	f32 Factor = 1.0f;
	Vector2 Offset = {0, 0};
	u32 SelectedCellColumn = 0;
	u32 SelectedCellRow = 0;
	while(!WindowShouldClose()){
		BeginDrawing();
		ClearBackground(GetColor(0x181818AA));

		Factor += GetMouseWheelMove() / 16;
		Factor = Factor <= 0.125f? 0.125f: Factor;
		u32 CellsHorizantalCount = 10 * Factor;
		u32 CellsVerticalCount = 40 * Factor;
		u32 ScreenWidth = GetScreenWidth();
		u32 ScreenHeight = GetScreenHeight();
		u32 CellWidth = ScreenWidth/CellsHorizantalCount;
		u32 CellHeight = ScreenHeight/CellsVerticalCount;
		u32 CellsVertical = CellsVerticalCount + Absf(Offset.y)/CellHeight;
		u32 CellsHorizontal = CellsHorizantalCount + Absf(Offset.x)/CellWidth;
		u32 TextPaddingHorizontal = CellWidth / 16;
		u32 TextPaddingVertical = CellHeight / 4;
		f32 LineThickness = 1/Factor >= 1.0? 1/Factor: 1.0f;
		u32 FontSize = 10 * LineThickness ; 

		if(IsMouseButtonDown(MOUSE_BUTTON_RIGHT)){
			Vector2 MouseDelta = GetMouseDelta();
			Offset.x += MouseDelta.x;
			Offset.y += MouseDelta.y;
			Offset.x = Offset.x >= 0? 0: Offset.x;
			Offset.y = Offset.y >= 0? 0: Offset.y;
		}
		//draw table
		for(u32 Row=0; Row<=CellsVertical; Row++)
			DrawLineEx((Vector2){0          , Row*CellHeight + Offset.y},
				   (Vector2){ScreenWidth, Row*CellHeight + Offset.y},
				   LineThickness, GRAY);

		for(u32 Column=0; Column<=CellsHorizontal; Column++)
			DrawLineEx((Vector2){Column*CellWidth + Offset.x, 0},
				   (Vector2){Column*CellWidth + Offset.x, ScreenHeight},
				   LineThickness, GRAY);

		//draw selected cell
		if(IsMouseButtonDown(MOUSE_BUTTON_LEFT)){
			Vector2 MouseLeftPress = GetMousePosition();
			MouseLeftPress.x -= Offset.x;
			MouseLeftPress.y -= Offset.y;
			SelectedCellColumn = Floor(MouseLeftPress.x/CellWidth);
			SelectedCellRow = Floor(MouseLeftPress.y/CellHeight);
		}
		DrawRectangleLinesEx((Rectangle){ SelectedCellColumn*CellWidth + Offset.x, SelectedCellRow*CellHeight + Offset.y, CellWidth, CellHeight}, 2*LineThickness, WHITE);

		//modify cells
		s32 CharPressed = GetCharPressed();
		if(CharPressed){
			string PressedCell = CellGet(Cells, SelectedCellRow, SelectedCellColumn);
			PressedCell.Data[PressedCell.Length] = CharPressed;
			PressedCell.Length += 1;
			CellSet(Cells, PressedCell, SelectedCellRow, SelectedCellColumn);
		}
		if(IsKeyPressed(KEY_BACKSPACE)){
			string PressedCell = CellGet(Cells, SelectedCellRow, SelectedCellColumn);
			PressedCell.Length -= PressedCell.Length > 0? 1: 0;
			CellSet(Cells, PressedCell, SelectedCellRow, SelectedCellColumn);
		}
		if(IsKeyPressed(KEY_ENTER)){
			ParseCells(Cells);
		}
		if(IsKeyPressed(KEY_RIGHT)) SelectedCellColumn += 1; 
		if(IsKeyPressed(KEY_LEFT))  SelectedCellColumn -= SelectedCellColumn > 0? 1: 0;
		if(IsKeyPressed(KEY_DOWN))  SelectedCellRow += 1;
		if(IsKeyPressed(KEY_UP))    SelectedCellRow -= SelectedCellRow > 0? 1: 0;

		//draw cells
		for(u32 Row=0; Row<=CellsVertical && Row<Cells.RowsCount; Row++)
			for(u32 Column=0; Column<=CellsHorizontal && Column<Cells.ColumnsCount; Column++){
				Vector2 Position = { Column*CellWidth + Offset.x + TextPaddingHorizontal, Row*CellHeight + Offset.y + TextPaddingVertical};
				string Text = CellGet(Cells, Row, Column);
				//fix this
				for(u32 CharIndex=0; CharIndex<Text.Length; CharIndex++)
					DrawTextCodepoint(GetFontDefault(), Text.Data[CharIndex], (Vector2){ Position.x + FontSize*CharIndex/1.5f, Position.y }, FontSize, WHITE); 
			}

		#ifdef DEBUG 
		{
			char DebugBuff[256];
			snprintf(DebugBuff, 256, "Factor = %f\n 1/F = %f\nLineThickness = %f\nOffset = %f , %f\nSelectedCell = %d, %d",
				 Factor, 1/Factor, LineThickness, Offset.x, Offset.y, SelectedCellRow, SelectedCellColumn);
			DrawText(DebugBuff, 0, 100, 18, MAGENTA);
		}
		#endif

		EndDrawing();
	}

	CloseWindow();

	return 0;
}
