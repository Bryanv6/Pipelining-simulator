#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NUMMEMORY 16 /* Maximum number of data words in memory */
#define NUMREGS 8    /* Number of registers */

/* Opcode values for instructions */
#define R 0
#define LW 35
#define SW 43
#define BNE 4
#define HALT 63

/* Funct values for R-type instructions */
#define ADD 32
#define SUB 34

/* Branch Prediction Buffer Values */
#define STRONGLYTAKEN 3
#define WEAKLYTAKEN 2
#define WEAKLYNOTTAKEN 1
#define STRONGLYNOTTAKEN 0

typedef struct IFIDStruct {
  unsigned int instr;              /* Integer representation of instruction */
  int PCPlus4;                     /* PC + 4 */
  int bpb;
} IFIDType;

typedef struct IDEXStruct {
  unsigned int instr;              /* Integer representation of instruction */
  int PCPlus4;                     /* PC + 4 */
  int readData1;                   /* Contents of rs register */
  int readData2;                   /* Contents of rt register */
  int immed;                       /* Immediate field */
  int rsReg;                       /* Number of rs register */
  int rtReg;                       /* Number of rt register */
  int rdReg;                       /* Number of rd register */
  int branchTarget;                /* Branch target, obtained from immediate field */
  int bpb;
} IDEXType;

typedef struct EXMEMStruct {
  unsigned int instr;              /* Integer representation of instruction */
  int aluResult;                   /* Result of ALU operation */
  int writeDataReg;                /* Contents of the rt register, used for store word */
  int writeReg;                    /* The destination register */
  int bpb;
} EXMEMType;

typedef struct MEMWBStruct {
  unsigned int instr;              /* Integer representation of instruction */
  int writeDataMem;                /* Data read from memory */
  int writeDataALU;                /* Result from ALU operation */
  int writeReg;                    /* The destination register */
  int bpb;
} MEMWBType;

typedef struct stateStruct {
  int PC;                                 /* Program Counter */
  unsigned int instrMem[NUMMEMORY];       /* Instruction memory */
  int dataMem[NUMMEMORY];                 /* Data memory */
  int regFile[NUMREGS];                   /* Register file */
  IFIDType IFID;                          /* Current IFID pipeline register */
  IDEXType IDEX;                          /* Current IDEX pipeline register */
  EXMEMType EXMEM;                        /* Current EXMEM pipeline register */
  MEMWBType MEMWB;                        /* Current MEMWB pipeline register */
  int cycles;                             /* Number of cycles executed so far */
  //unsigned int bpb;
} stateType;

void run();
void printState(stateType*);
void initState(stateType*);
unsigned int instrToInt(char*, char*);
int get_opcode(unsigned int);
void printInstruction(unsigned int);

int main(){
    run();
    return(0);
}

