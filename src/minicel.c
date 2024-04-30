#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include <stdlib.h>
#include <string.h>

typedef size_t usize;
typedef ssize_t ssize;
typedef float f32;

typedef struct{
	char *Data;
	usize Length;
} string;

#define STR(str) (string){(str), sizeof((str))-1}

static const string EMPTY_STRING = {"", 0};

typedef enum { NODE_TYPE_EMPTY, NODE_TYPE_NUMBER, NODE_TYPE_EXPR, NODE_TYPE_UNI_OP, NODE_TYPE_BIN_OP, NODE_TYPE_COUNT } node_type;
typedef struct node{
	struct node *Left, *Right;
	node_type NodeType;
	union{
		string Op;
		ssize NumInt;
		f32 NumFloat;
	};
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

#define CellIndexAt(Cells, Row, Column) ((Row) * (Cells).ColumnsCount + (Column) * (Cells.CellSize))

void PrintString(const string Input){
	for(usize CharIndex=0; CharIndex<Input.Length; CharIndex++)
		putchar(Input.Data[CharIndex]);
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

string TrimLeftRight(const string Input){
	string OutString = EMPTY_STRING;
	char *CurrentCharacter = Input.Data;
	while(*CurrentCharacter++ == ' ' && CurrentCharacter < (Input.Data + Input.Length));

	OutString.Data = CurrentCharacter - 1;

	CurrentCharacter = Input.Data + Input.Length - 1;
	while(*CurrentCharacter == ' ' && CurrentCharacter >= OutString.Data)
		CurrentCharacter--;
	
	OutString.Length = CurrentCharacter - OutString.Data + 1;
	return OutString;
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

	Cells->RowsCount = NewLineCount + 1;
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

ssize StrToInt(const string Expr){
	ssize Num = 0;
	for(usize CharIndex=0, Exponent = 1; CharIndex<Expr.Length; CharIndex++, Exponent *= 10)
		Num += (Expr.Data[Expr.Length - CharIndex - 1] - '0') * Exponent;
	return Num;
}

bool IsDigit(const string Expr){
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
	if(IsDigit(CurrentToken)){
		(*TokenIndex)++;
		//TODO: support float
		Nodes[CurrentNodeIndex] = (node){
			.Left = &EMPTY_NODE,
			.Right = &EMPTY_NODE,
			.NodeType = NODE_TYPE_NUMBER,
			.NumInt = StrToInt(CurrentToken)
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

ssize ParseAST(const node *AST){

	switch(AST->NodeType){
	case NODE_TYPE_EMPTY: return 0;

	case NODE_TYPE_NUMBER: return AST->NumInt;

	case NODE_TYPE_UNI_OP:{
		if(AST->Op.Data[0] == '+') return ParseAST(AST->Right); 
		else if(AST->Op.Data[0] == '-') return -ParseAST(AST->Right);
	} break; 

	case NODE_TYPE_BIN_OP:{
		ssize LeftNum = ParseAST(AST->Left);
		ssize RightNum = ParseAST(AST->Right);

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
	//for(usize x=0;x<Cells.DataCapacity;x++){ Tokens[x] = EMPTY_STRING; Nodes[x] = EMPTY_NODE; } //no need
	for(usize Row=0; Row<Cells.RowsCount; Row++)
		for(usize Column=0; Column<Cells.ColumnsCount; Column++){
			string Cell = CellGet(Cells, Row, Column);
			if(Cell.Data[0] == '='){
				string Field = (string){ .Data = &Cell.Data[1], .Length = Cell.Length - 1 };
				Split(Field, STR("+-*/()"), Tokens, &TokensCount);

				usize CurrentTokenIndex = 0;
				Expression(Tokens, &CurrentTokenIndex, Nodes, &NodesCount, 0);
				ssize ParsedCell = ParseAST(&Nodes[0]);
				
				CellErase(Cells, Row, Column);
				char Buff[10];
				usize BuffLength = snprintf(Buff, 10, "%ld", ParsedCell);
				CellSet(Cells, (string){Buff, BuffLength}, Row, Column);
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
	
	return 0;
}
