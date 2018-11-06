#include<stdlib.h>
#include<stdio.h>
#include<pthread.h>
#include<unistd.h>
#include<string.h>
#include<stdbool.h>

#define PNUM 5

///////////////////////////////// FUNCTION DEFINITIONS //////////////////////////////////////////////////

void* pick_or_drop(void *arg);
void pick_up_forks(int phil_num);
void release_forks(int phil_num);
void signal_neighbours(int phil_num);
void signal_not_neighbors(int phil_num);


/////////////////// PHILISOHPERS HAVE A UNIQUE NUMBER AND ALSO HAVE 2 INSTRUCTION E AND T //////////////////

typedef struct
{
	char instruction;
  	int phil_num;
} phil_input;


///////////// 2 DIFFERENT STATES ARE THINKING AND EATING /////////////////////
typedef enum
{
	Thinking, Eating
}state;

////////////////////////// VARIABLES ///////////////////////////////////////////


bool forks[PNUM] = {true, true, true, true, true}; // array of forks: all are avaialable
pthread_mutex_t input_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t fork_mutex = PTHREAD_MUTEX_INITIALIZER;
state* states; // ARRAY OF CURRENT USER STATES
phil_input* current_inputs; // ARRAY OF CURRENT INPUTS FOR EACH PHILOSOPHER
pthread_cond_t *phil_is_ready; //CONDITION VARIABLE (PHIL IS READY TO EAT)
pthread_cond_t *fork_is_ready; //CONDITION VARIABLE (FORK IS AVAILABLE)



///////////////////////////////////////////////////// MAIN //////////////////////////////////////////////


int main()
{
	current_inputs = malloc(sizeof(* current_inputs));

/*INITIALIZE CONDITION VARIABLES AND MAKE ALL PHIL STATES TO THINKING */

	states = malloc(sizeof(*states));
	phil_is_ready = malloc(PNUM * sizeof(pthread_cond_t));
	fork_is_ready = malloc(PNUM * sizeof(pthread_cond_t));

	for (int i = 0; i < PNUM; i++)
	{
		*(states + i) = Thinking;
		pthread_cond_init((phil_is_ready + i), NULL);
		pthread_cond_init((fork_is_ready + i), NULL);
	}

/* GENERATE ARRAY OF THREADS: 1 FOR EACH PHILIOSOPHER */

	pthread_t threads[PNUM];
	for(int i = 0; i < PNUM; i++)
	{
		phil_input* input = malloc(sizeof(phil_input));
		input->instruction = 'T';
		input->phil_num = i;
		pthread_create(&threads[i], NULL, pick_or_drop, (void*)input);
	}

/*wHILE INPUT IS NOT ( ! )*/

	bool valid_run = true;
	while(valid_run == true)
	{
		fflush(stdin); // CLEAR OUT STANDARD INPUT
		char input[2];
		read(0, input, 2);
		//input[3] = '\0';

		char instruction = input[0];
		int phil_num = atoi(input+1);

		/* if change of state is requested, if change is no requested then do nothing*/

		if((states[phil_num] == Thinking && instruction == 'E') || (states[phil_num] == Eating && instruction == 'T'))
		{
			pthread_mutex_lock(&input_mutex);
			current_inputs->instruction = instruction;
			current_inputs->phil_num = phil_num;
			pthread_cond_signal(phil_is_ready + phil_num);
			pthread_mutex_unlock(&input_mutex);
		}
		else if(instruction == 'P')
		{
			/* SPACE ONLT AFTER THE FIRST 4 PHILS*/
			char* printout = malloc(2);
			for(int i =0; i < PNUM-1; i++)
			{
				*printout = *(states + i) + '0';
				*(printout + 1) = ' ';
				write(1, printout, 2);
			}
			/* PHIL 5 HAS A NEW LINE CHARACTER RIGHT AFTER STATE*/
			*printout = *(states + (PNUM-1)) + '0';
			*(printout + 1) = '\n';
			write(1, printout, 2);
			free(printout);
		}
		else if(instruction == '!')
		{
			valid_run = false;/// STOP PROGRAM FROM RUNNING
		}

	}
/* cancel program*/

	for(int i = 0; i < PNUM; i++)
	{
		pthread_cancel(threads[i]);
	}

	return 0;
}



