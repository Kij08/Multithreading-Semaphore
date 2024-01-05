#include <pthread.h>
#include <stdio.h>
#include <semaphore.h>
#include <time.h>

#define MAXCUST 25

void* Customer(int custNum);
void* FrontDeskEmployee(int employeeNum);
void* Bellhop(int bellopNum);
void enqueueFrontDesk(int num);
int dequeueFrontDesk();
void enqueueBellhop(int num);
int dequeueBellhop();

sem_t mutexGuestQueue;
sem_t mutexRoom;
sem_t mutexBagsQueue;
sem_t frontDesk;
sem_t bellhop;
sem_t checkedIn[MAXCUST];
sem_t custWaiting;
sem_t keyReceived[MAXCUST];
sem_t needBellhop;
sem_t bagsTaken[MAXCUST];
sem_t enteredRoom[MAXCUST];
sem_t bagsDelivered[MAXCUST];
sem_t tipped[MAXCUST];
int custNumQueue[MAXCUST]; 
int qIn = 0;
int qOut = 0;
int bellhopGuestQueue[MAXCUST];
int qIn2 = 0;
int qOut2 = 0;
int roomNum = 1;
int GuestRooms[MAXCUST];
int guestMerged = 0;
int guestDeskEmployee[MAXCUST];
int guestBellhop[MAXCUST];

int main(){
	srand(time(NULL));
	int numGuests = 25;
	int numDesk = 2;
	int numBellhop = 2;
	
	
	pthread_t guestThreads[numGuests];
	pthread_t deskThreads[numDesk];
	pthread_t bellhopThreads[numBellhop];

	int i;

	for(i = 0; i < MAXCUST; i++){
		sem_init(&checkedIn[i], 0, 0); //Init all guest checkd in semaphores to 0
	}

	for(i = 0; i < MAXCUST; i++){
		sem_init(&bagsDelivered[i], 0, 0); //Init all bagsDelivered semaphores to 0
	}

	for(i = 0; i < MAXCUST; i++){
		sem_init(&bagsTaken[i], 0, 0); //Init all bagsTaken semaphores to 0
	}

	for(i = 0; i < MAXCUST; i++){
		sem_init(&enteredRoom[i], 0, 0); //Init all entered room semaphores to 0
	}

	for(i = 0; i < MAXCUST; i++){
		sem_init(&tipped[i], 0, 0); //Init all tipped semaphores to 0
	}

	for(i = 0; i < MAXCUST; i++){
		sem_init(&keyReceived[i], 0, 0); //Init all tipped semaphores to 0
	}


	sem_init(&mutexGuestQueue, 0, 1); //Init queue mutex with 1
	sem_init(&mutexRoom, 0, 1); //Init room mutex with 1
	sem_init(&mutexBagsQueue, 0, 1); //Init belhlhop queue with 1
	sem_init(&frontDesk, 0, 2); //Front desk init with 2
	sem_init(&bellhop, 0, 2); //Bellhop init with 2
	sem_init(&custWaiting, 0, 0); //Custwaiting init with 0
	//sem_init(&keyReceived, 0, 0); //Init key received with 0
	sem_init(&needBellhop, 0, 0); //needBellhop init with 0
	//sem_init(&bagsTaken, 0, 0); //Bags taken init with 0
	//sem_init(&tipped, 0, 0); //tipped init with 0

	

	for(i = 0; i < numDesk; i++){
		printf("Front desk employee %i created\n", i);
		pthread_create(&deskThreads[i], NULL, FrontDeskEmployee, (void*)i);
	}

	for(i = 0; i < numBellhop; i++){
		printf("Bellhop %i created\n", i);
		pthread_create(&bellhopThreads[i], NULL, Bellhop, (void*)i);
	}

	for(i = 0; i < numGuests; i++){
		printf("Guest %i created\n", i);
		pthread_create(&guestThreads[i], NULL, Customer, (void*)i);
	}


	for(i = 0; i < numGuests; i++){
		pthread_join(guestThreads[i], NULL);
		printf("Guest %i joined\n", i);
		
	}
	guestMerged = 1;

	for(i = 0; i < numDesk; i++){
		pthread_cancel(deskThreads[i]);
	}


	for(i = 0; i < numBellhop; i++){
		pthread_cancel(bellhopThreads[i]);
	}


	printf("Simulation ends\n");
	return 0;
}

