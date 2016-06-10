/**
 * Task1_2,find the minimum value of the given double-variable function using genetic algorithm.
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

double l1 = -3.0, r1 = -3.0, p1 = 1e-5;
double l2 = -3.0, r2 = -3.0, p2 = 1e-5;

size_t PopulationSize = 200;
size_t GenerationCnt = 40;
double P_Cross = 0.65;
double P_Mutate = 0.001;

double objFunc(double x1, double x2)
{
	return pow(x1 - 1.5, 4) + pow(x2 + 1.5, 2);
}

class chromosome
{
private:
	string gene;
	size_t len1, len2;
	double objVal, fitVal;

	void exchange(chromosome &rhs)
	{
		swap(gene, rhs.gene);
		swap(x1, rhs.x1);
		swap(x2, rhs.x2);
		swap(len1, rhs.len1);
		swap(len1, rhs.len2);
		swap(objVal, rhs.objVal);
		swap(fitVal, rhs.fitVal);
	}

public:
	double x1, x2;

	chromosome(){}
	chromosome(size_t _l1, size_t _l2) :gene(_l1 + _l2, '0'), len1(_l1), len2(_l2)
	{

		randomize();
		decode();
		calcObj();
		calcFit();
	}
	chromosome(const chromosome &rhs) :gene(rhs.gene)
	{
		objVal = rhs.objVal;
		fitVal = rhs.fitVal;
		len1 = rhs.len1;
		len2 = rhs.len2;
		x1 = rhs.x1;
		x2 = rhs.x2;
	}
	chromosome& operator=(chromosome rhs)
	{
		exchange(rhs);
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

	/*�������ƻ�������ת��������������ʮ���Ʊ�ʾ*/
	void decode()
	{
		double tmp = 0;
		for (size_t i = 0; i < len1; i++)
			if (gene[i] == '1')
				tmp += pow(2, len1 - 1 - i);
		x1 = min(l1 + p1*tmp, r1);

		tmp = 0;
		for (size_t i = 0; i < len2; i++)
			if (gene[i + len1] == '1')
				tmp += pow(2, len2 - 1 - i);
		x2 = min(l2 + p2*tmp, r2);
	}

	/*����Ŀ�꺯��ֵ*/
	double calcObj()
	{
		return objVal = objFunc(x1, x2);
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
	ofstream fout("./results/Test1_2.txt");
	if (!fout)
		throw "Invalid output file path!\n";

	//�������
	cout << "Ŀ�꺯����f(x1,x2)=(x1-1.5)^4+(x2+1.5)^2" << endl;
	cout << "��������ز�����" << endl;
	cout << "x1ȡֵ��Χ(�Կո�俪��e.g. -3 3)��"; cin >> l1 >> r1;
	cout << "x1�ľ���(e.g. 0.001 or 1e-3)��"; cin >> p1;
	cout << "x2ȡֵ��Χ(�Կո�俪��e.g. -3 3)��"; cin >> l2 >> r2;
	cout << "x2�ľ���(e.g. 0.001 or 1e-3)��"; cin >> p2;
	cout << "��Ⱥ��ģ(e.g. 200 or 2e2)��"; cin >> PopulationSize;
	cout << "��������(e.g. 50 or 5e1)��"; cin >> GenerationCnt;
	cout << "�������(e.g. 0.65 or 6.5e-1)��"; cin >> P_Cross;
	cout << "�������(e.g. 0.001 or 1e-3)��"; cin >> P_Mutate;
	cout << "Calculating..." << endl;

	//Բ��
	size_t chromoLen1 = ceil(log2f((r1 - l1) / p1));
	p1 = (r1 - l1) / pow(2, chromoLen1);
	size_t chromoLen2 = ceil(log2l((r2 - l2) / p2));
	p2 = (r2 - l2) / pow(2, chromoLen2);

	fout << "��ǰ���Բ������£�" << endl;
	fout << "Ŀ�꺯����f(x1,x2)=(x1-1.5)^4+(x2+1.5)^2" << endl;
	fout << "x1ȡֵ���䣺[" << l1 << "," << r1 << "]" << endl;
	fout << "x1Բ�����ʵ�ʾ��ȣ�" << p1 << endl;
	fout << "x2ȡֵ���䣺[" << l2 << "," << r2 << "]" << endl;
	fout << "x2Բ�����ʵ�ʾ��ȣ�" << p2 << endl;
	fout << "���򴮳��ȣ�" << chromoLen1 + chromoLen2 << endl;
	fout << "��Ⱥ��ģ��" << PopulationSize << endl;
	fout << "����������" << GenerationCnt << endl;
	fout << "������ʣ�" << P_Cross << endl;
	fout << "������ʣ�" << P_Mutate << endl;
	fout << endl;

	//��ʼ����Ⱥ
	srand(time(NULL));
	vector<chromosome> grp_cur;
	for (size_t i = 0; i < PopulationSize; i++)
		grp_cur.push_back(chromosome(chromoLen1, chromoLen2));
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
			 << "��Ⱥ�����Ÿ���Ϊ��x1=" << grp_cur.back().x1 << "��x2=" << grp_cur.back().x2
			 << "����Ŀ�꺯�������µĽ��Ϊ��" << grp_cur.back().getObjVal()
			 << "����Ӧ��Ϊ��" << grp_cur.back().getFitVal() << endl;
	}

	delete[] hasCrossed;
    fout.close();
	return 0;
}

