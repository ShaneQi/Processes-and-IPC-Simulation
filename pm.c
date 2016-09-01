#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

int main(int argc, char *argv[]) {
	//  Create two pipes.
	int fd1[2];
	int fd2[2];
	pipe(fd1);
	pipe(fd2);

	//  Fork to generating child process who will act as Memory.
  int pid = fork();

	//  Random seeds set.
	srand(time(NULL));

	//  If fork failed.
	if (pid == -1) {
		printf("Fork failed.");
	}

	//  Child: Memory.
	else if (pid == 0) {
		//  Memory storage.
		int mem[2000];

		//  Load file.
		FILE* fr;
		fr = fopen(argv[1], "r");
		char line[1024];
		int address = 0;
		while (fgets(line, 1024, fr) != NULL) {
			int index = 0;
			//  If this line of program begins with Period.
			if (line[0] == '.') {
				index++;
				char newAddress[10];
				while (line[index] >= '0' && line[index] <= '9') {
          newAddress[index-1] = line[index];
          index++;
        }
				address = atoi(newAddress);
			}
			//  If this line of program begins with number.
			else if (line[0] <= '9' && line[0] >= '0'){
				mem[address] = atoi(line);
				address++;
			}
		}
		fclose(fr);

		//  Responds to processor's fetch request.
		char operationFlag;
		int tempAddress;
		while (operationFlag != 'E') {
			read(fd1[0], &operationFlag, sizeof(operationFlag));
			switch (operationFlag) {
				//  Read:
				case 'R':
		      read(fd1[0], &tempAddress, sizeof(tempAddress));
					// printf("DEBUG: Memory: Reading from: %d. \n", tempAddress);
					write(fd2[1], &mem[tempAddress], sizeof(mem[tempAddress]));
					// printf("DEBUG: Memory: Sending: %d. \n", mem[tempAddress]);
					break;
				//  Write:
				case 'W':
					//  Where to store.
					read(fd1[0], &tempAddress, sizeof(tempAddress));
					//  What to store.
					read(fd1[0], &mem[tempAddress], sizeof(mem[tempAddress]));
					// printf("DEBUG: Memory: Stored %d to mem[%d].\n", mem[tempAddress], tempAddress);
					break;
				//  End:
				case 'E':
					break;
			}
		}
	}

	//  Parent: Processor.
	else {
		//  Declare registers.
		int PC = 0, SP = 1000, IR = 0, AC = 0, X = 0, Y = 0;

		//  Timer initialize.
		int timer = atoi(argv[2]);

		//  Model flag:
		//  0: user;
		//  1: kernel.
		int model = 0;

		char operationFlag;
		int tempValue;
		//  Execute program until the end.
		while (IR != 50) {
			//  Timer interrupt.
			if (model == 0 && timer <= 0) {
				//  Switch model flag.
				model = 1;
				//	Save SP to system stack.
				tempValue = SP;
				SP = 2000;
				SP--;
				operationFlag = 'W';
				write(fd1[1], &operationFlag, sizeof(operationFlag));
				write(fd1[1], &SP, sizeof(SP));
				write(fd1[1], &tempValue, sizeof(tempValue));
				//	Save PC to system stack.
				tempValue = PC;
				SP--;
				operationFlag = 'W';
				write(fd1[1], &operationFlag, sizeof(operationFlag));
				write(fd1[1], &SP, sizeof(SP));
				write(fd1[1], &tempValue, sizeof(tempValue));
				//  Execute from 1000.
				PC = 1000;
				timer = atoi(argv[2]);
			}
			else {
				//  Timer decrement.
				if (model == 0) {
					timer--;
				}

				//  Fetch a instruction.
				operationFlag = 'R';
				write(fd1[1], &operationFlag, sizeof(operationFlag));
				write(fd1[1], &PC, sizeof(PC));
				read(fd2[0], &IR, sizeof(IR));
				PC++;

				//  Run the instruction.
				switch (IR) {

				//  Load the value into the AC.
				case 1:
					//  Fetch and assign to AC.
					operationFlag = 'R';
					write(fd1[1], &operationFlag, sizeof(operationFlag));
					write(fd1[1], &PC, sizeof(PC));
					read(fd2[0], &AC, sizeof(AC));
					PC++;
					break;

				//  Load the value at the address into the AC.
				case 2:
					//  Fetch the address.
					operationFlag = 'R';
					write(fd1[1], &operationFlag, sizeof(operationFlag));
	        write(fd1[1], &PC, sizeof(PC));
	        read(fd2[0], &tempValue, sizeof(tempValue));
					if (model == 0 && tempValue >= 1000) {
						printf("ERROR: Access denied!\n");
						IR = 50;
						break;
					}
					//  Fetch again from the address and assign to AC.
					operationFlag = 'R';
					write(fd1[1], &operationFlag, sizeof(operationFlag));
	        write(fd1[1], &tempValue, sizeof(tempValue));
	        read(fd2[0], &AC, sizeof(AC));
					PC++;
	        break;

				//  Load the value from the address found in the given address into the AC.
				case 3:
					//  Fetch the first address.
					operationFlag = 'R';
					write(fd1[1], &operationFlag, sizeof(operationFlag));
					write(fd1[1], &PC, sizeof(PC));
	        read(fd2[0], &tempValue, sizeof(tempValue));
					if (model == 0 && tempValue >= 1000) {
						printf("ERROR: Access denied!\n");
						IR = 50;
						break;
					}
					//  Fetch the the address stored in the first address.
					operationFlag = 'R';
					write(fd1[1], &operationFlag, sizeof(operationFlag));
	        write(fd1[1], &tempValue, sizeof(tempValue));
	        read(fd2[0], &tempValue, sizeof(tempValue));
					if (model == 0 && tempValue >= 1000) {
						printf("ERROR: Access denied!\n");
						IR = 50;
						break;
					}
					//  Go to the second address and assign to AC.
					operationFlag = 'R';
					write(fd1[1], &operationFlag, sizeof(operationFlag));
	        write(fd1[1], &tempValue, sizeof(tempValue));
	        read(fd2[0], &AC, sizeof(AC));
					PC++;
					break;

				//  Load the value at (address + X) into the AC.
				case 4:
					//  Fetch the address.
					operationFlag = 'R';
					write(fd1[1], &operationFlag, sizeof(operationFlag));
	        write(fd1[1], &PC, sizeof(PC));
	        read(fd2[0], &tempValue, sizeof(tempValue));
					//  Add address with X.
					tempValue = tempValue + X;
					if (model == 0 && tempValue >= 1000) {
						printf("ERROR: Access denied!\n");
						IR = 50;
						break;
					}
					//  Fetch the value into AC.
					operationFlag = 'R';
					write(fd1[1], &operationFlag, sizeof(operationFlag));
					write(fd1[1], &tempValue, sizeof(tempValue));
	        read(fd2[0], &AC, sizeof(AC));
					PC++;
					break;

				//  Load the value at (address + Y) into the AC.
				case 5:
					//  Fetch the address.
					operationFlag = 'R';
					write(fd1[1], &operationFlag, sizeof(operationFlag));
					write(fd1[1], &PC, sizeof(PC));
					read(fd2[0], &tempValue, sizeof(tempValue));
					//  Add address with Y.
					tempValue = tempValue + Y;
					if (model == 0 && tempValue >= 1000) {
						printf("ERROR: Access denied!\n");
						IR = 50;
						break;
					}
					//  Fetch the value into AC.
					operationFlag = 'R';
					write(fd1[1], &operationFlag, sizeof(operationFlag));
					write(fd1[1], &tempValue, sizeof(tempValue));
					read(fd2[0], &AC, sizeof(AC));
					PC++;
					break;

				//  Load from (Sp+X) into the AC.
				case 6:
					//  Calculate the address.
					tempValue = SP + X;
					if (model == 0 && tempValue >= 1000) {
						printf("ERROR: Access denied!\n");
						IR = 50;
						break;
					}
					//  Fetch the value into AC.
					operationFlag = 'R';
					write(fd1[1], &operationFlag, sizeof(operationFlag));
					write(fd1[1], &tempValue, sizeof(tempValue));
					read(fd2[0], &AC, sizeof(AC));
					break;

				//  Store the value in the AC into the address.
				case 7:
					//  Fetch the address where the AC should be stored.
					operationFlag = 'R';
					write(fd1[1], &operationFlag, sizeof(operationFlag));
					write(fd1[1], &PC, sizeof(PC));
					read(fd2[0], &tempValue, sizeof(tempValue));
					if (model == 0 && tempValue >= 1000) {
						printf("ERROR: Access denied!\n");
						IR = 50;
						break;
					}
					//  Enter write model, Send address, then send value (AC).
					operationFlag = 'W';
					write(fd1[1], &operationFlag, sizeof(operationFlag));
					write(fd1[1], &tempValue, sizeof(tempValue));
					write(fd1[1], &AC, sizeof(AC));
					PC++;
					break;

				//  Gets a random int from 1 to 100 into the AC.
				case 8:
					AC = rand()%100 + 1;
					break;

				//  If port=1, writes AC as an int to the screen
				//  If port=2, writes AC as a char to the screen
				case 9:
					operationFlag = 'R';
					write(fd1[1], &operationFlag, sizeof(operationFlag));
					write(fd1[1], &PC, sizeof(PC));
					read(fd2[0], &tempValue, sizeof(tempValue));
					if (tempValue == 1) {
						printf("%d", AC);
					}
					else if (tempValue == 2) {
						printf("%c", AC);
					}
					PC++;
					break;

				//  Add the value in X to the AC.
				case 10:
					AC = AC + X;
					break;

	      //  Add the value in Y to the AC.
	      case 11:
	        AC = AC + Y;
	        break;

				//  Subtract the value in X from the AC.
				case 12:
					AC = AC - X;
					break;

				//  Subtract the value in Y from the AC.
				case 13:
					AC = AC - Y;
					break;

				//  Copy the value in the AC to X.
				case 14:
					X = AC;
					break;

				//  Copy the value in X to the AC.
				case 15:
					AC = X;
					break;

				//  Copy the value in the AC to Y.
				case 16:
					Y = AC;
					break;

				//  Copy the value in Y to the AC.
				case 17:
					AC = Y;
					break;

				//  Copy the value in the AC to SP.
				case 18:
					SP = AC;
					break;

				//  Copy the value in SP to the AC.
				case 19:
					AC = SP;
					break;

				//  Jump to the address.
				case 20:
					operationFlag = 'R';
					write(fd1[1], &operationFlag, sizeof(operationFlag));
					//  Fetch the address want to jump to.
					write(fd1[1], &PC, sizeof(PC));
					read(fd2[0], &tempValue	, sizeof(tempValue));
					if (model == 0 && tempValue >= 1000) {
						printf("ERROR: Access denied!\n");
						IR = 50;
						break;
					}
					//  Jump!
					PC = tempValue;
					break;

				//  Jump to the address only if the value in the AC is zero.
				case 21:
					operationFlag = 'R';
					write(fd1[1], &operationFlag, sizeof(operationFlag));
					//  Fetch the address want to jump to.
					write(fd1[1], &PC, sizeof(PC));
					read(fd2[0], &tempValue, sizeof(tempValue));
					if (AC == 0) {
						if (model == 0 && tempValue >= 1000) {
							printf("ERROR: Access denied!\n");
							IR = 50;
							break;
						}
						//  If AC == 0, JUMP!
						PC = tempValue;
					}
					else {
						//  If AC != 0, don't jump.
						PC++;
					}
					break;

				//  Jump to the address only if the value in the AC is not zero.
				case 22:
					operationFlag = 'R';
					write(fd1[1], &operationFlag, sizeof(operationFlag));
					//  Fetch the address want to jump to.
					write(fd1[1], &PC, sizeof(PC));
					read(fd2[0], &tempValue, sizeof(tempValue));
					if (AC != 0) {
						if (model == 0 && tempValue >= 1000) {
							printf("ERROR: Access denied!\n");
							IR = 50;
							break;
						}
						//  If AC == 0, JUMP!
						PC = tempValue;
					}
					else {
						//  If AC != 0, don't jump.
						PC++;
					}
					break;

				//  Push return address onto stack, jump to the address.
				case 23:
					//  Push return address onto stack.
					SP--;
					operationFlag = 'W';
					write(fd1[1], &operationFlag, sizeof(operationFlag));
					write(fd1[1], &SP, sizeof(SP));
					//  Return address should be PC + 1.
					tempValue = PC + 1;
					write(fd1[1], &tempValue, sizeof(tempValue));

					//  Jump to the address.
					operationFlag = 'R';
					write(fd1[1], &operationFlag, sizeof(operationFlag));
					write(fd1[1], &PC, sizeof(PC));
					read(fd2[0], &tempValue, sizeof(tempValue));
					if (model == 0 && tempValue >= 1000) {
						printf("ERROR: Access denied!\n");
						IR = 50;
						break;
					}
					PC = tempValue;
					break;

				//	Pop return address from the stack, jump to the address.
				case 24:
					model = 0;
					operationFlag = 'R';
					write(fd1[1], &operationFlag, sizeof(operationFlag));
					write(fd1[1], &SP, sizeof(SP));
					read(fd2[0], &PC, sizeof(PC));
					SP++;
					break;

				//  Increment the value in X.
				case 25:
					X = X + 1;
					break;

				//  Decrement the value in X.
				case 26:
					X = X - 1;
					break;

				//  Push AC onto stack
				case 27:
					operationFlag = 'W';
					write(fd1[1], &operationFlag, sizeof(operationFlag));
					SP--;
					write(fd1[1], &SP, sizeof(SP));
					write(fd1[1], &AC, sizeof(AC));
					break;

				//  Pop from stack into AC.
				case 28:
					operationFlag = 'R';
					write(fd1[1], &operationFlag, sizeof(operationFlag));
					write(fd1[1], &SP, sizeof(SP));
					read(fd2[0], &AC, sizeof(AC));
					SP++;
					break;

				//  Perform system call.
				case 29:
					if (model == 1) {
						break;
					}
					model = 1;
					//	Save SP to system stack.
					tempValue = SP;
					SP = 2000;
					operationFlag = 'W';
					write(fd1[1], &operationFlag, sizeof(operationFlag));
					SP--;
					write(fd1[1], &SP, sizeof(SP));
					write(fd1[1], &tempValue, sizeof(tempValue));
					//	Save PC to system stack.
					tempValue = PC;
					operationFlag = 'W';
					write(fd1[1], &operationFlag, sizeof(operationFlag));
					SP--;
					write(fd1[1], &SP, sizeof(SP));
					write(fd1[1], &tempValue, sizeof(tempValue));
					//  Execute from 1500.
					PC = 1500;
					break;

				//  Return from system call.
				case 30:
					//  Switch modl flag.
					model = 0;
					//  Swtch the SP to where the former SP and PC is stored.
					SP = 1998;
					//	Resume user SP and PC.
					operationFlag = 'R';
					write(fd1[1], &operationFlag, sizeof(operationFlag));
					write(fd1[1], &SP, sizeof(SP));
					read(fd2[0], &PC, sizeof(PC));
					SP++;
					operationFlag = 'R';
					write(fd1[1], &operationFlag, sizeof(operationFlag));
					write(fd1[1], &SP, sizeof(SP));
					read(fd2[0], &SP, sizeof(SP));
					break;

				//  End execution.
				case 50:
					operationFlag = 'E';
					write(fd1[1], &operationFlag, sizeof(operationFlag));
					break;
				}
			}
		}
	}
}
