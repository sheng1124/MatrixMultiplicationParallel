#include <iostream>
#include <stdlib.h>
#include <time.h>
#include <vector>
#include <iomanip>
#include <pthread.h>
#define NUM_THREAD 8
using namespace std;

void * pd1(void *aug);
void * pd2(void *aug);
pthread_t threads[NUM_THREAD];
volatile int running_threads = 0, calculating_threads = 0;
pthread_mutex_t running_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t calculating_mutex = PTHREAD_MUTEX_INITIALIZER;

struct ThreadPara
{
	vector<vector<int>> *Carray2D;
	vector<vector<int>> *Aarray2D;
	vector<vector<int>> *Barray2D;
	
	vector<struct Index> *workTab;
	int size_;
	int threadIndex;
};

class Chronometer
{
public:
	const Chronometer & setStartTime();
	double getExeTime();
private:
	struct timespec t_start, t_end;
	double elapsedTime;
};
Chronometer A;

const Chronometer & Chronometer::setStartTime()
{
	clock_gettime( CLOCK_REALTIME, &t_start); 
	return *this;
}

double Chronometer::getExeTime()
{
	clock_gettime( CLOCK_REALTIME, &t_end);
	// compute and print the elapsed time in millisec
	elapsedTime = (t_end.tv_sec - t_start.tv_sec) * 1000.0;
	elapsedTime += (t_end.tv_nsec - t_start.tv_nsec) / 1000000.0;
	cout << "Sequential elapsedTime: " << elapsedTime << " ms" << endl;
	return elapsedTime;
}



class Matrix{
public:
	vector<vector<int>> array2D;
	int size_;
	
	Matrix(int n);
	const Matrix & randomize();
	bool isSame(const Matrix & a) const;
	void print() const;
	const Matrix & transposeMatrix();
	const Matrix & matrixMulti(const Matrix & a, const Matrix & b);
	const Matrix & transposeMatrixPthread();
	const Matrix & matrixMultiPthread(Matrix & a, Matrix & b);
private:
};

Matrix::Matrix(int n)
{
	vector<int> row;
	row.assign(n,0);
	array2D.assign(n,row);
	size_ = n;
}

const Matrix & Matrix::randomize()
{
	for(int i = 0; i < size_; i++){
		for(int j = 0; j < size_; j++){
			array2D[i][j] = rand() % 100;
		}
	}
	return *this;
}

bool Matrix::isSame(const Matrix & a) const
{
	if(a.size_ != size_) return false;
	
	for(int i = 0; i < size_; i++)
	{
		for(int j = 0; j < size_; j++)
		{
			if((a.array2D[i][j] != array2D[i][j]))
			{
				return false;
			}
		}
	}
	return true;
}

void Matrix::print() const
{
	for(int i = 0; i < size_; i++){
		for(int j = 0; j < size_; j++){
			cout << setw(3) << array2D[i][j] << " ";
		}
		cout << endl;
	}
	cout << endl;
}

const Matrix & Matrix::transposeMatrix()
{
	for(int i = 0; i < size_; i++){
		for(int j = i + 1; j < size_; j++){
			int temp = array2D[j][i];
			array2D[j][i] = array2D[i][j];
			array2D[i][j] = temp;
		}
	}
}

const Matrix & Matrix::matrixMulti(const Matrix & a, const Matrix & b)
{
	if(a.size_ != b.size_)
	{
		cerr << "size not match" << endl;
		return *this;
	}
	
	for(int i = 0; i < size_; i++)
	{
		for(int j = 0; j < size_; j++)
		{
			array2D[i][j] = 0;
			for(int k = 0; k < size_; k++)
			{
				array2D[i][j] += (a.array2D)[i][k] * (b.array2D)[j][k];
			}
		}
	}
	return *this;
}

const Matrix & Matrix::transposeMatrixPthread()
{	
	for(int i = 0; i < NUM_THREAD; i++)
	{   
		//生成參數
		struct ThreadPara *para = new struct ThreadPara;
		(*para).Carray2D = &array2D;
		(*para).size_ = size_;
		(*para).threadIndex = i;
		//參數轉型
		void *para_ = static_cast<void*>(para);
		pthread_mutex_lock(&running_mutex);
		running_threads++;
		pthread_mutex_unlock(&running_mutex);
		pthread_create(&threads[i],NULL,pd1, para_);
	}
	
	while(running_threads > 0)
	{
		;
	}
}