void run(){

  stateType state;           /* Contains the state of the entire pipeline before the cycle executes */
  stateType newState;        /* Contains the state of the entire pipeline after the cycle executes */
  initState(&state);         /* Initialize the state of the pipeline */
  int stalled = 0; //bool check
  int stalls = 0; //num of stalls
  int mispredictions = 0;
  int branches = 0;
  unsigned bpb = WEAKLYTAKEN;
    while (1) {

        printState(&state);


	/* If a halt instruction is entering its WB stage, then all of the legitimate */
	/* instruction have completed. Print the statistics and exit the program. */
        if (get_opcode(state.MEMWB.instr) == HALT) {
            printf("Total number of cycles executed: %d\n", state.cycles);
            printf("Total number of stalls: %d\n", stalls);
            printf("Total number of branches %d\n", branches);
            printf("Total number of mispredicted branches: %d\n", mispredictions);
            /* Remember to print the number of stalls, branches, and mispredictions! */
            exit(0);
        }

        newState = state;     /* Start by making newState a copy of the state before the cycle */
        newState.cycles++;
	/* Modify newState stage-by-stage below to reflect the state of the pipeline after the cycle has executed */

        /* --------------------- IF stage --------------------- */

        newState.PC = state.PC + 4;
        newState.IFID.PCPlus4 = state.PC + 4;

        newState.IFID.instr = state.instrMem[(newState.PC/4) - 1];

        /* --------------------- ID stage --------------------- */
        if(newState.cycles > 1){

          newState.IDEX.instr = state.IFID.instr;
          newState.IDEX.PCPlus4 = state.IFID.PCPlus4;
          newState.IDEX.immed = get_immed(state.IFID.instr);
          newState.IDEX.branchTarget = get_immed(state.IFID.instr); //branch target is in the immed field
          newState.IDEX.rsReg = get_rs(state.IFID.instr);
          newState.IDEX.rtReg = get_rt(state.IFID.instr);
          newState.IDEX.rdReg = get_rd(state.IFID.instr);
          newState.IDEX.bpb = state.IFID.bpb;

          if(get_opcode(state.IFID.instr) == LW){
            newState.IDEX.readData1 = get_rs(newState.IDEX.rsReg);  // get content of rs reg
            newState.IDEX.readData2 = get_rt(newState.IDEX.rtReg );  // get content of rt reg
          }
          else if(get_opcode(state.IFID.instr) == SW){

            if((get_rt(state.IFID.instr) == get_rd(state.MEMWB.instr)) && get_opcode(state.MEMWB.instr) == R){
              newState.IDEX.readData2 = state.MEMWB.writeDataALU;
              newState.IDEX.readData1 = state.regFile[get_rs(state.IFID.instr)];  // get content of rt reg
            }
            else{
              newState.IDEX.readData1 = state.regFile[get_rs(state.IFID.instr)];
              newState.IDEX.readData2 = state.regFile[get_rt(state.IFID.instr)];
            }
          }
          else if(newState.IDEX.instr == 0){
            newState.IDEX.PCPlus4 = 0;
            newState.IDEX.immed = 0;
            newState.IDEX.branchTarget = 0;
            newState.IDEX.rsReg = 0;
            newState.IDEX.rtReg = 0;
            newState.IDEX.rdReg = 0;
            newState.IDEX.readData1 = 0;
            newState.IDEX.readData2 = 0;
          }
          else if(get_opcode(state.IFID.instr) == R){
            newState.IDEX.readData1 = state.regFile[newState.IDEX.rsReg];
            newState.IDEX.readData2 = state.regFile[newState.IDEX.rtReg];

            if(get_rt(state.IFID.instr) == (get_rs(state.IFID.instr))){
              newState.IDEX.readData1 = state.MEMWB.writeDataMem;
              newState.IDEX.readData2 = state.MEMWB.writeDataMem;
            }
              else if((get_rs(state.IFID.instr) == get_rt(state.MEMWB.instr)) && get_opcode(state.MEMWB.instr) == LW){
                newState.IDEX.readData1 = state.MEMWB.writeDataMem;
                newState.IDEX.readData2 = state.regFile[newState.IDEX.rtReg];  // get content of rt reg

              }
              else if((get_rt(state.IFID.instr) == get_rt(state.MEMWB.instr)) && get_opcode(state.MEMWB.instr) == LW){
                newState.IDEX.readData2 = state.MEMWB.writeDataMem;
                newState.IDEX.readData1 = state.regFile[newState.IDEX.rsReg];

              }
              //if RS & RT the same REG

              if(get_rt(state.IFID.instr) == (get_rs(state.IFID.instr))){
                if((get_rt(state.IFID.instr) == get_rd(newState.MEMWB.instr)) && get_opcode(newState.MEMWB.instr) == R){
                  newState.IDEX.readData2 = newState.MEMWB.writeDataALU;
                  newState.IDEX.readData1 = newState.MEMWB.writeDataALU;  // get content of rt reg

                }
              }
              else if((newState.IDEX.rtReg == get_rd(newState.MEMWB.instr)) && get_opcode(newState.MEMWB.instr) == R){
                newState.IDEX.readData2 = newState.MEMWB.writeDataALU;
                newState.IDEX.readData1 = state.regFile[newState.IDEX.rsReg];  // get content of rt reg

              }
              else if((newState.IDEX.rsReg == get_rd(newState.MEMWB.instr)) && get_opcode(newState.MEMWB.instr) == R){
                newState.IDEX.readData1 = newState.MEMWB.writeDataALU;
                newState.IDEX.readData2 = state.regFile[newState.IDEX.rtReg];  // get content of rt reg

              }

          }
          else if(get_opcode(state.IFID.instr) == BNE){
            if((get_rs(state.IFID.instr) == get_rt(state.MEMWB.instr)) && get_opcode(state.MEMWB.instr) == LW){
              newState.IDEX.readData1 = state.MEMWB.writeDataMem;
              newState.IDEX.readData2 = state.regFile[newState.IDEX.rtReg];  // get content of rt reg

            }
            if((get_rt(state.IFID.instr) == get_rt(state.MEMWB.instr)) && get_opcode(state.MEMWB.instr) == LW){
              newState.IDEX.readData2 = state.MEMWB.writeDataMem;
              newState.IDEX.readData1 = state.regFile[newState.IDEX.rsReg];

            }
            else{

              newState.IDEX.readData1 = state.regFile[get_rs(state.IFID.instr)];
              newState.IDEX.readData2 = state.regFile[get_rt(state.IFID.instr)];

            }
            if(bpb > 2){

              stalls++;
              stalled = 1;
              branches++;
              newState.PC = newState.IDEX.immed;
              newState.IFID.instr = 0;
              newState.IFID.PCPlus4 = 0;
            }
          }
          else if(get_opcode(state.IFID.instr) == HALT){
            newState.IDEX.readData1 = 0;
            newState.IDEX.readData2 = 0;
          }

          /*---stall next stage if RS or RT from this stage are
          dependent on data from RD from previous instructions*/


          if(((get_rt(state.IFID.instr) == state.IDEX.rtReg) || (get_rs(state.IFID.instr) == state.IDEX.rtReg)) && state.IDEX.instr!= 0 && get_opcode(state.IDEX.instr) != HALT && get_opcode(state.IDEX.instr) != R && get_opcode(state.IDEX.instr) != BNE){
            stalls++;
            stalled = 1;
            newState.IDEX.instr = 0; //NOOP instr
            newState.PC = state.PC;
            newState.IFID.PCPlus4 = state.PC;
            newState.IFID.instr = state.instrMem[(state.PC/4) - 1];;
            newState.IDEX.PCPlus4 = 0;
            newState.IDEX.immed = 0;
            newState.IDEX.branchTarget = 0;
            newState.IDEX.rsReg = 0;
            newState.IDEX.rtReg = 0;
            newState.IDEX.rdReg = 0;
            newState.IDEX.readData1 = 0;
            newState.IDEX.readData2 = 0;
          }

        }


        /* --------------------- EX stage --------------------- */
        if(newState.cycles > 2){
          newState.EXMEM.instr = state.IDEX.instr;
          newState.EXMEM.writeDataReg = state.IDEX.readData2;
          newState.EXMEM.bpb = state.IDEX.bpb;


          if(stalled == 1 && (get_opcode(newState.IDEX.instr) == R || get_opcode(newState.IDEX.instr) == LW)){
            newState.IDEX.readData1 = state.MEMWB.writeReg; // get content of rt reg
            stalled = 0;
          }

          if(newState.EXMEM.instr == 0){ //if instruction is noop
            newState.EXMEM.aluResult = 0;
            newState.EXMEM.writeReg = 0;
            newState.EXMEM.writeDataReg = 0;
          }
          if(get_opcode(state.IDEX.instr) == R){

            if(get_funct(state.IDEX.instr) == ADD){

              if((get_rs(state.IDEX.instr) == get_rt(state.MEMWB.instr)) && get_opcode(state.MEMWB.instr) == LW ){

                state.IDEX.readData1 = state.MEMWB.writeDataMem;

              }
              if((get_rt(state.IDEX.instr) == get_rt(state.MEMWB.instr)) && get_opcode(state.MEMWB.instr) == LW){

                state.IDEX.readData2 = state.MEMWB.writeDataMem;

              }
              if((get_rs(state.IDEX.instr) == get_rd(newState.EXMEM.instr)) && get_opcode(newState.EXMEM.instr) == R){
                state.IDEX.readData1 = newState.EXMEM.aluResult;

              }
              if((get_rt(state.IDEX.instr) == get_rd(newState.EXMEM.instr)) && get_opcode(newState.EXMEM.instr) == R){
                state.IDEX.readData2 = newState.EXMEM.aluResult;

              }

                newState.EXMEM.aluResult = state.IDEX.readData1 + state.IDEX.readData2;
                newState.EXMEM.writeDataReg = state.MEMWB.writeDataMem;
                newState.EXMEM.writeReg = state.IDEX.rdReg;
            }



            else if(get_funct(state.IDEX.instr) == SUB){

              if((get_rs(state.IDEX.instr) == get_rt(state.MEMWB.instr)) && get_opcode(state.MEMWB.instr) == LW){

                state.IDEX.readData1 = state.MEMWB.writeDataMem;

              }
              if((get_rt(state.IDEX.instr) == get_rt(state.MEMWB.instr)) && get_opcode(state.MEMWB.instr) == LW){

                state.IDEX.readData2 = state.MEMWB.writeDataMem;

              }
              if((get_rs(state.IDEX.instr) == get_rd(state.EXMEM.instr)) && get_opcode(state.EXMEM.instr) == R){
                state.IDEX.readData1 = state.EXMEM.aluResult;
              }
              if((get_rt(state.IDEX.instr) == get_rd(state.EXMEM.instr)) && get_opcode(state.EXMEM.instr) == R){
                state.IDEX.readData2 = state.EXMEM.aluResult;

              }

                newState.EXMEM.aluResult = state.IDEX.readData1 - state.IDEX.readData2;
                newState.EXMEM.writeDataReg = state.MEMWB.writeDataMem;
                newState.EXMEM.writeReg = state.IDEX.rdReg;


            }
          }
          else if(get_opcode(state.IDEX.instr) == LW){

            newState.EXMEM.aluResult = state.IDEX.immed + state.IDEX.readData2;
            newState.EXMEM.writeReg = state.IDEX.rtReg;
          }
          else if(get_opcode(state.IDEX.instr) == SW){
            newState.EXMEM.aluResult = state.IDEX.immed + state.IDEX.readData1;
            newState.EXMEM.writeReg = state.IDEX.rtReg;

            if((get_rt(state.IDEX.instr) == get_rd(state.EXMEM.instr)) && get_opcode(state.EXMEM.instr) == R){
              newState.EXMEM.writeDataReg = state.EXMEM.aluResult;
            }
            else if((get_rt(state.IDEX.instr) == get_rt(state.MEMWB.instr)) && get_opcode(state.MEMWB.instr) == LW){
              newState.EXMEM.writeDataReg = state.MEMWB.writeDataMem;
            }


          }
          else if(get_opcode(state.IDEX.instr) == BNE){


            if((get_rs(state.IDEX.instr) == get_rt(state.MEMWB.instr)) && get_opcode(state.MEMWB.instr) == LW){

              state.IDEX.readData1 = state.MEMWB.writeDataMem;
              newState.EXMEM.writeDataReg = state.MEMWB.writeDataMem;
            }
            if((get_rt(state.IDEX.instr) == get_rt(state.MEMWB.instr)) && get_opcode(state.MEMWB.instr) == LW){

              state.IDEX.readData2 = state.MEMWB.writeDataMem;
              newState.EXMEM.writeDataReg = state.MEMWB.writeDataMem;
            }
            if((get_rs(state.IDEX.instr) == get_rd(state.EXMEM.instr)) && get_opcode(state.EXMEM.instr) == R){
              state.IDEX.readData1 = state.EXMEM.aluResult;
            }
            if((get_rt(state.IDEX.instr) == get_rd(state.EXMEM.instr)) && get_opcode(state.EXMEM.instr) == R){
              state.IDEX.readData2 = state.EXMEM.aluResult;
            }


            newState.EXMEM.writeReg = state.IDEX.rtReg;
            newState.EXMEM.writeDataReg = state.IDEX.readData2;
            newState.EXMEM.aluResult = state.IDEX.readData1 - state.IDEX.readData2;

            if((state.IDEX.readData1 - state.IDEX.readData2) != 0){

              if(bpb < 3 & newState.EXMEM.aluResult != 0){
                mispredictions++;
                stalls++;
                stalled = 1;
                bpb++;
                newState.IDEX.instr = 0; //NOOP instr
                newState.PC = state.IDEX.immed;
                newState.IFID.PCPlus4 = 0;
                newState.IFID.instr = 0;
                newState.IDEX.PCPlus4 = 0;
                newState.IDEX.immed = 0;
                newState.IDEX.branchTarget = 0;
                newState.IDEX.rsReg = 0;
                newState.IDEX.rtReg = 0;
                newState.IDEX.rdReg = 0;
                newState.IDEX.readData1 = 0;
                newState.IDEX.readData2 = 0;
              }
              else{
                stalls++;
                  bpb++;

              }

            }
            else if((state.IDEX.readData1 - state.IDEX.readData2) == 0){

              if(bpb > 2){
                bpb--;
                mispredictions++;
              }


              stalled = 0;
              if(branches < 1){
                newState.PC = newState.IDEX.PCPlus4 + 4;
                newState.IFID.PCPlus4 =  0;

              }
              else
                newState.PC = state.IDEX.PCPlus4;
              branches++;
              newState.IFID.PCPlus4 =newState.IDEX.PCPlus4 + 4;
              newState.IFID.instr = 0;


            }

          }
          else if(get_opcode(state.IDEX.instr) == HALT){
            newState.EXMEM.aluResult = 0;
            newState.EXMEM.writeReg = 0;
            newState.EXMEM.writeDataReg = 0;
          }

        }

        /* --------------------- MEM stage --------------------- */
        if(newState.cycles > 3){

          newState.MEMWB.instr = state.EXMEM.instr;
          newState.MEMWB.writeDataALU = state.EXMEM.aluResult;
          newState.MEMWB.writeReg = state.EXMEM.writeReg;

          if(get_opcode(state.EXMEM.instr) == R){
            if(get_funct(state.EXMEM.instr) == ADD){

          }
          if(get_funct(state.EXMEM.instr) == SUB){

          }
        }
          else if(get_opcode(state.EXMEM.instr) == LW){
            newState.MEMWB.writeDataMem = state.dataMem[state.EXMEM.aluResult/4];
          }
          else if(get_opcode(state.EXMEM.instr) == SW){
            newState.dataMem[(state.EXMEM.aluResult/4)] = state.EXMEM.writeDataReg;
          }
          else if(get_opcode(state.EXMEM.instr) == HALT){
            newState.MEMWB.writeDataALU = 0;
            newState.MEMWB.writeReg = 0;
          }
          else if(state.EXMEM.instr == 0){
            newState.MEMWB.writeDataALU = 0;
            newState.MEMWB.writeReg = 0;

          }

        }

        /* --------------------- WB stage --------------------- */
        if(newState.cycles > 4){
          if(get_opcode(state.MEMWB.instr) == LW){
            newState.regFile[state.MEMWB.writeReg] = state.MEMWB.writeDataMem;
          }
          else if(get_opcode(state.MEMWB.instr) == R){
            if(get_funct(state.MEMWB.instr) == ADD ||get_funct(state.MEMWB.instr) == SUB )
              newState.regFile[state.MEMWB.writeReg] = state.MEMWB.writeDataALU;
          }
        }

        state = newState;    /* The newState now becomes the old state before we execute the next cycle */

    }
}