void* Customer(int custNum){
	int bagCount = rand() % 6; //Random bag from 1-5
	//int bagCount = 1;
	printf("Guest %i enters hotel with %i bags\n", custNum, bagCount);
	sem_wait(&frontDesk); //Wait for front desk available
	sem_wait(&mutexGuestQueue); //Mutex wait for enqueue
	enqueueFrontDesk(custNum);
	sem_post(&mutexGuestQueue); 
	//sem_wait(&frontDesk); //Wait for front desk available
	sem_post(&custWaiting); //Let employee know a cust is ready
	sem_wait(&checkedIn[custNum]); //Wait for correct guest number to receive the checked in signal
	printf("Guest %i receives roomkey for room %i from front desk employee %i\n", custNum, GuestRooms[custNum], guestDeskEmployee[custNum]);
	sem_post(&keyReceived[custNum]); //Signal front desk that key is received

	if(bagCount > 2){
		printf("Guest %i requests help with their bags\n", custNum);
		sem_wait(&bellhop);
		sem_wait(&mutexBagsQueue); //Mutex wait for enqueue
		enqueueBellhop(custNum);
		sem_post(&mutexBagsQueue);
		sem_post(&needBellhop);
		sem_wait(&bagsTaken[custNum]);
		printf("Guest %i enters room %i\n", custNum, GuestRooms[custNum]);
		sem_post(&enteredRoom[custNum]);
		sem_wait(&bagsDelivered[custNum]);
		printf("Guest %i receives bags from bellhop %i and gives tip\n", custNum, guestBellhop[custNum]);
		sem_post(&tipped[custNum]);
		printf("Guest %i retires for the evening\n", custNum);

	} 
	else{
		printf("Guest %i enters room %i\n", custNum, GuestRooms[custNum]);
		printf("Guest %i retires for the evening\n", custNum);
	}
}

void* FrontDeskEmployee(int employeeNum){
	int custNum;
	while(guestMerged == 0){
		sem_wait(&custWaiting); //Wait for a cust to be ready
		sem_wait(&mutexGuestQueue); //mutex wait for cust queue
		custNum = dequeueFrontDesk();
		guestDeskEmployee[custNum] = employeeNum;
		sem_post(&mutexGuestQueue);
		sem_wait(&mutexRoom); //Mutex wait for setting cutomer room
		GuestRooms[custNum] = roomNum;
		roomNum++;
		sem_post(&mutexRoom);
		printf("Front desk employee %i registers guest %i and assigns room %i\n", employeeNum, custNum, GuestRooms[custNum]);
		sem_post(&checkedIn[custNum]); //Post checked in for specific guest num
		sem_wait(&keyReceived[custNum]); //Wait for guest to get key
		sem_post(&frontDesk); //An employee is avaliable again


	}
}

void* Bellhop(int bellhopNum){
	int guestNum;
	while(guestMerged == 0){
		sem_wait(&needBellhop);
		sem_wait(&mutexBagsQueue);
		guestNum = dequeueBellhop();
		guestBellhop[guestNum] = bellhopNum;
		sem_post(&mutexBagsQueue);
		printf("Bellhop %i receives bags from guest %i\n", bellhopNum, guestNum);
		sem_post(&bagsTaken[guestNum]);
		sem_wait(&enteredRoom[guestNum]);
		printf("Bellhop %i delivered bags to guest %i\n", bellhopNum, guestNum);
		sem_post(&bagsDelivered[guestNum]);
		sem_wait(&tipped[guestNum]);
		sem_post(&bellhop);

	}
}

//Very rudimentary queue implementation lol
void enqueueFrontDesk(int num){
	if(qIn > 24){
		return;
	}
	custNumQueue[qIn] = num;
	qIn += 1;
}

int dequeueFrontDesk(){
	if(qOut > 24){
		return -1;
	}
	int i = custNumQueue[qOut];
	qOut += 1;
	return i;
}




void enqueueBellhop(int num){
        if(qIn2 > 24){
		return;
	}
	bellhopGuestQueue[qIn2] = num;
	qIn2 += 1;
}	

int dequeueBellhop(){
	if(qOut2 > 24){
		return -1;
	}
	int i = bellhopGuestQueue[qOut2];
	qOut2 += 1;
	return i;
}
