//complie g++ -std=c++11 hw1.cpp -fopenmp -o hw1
#include <iostream>
#include <stdlib.h>
#include <time.h>
#include <vector>
#include <iomanip>
#include <omp.h>
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
	const Matrix & transposeMatrixOmp();
	const Matrix & matrixMultiOmp(Matrix & a, Matrix & b);
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


const Matrix & Matrix::transposeMatrixOmp()
{
	#pragma omp parallel num_threads(NUM_THREAD)
	#pragma omp for schedule(dynamic)
	for(int i = 0; i < size_; i++){
		for(int j = i + 1; j < size_; j++){
			int temp = array2D[j][i];
			array2D[j][i] = array2D[i][j];
			array2D[i][j] = temp;
		}
	}
}

const Matrix & Matrix::matrixMultiOmp(Matrix & a, Matrix & b)
{
	if(a.size_ != b.size_)
	{
		cerr << "size not match" << endl;
		return *this;
	}
	int i,j,k;
	#pragma omp parallel private(i,j,k) num_threads(8)
	{
		#pragma omp for schedule(dynamic)
		for(i = 0; i < size_; i++)
		{
			for(j = 0; j < size_; j++)
			{
				array2D[i][j] = 0;
				for(k = 0; k < size_; k++)
				{
					array2D[i][j] += (a.array2D)[i][k] * (b.array2D)[j][k];
				}
			}
		}
	}
	
	return *this;
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
		b.transposeMatrixOmp();
		A.getExeTime();
		c.matrixMultiOmp(a, b);
		A.getExeTime();
		//c.print(n);
		cout << "HW1測試結果: ";
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