/******************************************************************/
/* The initState function accepts a pointer to the current        */
/* state as an argument, initializing the state to pre-execution  */
/* state. In particular, all registers are zero'd out. All        */
/* instructions in the pipeline are NOOPS. Data and instruction   */
/* memory are initialized with the contents of the assembly       */
/* input file.                                                    */
/*****************************************************************/
void initState(stateType *statePtr)
{
    unsigned int dec_inst;
    int data_index = 0;
    int inst_index = 0;
    char line[130];
    char instr[5];
    char args[130];
    char* arg;

    statePtr->PC = 0;
    statePtr->cycles = 0;

    /* Zero out data, instructions, and registers */
    memset(statePtr->dataMem, 0, 4*NUMMEMORY);
    memset(statePtr->instrMem, 0, 4*NUMMEMORY);
    memset(statePtr->regFile, 0, 4*NUMREGS);

    /* Parse assembly file and initialize data/instruction memory */
    while(fgets(line, 130, stdin)){
        if(sscanf(line, "\t.%s %s", instr, args) == 2){
            arg = strtok(args, ",");
            while(arg != NULL){
                statePtr->dataMem[data_index] = atoi(arg);
                data_index += 1;
                arg = strtok(NULL, ",");
            }
        }
        else if(sscanf(line, "\t%s %s", instr, args) == 2){
            dec_inst = instrToInt(instr, args);
            statePtr->instrMem[inst_index] = dec_inst;
            inst_index += 1;
        }
    }

    /* Zero-out all registers in pipeline to start */
    statePtr->IFID.instr = 0;
    statePtr->IFID.PCPlus4 = 0;
    statePtr->IDEX.instr = 0;
    statePtr->IDEX.PCPlus4 = 0;
    statePtr->IDEX.branchTarget = 0;
    statePtr->IDEX.readData1 = 0;
    statePtr->IDEX.readData2 = 0;
    statePtr->IDEX.immed = 0;
    statePtr->IDEX.rsReg = 0;
    statePtr->IDEX.rtReg = 0;
    statePtr->IDEX.rdReg = 0;

    statePtr->EXMEM.instr = 0;
    statePtr->EXMEM.aluResult = 0;
    statePtr->EXMEM.writeDataReg = 0;
    statePtr->EXMEM.writeReg = 0;

    statePtr->MEMWB.instr = 0;
    statePtr->MEMWB.writeDataMem = 0;
    statePtr->MEMWB.writeDataALU = 0;
    statePtr->MEMWB.writeReg = 0;
 }


 /***************************************************************************************/
 /*              You do not need to modify the functions below.                         */
 /*                They are provided for your convenience.                              */
 /***************************************************************************************/