void *pick_or_drop(void *pArg)
{

	phil_input* input = pArg;
	char instruction = input->instruction;
	int phil_num = input->phil_num;

	pthread_mutex_lock(&input_mutex);
	while(true)
	{
		pthread_cond_wait(phil_is_ready + phil_num, &input_mutex);
		int phil_num = current_inputs->phil_num;
		char instruction = current_inputs->instruction;


		pthread_mutex_unlock(&input_mutex);

		if(instruction == 'E')
		{
			pick_up_forks(phil_num);
		}

		else if(instruction == 'T')
		{
			release_forks(phil_num);
		}
	}

	pthread_exit(0);
}

/////////////// RELEASE FORKS BEFORE ENTERING THINKING STATE /////////////////

void release_forks(int phil_num)
{
	int right = phil_num;
	int left = (phil_num + 1) % 5;

	pthread_mutex_lock(&fork_mutex); //LOCK
	//SET BOTH FORKS TO AVAILABLE
	forks[left] = true;
	forks[right] = true;
	states[phil_num] = Thinking;
	pthread_mutex_unlock(&fork_mutex); //UNLOCK
	signal_neighbours(phil_num);	// TELL NEIGHBORS THAT FORKS ARE FREE
}


////////////// LOWER FORK IS ON RIGHT WHILE HIGHER IS ON LEFT. EXCEPT FOR PHIL 4 WHICH IS OPPOSITE //////////////////////////////

void pick_up_forks(int phil_num)
{

	int left = (phil_num + 1) % 5;
	int right = phil_num;
	int low;
	int high;


	// THIS IS FOR PHILOSOPHER 1-3
	if (right < left)
	{
		low = right;
		high = left;
	}
	// THIS IS FOR PHILOSOPHER 4
	else
	{
		low = left;
		high = right;
	}

///////////////////////// LOCK /////////////////////////

	pthread_mutex_lock(&fork_mutex);
	//first pick up left fork
	while(forks[low] == false)
	{
		pthread_cond_wait(fork_is_ready + low, &fork_mutex); // WHILE FORK IS IN USE, WAIT!
	}
	forks[low] = false; // ONCE IT BECOMES AVAILABLE, PICK LOWER UP

	// After you pick up left fork try to pick up right fork
	while(forks[high] == false)
	{
		pthread_cond_wait(fork_is_ready + high, &fork_mutex);
	}
	forks[high] = false; // ONCE IT BECOMES AVAILABLE, PICK HIGHER UP
	*(states + phil_num) = Eating; //CHANGE PHILOSOPHER STATE TO EATING
	pthread_mutex_unlock(&fork_mutex);

////////////////////// UNLOCK //////////////////////////

	signal_not_neighbors(phil_num); // TELL NOT NEIGHBORS THAT forkMutex IS FREE;
}

///////////////// SIGNAL THE PHIL THAT ARE NEIGHBORS THAT THE FORKS ARE READY ///////////////////////////////

void signal_neighbours(int phil_num)
{
	int right = phil_num; // left is the phil number
	int left = (phil_num + 1) % 5; // right is the phil number plus 1 except for the last one
	pthread_cond_signal(fork_is_ready + right);
	pthread_cond_signal(fork_is_ready + left);
}

////////////////////////// SIGNAL PHILS THAT ARE NOT NEIGHBORS THAT THE FORKS ARE READY ///////////////////
void signal_not_neighbors(int phil_num)
{
	int not_neighbor1 = (phil_num + 3) % 5;// CLOSER TO RIGHT SIDE
	int not_neighbor2 = (phil_num + 2) % 5;//CLOSER TO LEFT sIDE
	pthread_cond_signal(fork_is_ready + not_neighbor1);
	pthread_cond_signal(fork_is_ready + not_neighbor2);
}


// gcc -o dinphil dinphil.c -lpthread
