/**
 * Task1_1,find the minimum value of the given single variable function using genetic algorithm.
 */

#include <iostream>
#include <fstream>
#include <iomanip>
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <cstring>

using namespace std;

const double eps = 1e-12;

double sectionLeft = -5.0;
double sectionRight = 5.0;

double precision = 1e-5;
size_t PopulationSize = 200;
size_t GenerationCnt = 40;
double P_Cross = 0.65;
double P_Mutate = 0.001;

double objFunc(double x)
{
	return pow(x - 2.5, 2);
}

class chromosome
{
private:
	string gene;
	double realVal, objVal, fitVal;

public:
	chromosome(){}
	chromosome(size_t len) :gene(len, '0')
	{
		randomize();
		decode();
		calcObj();
		calcFit();
	}
	chromosome(const chromosome &rhs) :gene(rhs.gene), realVal(rhs.realVal), objVal(rhs.objVal), fitVal(rhs.fitVal){}
	chromosome& operator=(chromosome rhs)
	{
		swap(gene, rhs.gene);
		swap(realVal, rhs.realVal);
		swap(objVal, rhs.objVal);
		swap(fitVal, rhs.fitVal);
		return *this;
	}
	~chromosome() {}

	bool operator<(const chromosome &rhs) const { return fitVal < rhs.fitVal; }

	/*��������Ļ�������*/
	void randomize()
	{
		for (size_t i = 0; i < gene.length(); i++)
		{
			int choice = rand() % 1000;
			if (choice < 500)
				gene[i] = '0';
			else
				gene[i] = '1';
		}
	}

	/*�������ƻ�������ת����ʮ���Ƶ�С��*/
	double decode()
	{
		double tmp = 0;
		const int n = gene.length();
		for (int i = n - 1; i >= 0; i--)
			if (gene[i] == '1')
				tmp += pow(2, n - 1 - i);

		return realVal = min(sectionLeft + tmp*precision, sectionRight);
	}

	/*����Ŀ�꺯��ֵ*/
	double calcObj()
	{
		return objVal = objFunc(realVal);
	}

	/*������Ӧֵ*/
	double calcFit()
	{
		return fitVal = -objVal;
	}

	/*���¼�������ֵ*/
	void update()
	{
		decode();
		calcObj();
		calcFit();
	}

	/*��ȡ��ؽ��*/
	double getRealVal() const { return realVal; }
	double getObjVal() const { return objVal; }
	double getFitVal() const { return fitVal; }

	/*������㽻��*/
	void crossOver(chromosome &partner, double probability)
	{
		if (gene.length() != partner.gene.length())
			throw "Chromosome not match!\n";
		if (probability < 0 || probability>1)
			throw "Invalid probability!\n";

		bool doSwap = false;
		if (fabs(probability) < eps)//Ԥ��Ľ������Ϊ0�����ֲ���
			return;
		else if (fabs(probability - 1) < eps)//Ԥ��ı������Ϊ1����Ȼ�в���Ҫ���н���
			doSwap = true;
		else
		{
			double choice = (rand() % 32749) / 32749.0;
			doSwap |= choice < probability;
		}

		if (doSwap)
		{
			int pos = rand() % gene.length();//Position to start swapping
			for (int i = pos; i < gene.length(); i++)
				swap(gene[i], partner.gene[i]);
		}
	}

	/*������λ����*/
	void mutate(double probability)
	{
		if (probability < 0 || probability>1)
			throw "Invalid probability!\n";

		const int total = '0' + '1';

		if (fabs(probability) < eps)//Ԥ��ı������Ϊ0�����ֲ���
			return;
		else if (fabs(probability - 1) < eps)//Ԥ��ı������Ϊ1��ȫ��ȡ��
			for (int i = 0; i < gene.length(); i++)
				gene[i] = total - gene[i];
		else
		{
			double choice = 0;
			for (int i = 0; i < gene.length(); i++)
			{
				choice = (rand() % 32749) / 32749.0;
				if (choice < probability)
					gene[i] = total - gene[i];
			}
		}
	}
};