/*************************************************************/
/* The printState function accepts a pointer to a state as   */
/* an argument and prints the formatted contents of          */
/* pipeline register.                                        */
/* You should not modify this function.                      */
/*************************************************************/
void printState(stateType *statePtr)
{
    int i;
    printf("\n********************\nState at the beginning of cycle %d:\n", statePtr->cycles+1);
    printf("\tPC = %d\n", statePtr->PC);
    printf("\tData Memory:\n");
    for (i=0; i<(NUMMEMORY/2); i++) {
        printf("\t\tdataMem[%d] = %d\t\tdataMem[%d] = %d\n",
            i, statePtr->dataMem[i], i+(NUMMEMORY/2), statePtr->dataMem[i+(NUMMEMORY/2)]);
    }
    printf("\tRegisters:\n");
    for (i=0; i<(NUMREGS/2); i++) {
        printf("\t\tregFile[%d] = %d\t\tregFile[%d] = %d\n",
            i, statePtr->regFile[i], i+(NUMREGS/2), statePtr->regFile[i+(NUMREGS/2)]);
    }
    printf("\tIF/ID:\n");
    printf("\t\tInstruction: ");
    printInstruction(statePtr->IFID.instr);
    printf("\t\tPCPlus4: %d\n", statePtr->IFID.PCPlus4);
    printf("\tID/EX:\n");
    printf("\t\tInstruction: ");
    printInstruction(statePtr->IDEX.instr);
    printf("\t\tPCPlus4: %d\n", statePtr->IDEX.PCPlus4);
    printf("\t\tbranchTarget: %d\n", statePtr->IDEX.branchTarget);
    printf("\t\treadData1: %d\n", statePtr->IDEX.readData1);
    printf("\t\treadData2: %d\n", statePtr->IDEX.readData2);
    printf("\t\timmed: %d\n", statePtr->IDEX.immed);
    printf("\t\trs: %d\n", statePtr->IDEX.rsReg);
    printf("\t\trt: %d\n", statePtr->IDEX.rtReg);
    printf("\t\trd: %d\n", statePtr->IDEX.rdReg);
    printf("\tEX/MEM:\n");
    printf("\t\tInstruction: ");
    printInstruction(statePtr->EXMEM.instr);
    printf("\t\taluResult: %d\n", statePtr->EXMEM.aluResult);
    printf("\t\twriteDataReg: %d\n", statePtr->EXMEM.writeDataReg);
    printf("\t\twriteReg:%d\n", statePtr->EXMEM.writeReg);
    printf("\tMEM/WB:\n");
    printf("\t\tInstruction: ");
    printInstruction(statePtr->MEMWB.instr);
    printf("\t\twriteDataMem: %d\n", statePtr->MEMWB.writeDataMem);
    printf("\t\twriteDataALU: %d\n", statePtr->MEMWB.writeDataALU);
    printf("\t\twriteReg: %d\n", statePtr->MEMWB.writeReg);
}