void * pd1 (void *para_)
{
	struct ThreadPara *para = static_cast<struct ThreadPara*>(para_);
	vector<vector<int>> *Carray2D = (*para).Carray2D;
	int size_ = (*para).size_;
	int threadIndex = (*para).threadIndex;
	
	for(int i = 0; i < size_; i++){
		for(int j = i + 1 + threadIndex; j < size_; j += NUM_THREAD){
			int temp = (*Carray2D)[j][i];
			(*Carray2D)[j][i] = (*Carray2D)[i][j];
			(*Carray2D)[i][j] = temp;
		}
	}
	
	pthread_mutex_lock(&running_mutex);
	running_threads--;
	pthread_mutex_unlock(&running_mutex);
	pthread_exit(NULL);
}

const Matrix & Matrix::matrixMultiPthread(Matrix & a, Matrix & b)
{
	for(int i = 0; i < NUM_THREAD; i++)
	{
		//生成參數
		struct ThreadPara *para = new struct ThreadPara;
		//設定引數
		(*para).Carray2D = &array2D;
		(*para).Aarray2D = &(a.array2D);
		(*para).Barray2D = &(b.array2D);
		(*para).size_ = size_;
		(*para).threadIndex = i;
		//參數轉型
		void *para_ = static_cast<void*>(para);
		//傳遞參數c
		pthread_mutex_lock(&running_mutex);
		running_threads++;
		pthread_mutex_unlock(&running_mutex);
		pthread_create(&threads[i],NULL,pd2, para_);
	}
	
	while(running_threads > 0)
	{
		;
	}
	
}

void * pd2 (void *para_)
{
	struct ThreadPara *para = static_cast<struct ThreadPara*>(para_);
	vector<vector<int>> *Carray2D = (*para).Carray2D;
	vector<vector<int>> *Aarray2D = (*para).Aarray2D;
	vector<vector<int>> *Barray2D = (*para).Barray2D;
	
	int size_ = (*para).size_;
	int threadIndex = (*para).threadIndex;
	
	int temp;
	
	for(int i = 0; i < size_; i++)
	{
		for(int j = 0; j < size_; j++)
		{	
			temp = 0;
			for(int k = threadIndex; k < size_; k += NUM_THREAD)
			{
				temp += (*Aarray2D)[i][k] * (*Barray2D)[j][k]; 
			}
			
			//進入存取 (*Carray2D)[i][j]
			
			pthread_mutex_lock(&calculating_mutex);
			(*Carray2D)[i][j] += temp;
			pthread_mutex_unlock(&calculating_mutex);			
		}
	}
	
	pthread_mutex_lock(&running_mutex);
	running_threads--;
	pthread_mutex_unlock(&running_mutex);
	pthread_exit(NULL);
}

int main(){
	int n;
	cout << "輸入陣列規模 -1離開" << endl;
	cin >> n;
	while(n != -1){
		Matrix a(n);
		Matrix b(n);
		Matrix c(n);
		Matrix golden(n);
		
		a.randomize();
		b.randomize();
		
		if(a.size_ <= 10)
		{
			cout << "a: " << endl;
			a.print();
			cout << "b: " << endl;
			b.print();
		}
		
		cout << "計算非平行化時間" << endl;
		A.setStartTime();
		b.transposeMatrix();
		golden.matrixMulti(a, b);
		A.getExeTime();
		
		cout << "計算平行化時間" << endl;
		b.transposeMatrix();
		A.setStartTime();
		b.transposeMatrixPthread();
		A.getExeTime();
		c.matrixMultiPthread(a, b);
		A.getExeTime();
		//c.print(n);
		cout << "HW3測試結果: ";
		if(a.size_ <= 10)
		{
			cout << "golden: " << endl;
			golden.print();
			cout << "c: " << endl;
			c.print();
		}
		
		if(c.isSame(golden)) cout << "一致" << endl << endl;
		else cout << "不一致" << endl << endl;

		cout << "輸入陣列規模 -1離開" << endl;
		cin >> n;
	}

	return 0;
}