int main(int argc, char **argv)
{
	ofstream fout("./results/Test1_1.txt");
	if (!fout)
		throw "Invalid output file path!\n";

	//�������
	cout << "Ŀ�꺯����f(x)=(x-2.5)^2" << endl;
	cout << "��������ز�����" << endl;
	cout << "������(�Կո�俪��e.g. -5 5)��"; cin >> sectionLeft >> sectionRight;
	cout << "��ľ���(e.g. 0.001 or 1e-3)��"; cin >> precision;
	cout << "��Ⱥ��ģ(e.g. 200 or 2e2)��"; cin >> PopulationSize;
	cout << "��������(e.g. 50 or 5e1)��"; cin >> GenerationCnt;
	cout << "�������(e.g. 0.65 or 6.5e-1)��"; cin >> P_Cross;
	cout << "�������(e.g. 0.001 or 1e-3)��"; cin >> P_Mutate;
	cout << "Calculating..." << endl;

	//Բ��
	size_t chromoLen = ceil(log2f((sectionRight - sectionLeft) / precision));
	precision = (sectionRight - sectionLeft) / pow(2, chromoLen);

	fout << "��ǰ���Բ������£�" << endl;
	fout << "Ŀ�꺯����f(x)=(x-2.5)^2" << endl;
	fout << "ȡֵ���䣺[" << sectionLeft << "," << sectionRight << "]" << endl;
	fout << "Բ�����ʵ�ʾ��ȣ�" << precision << endl;
	fout << "���򴮳��ȣ�" << chromoLen << endl;
	fout << "��Ⱥ��ģ��" << PopulationSize << endl;
	fout << "����������" << GenerationCnt << endl;
	fout << "������ʣ�" << P_Cross << endl;
	fout << "������ʣ�" << P_Mutate << endl;
	fout << endl;

	//��ʼ����Ⱥ
	srand(time(NULL));
	vector<chromosome> grp_cur;
	for (size_t i = 0; i < PopulationSize; i++)
		grp_cur.push_back(chromosome(chromoLen));
	sort(grp_cur.begin(), grp_cur.end());

	//��������
	bool *hasCrossed = new bool[PopulationSize];

	for (size_t k = 0; k < GenerationCnt; k++)
	{
		vector<chromosome> grp_next;		
		int c1 = -1, c2 = -1;

		//ѡ��
		for (size_t cnt = 0; cnt < PopulationSize; cnt++)
		{
			do{
				c1 = rand() % PopulationSize;
				c2 = rand() % PopulationSize;
			} while (c1 == c2);
			
			double f1 = grp_cur[c1].getFitVal();
			double f2 = grp_cur[c2].getFitVal();

			if (f1 > f2)
				grp_next.push_back(grp_cur[c1]);
			else
				grp_next.push_back(grp_cur[c2]);
		}

		//����
		memset(hasCrossed, 0, PopulationSize*sizeof(bool));

		for (size_t i = 0; i < PopulationSize; i += 2)
		{
			do{
				c1 = rand() % PopulationSize;
				c2 = rand() % PopulationSize;
			} while (c1 == c2 || hasCrossed[c1] || hasCrossed[c2]);

			grp_next[c1].crossOver(grp_next[c2], P_Cross);
			hasCrossed[c1] = true;
			hasCrossed[c2] = true;
		}

		//����
		for (size_t i = 0; i < PopulationSize; i++)
			grp_next[i].mutate(P_Mutate);

		//Update
		for (size_t i = 0; i < PopulationSize; i++)
			grp_next[i].update();
		sort(grp_next.begin(), grp_next.end());

		//Keep the best
		//swap(grp_next[0], grp_cur[grp_cur.size() - 1]);
		//sort(grp_next.begin(), grp_next.end());
		grp_cur = grp_next;

		fout << "��" << k << "�ε�����" << endl
			 << "����ֵΪ��" << grp_cur.back().getRealVal()
			 << "����Ŀ�꺯�������µĽ��Ϊ��" << grp_cur.back().getObjVal()
			 << "����Ӧ��Ϊ��" << grp_cur.back().getFitVal() << endl;
			 //<< "���ֵΪ��" << grp_cur.front().getRealVal()
			 //<< " ����Ŀ�꺯�������µĽ��Ϊ��" << grp_cur.front().getObjVal()
			 //<< " ,��Ӧ��Ϊ��" << grp_cur.front().getFitVal() << endl;
	}

	delete[] hasCrossed;
	fout.close();
	return 0;
}