/*************************************************************/
/*  The instrToInt function converts an instruction from the */
/*  assembly file into an unsigned integer representation.   */
/*  For example, consider the add $0,$1,$2 instruction.      */
/*  In binary, this instruction is:                          */
/*   000000 00001 00010 00000 00000 100000                   */
/*  The unsigned representation in decimal is therefore:     */
/*   2228256                                                 */
/*************************************************************/
unsigned int instrToInt(char* inst, char* args){

    int opcode, rs, rt, rd, shamt, funct, immed;
    unsigned int dec_inst;

    if((strcmp(inst, "add") == 0) || (strcmp(inst, "sub") == 0)){
        opcode = 0;
        if(strcmp(inst, "add") == 0)
            funct = ADD;
        else
            funct = SUB;
        shamt = 0;
        rd = atoi(strtok(args, ",$"));
        rs = atoi(strtok(NULL, ",$"));
        rt = atoi(strtok(NULL, ",$"));
        dec_inst = (opcode << 26) + (rs << 21) + (rt << 16) + (rd << 11) + (shamt << 6) + funct;
    } else if((strcmp(inst, "lw") == 0) || (strcmp(inst, "sw") == 0)){
        if(strcmp(inst, "lw") == 0)
            opcode = LW;
        else
            opcode = SW;
        rt = atoi(strtok(args, ",$"));
        immed = atoi(strtok(NULL, ",("));
        rs = atoi(strtok(NULL, "($)"));
        dec_inst = (opcode << 26) + (rs << 21) + (rt << 16) + immed;
    } else if(strcmp(inst, "bne") == 0){
        opcode = 4;
        rs = atoi(strtok(args, ",$"));
        rt = atoi(strtok(NULL, ",$"));
        immed = atoi(strtok(NULL, ","));
        dec_inst = (opcode << 26) + (rs << 21) + (rt << 16) + immed;
    } else if(strcmp(inst, "halt") == 0){
        opcode = 63;
        dec_inst = (opcode << 26);
    } else if(strcmp(inst, "noop") == 0){
        dec_inst = 0;
    }
    return dec_inst;
}

