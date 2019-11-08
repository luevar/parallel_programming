#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <math.h>
#include <time.h>
#include <queue>
using std::vector;
using std::queue;

#define ERROR_CREATE_THREAD -11
#define ERROR_JOIN_THREAD -12
#define US_PER_SECOND 1000000000.0

//вектор очередей с задачами(числами) для побочных потоков
typedef vector<queue<int>> QueueArray;

//обертка для id, которое передается каждому потоку и через
//которое идет обращение к конкретной очереди в векторе
typedef struct Args_tag {
    int id;
} Args_t;

QueueArray queueArr;
pthread_mutex_t* mutexArr;
int readingContinues = 1;

//поиск самих множителей текущего числа и запись в файл потока
void valuesCalculation(FILE** fp, int id) {
	//если очередь задач для конкретного потока пуста
	if (queueArr[id].size() == 0) {
		//и основной поток закончил считывание чисел из входного файла
		if (!readingContinues) {
			//тогда файл закрывается и работа потока прекращается
			fclose(*fp);
			pthread_exit(NULL);
		} else {
			return;
		}
	}

	//иначе мы берем следующее в очереди число и проводим с ним все нужные операции
	//(важно работать с очередью через мьютекс, так как мы удаляем элементы в побочном потоке 
	//параллельно с добавлением элементов в основном потоке, что может привести к конфликту и потере данных)
	pthread_mutex_lock(&mutexArr[id]);
	int currentNumber = queueArr[id].front();
	printf("Pop %d from queue %d\n", currentNumber, id);
	queueArr[id].pop();
	pthread_mutex_unlock(&mutexArr[id]);
	
	char i_str[10];
	for (int i = 1; i <= currentNumber; i++) {
		if (currentNumber % i == 0) {
   			//Преобразуем i в строковый формат
			sprintf(i_str, "%d ", i);
			fputs(i_str, *fp);
		}
	}
	fputs("\n", *fp);
}


//функция, которая будет выполняться побочными потоками
void* findFactors(void* id_wrapper) {
	Args_t* arg = (Args_t*)id_wrapper;
	int id = arg->id;
	char buf[10];
	char id_str[10];

    //преобразуем id потока в строку
	sprintf(id_str, "%d", id);
  	//создаем имя нового файла для текущего потока, используя его id 
	snprintf(buf, sizeof(buf), "%s.txt", id_str);
	FILE* fp = fopen(buf, "w+");

	while(1) {
		valuesCalculation(&fp, id);
	}
}


//функция перевода строки в целое число
int stringToNum(char str[]) { 
	int result = 0;
	size_t len = strlen(str);
	for (int i = len; i > 0; i--) {
		result += (str[len - i] - '0') * (int)pow(10, i - 1);
	}
	return result;
}


int main() {
	FILE* inputFile;
	int threads_count, status;
	struct timespec start, finish;
	double elapsed;
	char buff[255];

	//открываем файл для чтения
	inputFile = fopen("input.txt", "r");
	if(!inputFile) {
		printf("Невозможно открыть файл.\n");
		exit(1);
	}

	printf("Input amount of threads = ");
	scanf("%d", &threads_count);
	pthread_t* threads = (pthread_t*)malloc(sizeof(pthread_t) * threads_count);
	queueArr.resize(threads_count);
	Args_t* args = (Args_t*)malloc(sizeof(Args_t) * threads_count);	
	mutexArr = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t) * threads_count);

	clock_gettime(CLOCK_MONOTONIC, &start);

	//создание побочных потоков
	for (int id = 0; id < threads_count; id++) {
		args[id].id = id;
		status = pthread_create(&threads[id], NULL, findFactors, (void*)&args[id]);
		if (status != 0) {
			printf("Can not create thread number %d\n", id + 1);
			exit(ERROR_CREATE_THREAD);
		}
	}

	//считывание чисел из входного файла и их равномерное распределение по побочным потокам (в очереди queueArr)
	int minIndex = 0;
	while (fscanf(inputFile, "%s", buff) != EOF)
	{	
		for (int i = 0; i < threads_count; i++) {
			if (queueArr[i].size() < queueArr[minIndex].size()) {
				minIndex = i;
			}
		}
		pthread_mutex_lock(&mutexArr[minIndex]);
		queueArr[minIndex].push(stringToNum(buff));
		printf("Push number %s to %d queue.\n", buff, minIndex);
		pthread_mutex_unlock(&mutexArr[minIndex]);
	}
	//когда чтение из входного файла заканчивается, то
	readingContinues = 0;

	//место синхронизации вспомогательных потоков с основным
	for (int i = 0; i < threads_count; i++) {
		status = pthread_join(threads[i], NULL);
		if (status != 0) {
			printf("Error, cant join thread %d from main thread.\n", i + 1);
			exit(ERROR_JOIN_THREAD);
		}
	}

	clock_gettime(CLOCK_MONOTONIC, &finish);
	elapsed = (finish.tv_sec - start.tv_sec);
	elapsed += (finish.tv_nsec - start.tv_nsec) / US_PER_SECOND;
	printf("Program runtime: %f\n", elapsed);
	fclose(inputFile);
	free(threads);
	free(args);
	free(mutexArr);
	queueArr.clear();
	queueArr.shrink_to_fit();	
}