int get_rs(unsigned int instruction){
    return( (instruction>>21) & 0x1F);
}

int get_rt(unsigned int instruction){
    return( (instruction>>16) & 0x1F);
}

int get_rd(unsigned int instruction){
    return( (instruction>>11) & 0x1F);
}

int get_funct(unsigned int instruction){
    return(instruction & 0x3F);
}

int get_immed(unsigned int instruction){
    return(instruction & 0xFFFF);
}

int get_opcode(unsigned int instruction){
    return(instruction>>26);
}

/*************************************************/
/*  The printInstruction decodes an unsigned     */
/*  integer representation of an instruction     */
/*  into its string representation and prints    */
/*  the result to stdout.                        */
/*************************************************/
void printInstruction(unsigned int instr)
{
    char opcodeString[10];
    if (instr == 0){
      printf("NOOP\n");
    } else if (get_opcode(instr) == R) {
        if(get_funct(instr)!=0){
            if(get_funct(instr) == ADD)
                strcpy(opcodeString, "add");
            else
                strcpy(opcodeString, "sub");
            printf("%s $%d,$%d,$%d\n", opcodeString, get_rd(instr), get_rs(instr), get_rt(instr));
        }
        else{
            printf("NOOP\n");
        }
    } else if (get_opcode(instr) == LW) {
        printf("%s $%d,%d($%d)\n", "lw", get_rt(instr), get_immed(instr), get_rs(instr));
    } else if (get_opcode(instr) == SW) {
        printf("%s $%d,%d($%d)\n", "sw", get_rt(instr), get_immed(instr), get_rs(instr));
    } else if (get_opcode(instr) == BNE) {
        printf("%s $%d,$%d,%d\n", "bne", get_rs(instr), get_rt(instr), get_immed(instr));
    } else if (get_opcode(instr) == HALT) {
        printf("%s\n", "halt");
    }
}
