#include <iostream>
#include <fstream>
#include <iomanip>
#include <cstring>
#include <vector>
#include <algorithm>
#include <cmath>
#include <numeric>
#include <limits>
#include <cassert>

using namespace std;

void convertToPhysicalVar(vector<double> &conservative, vector<double> &primitive);
void convertToConservativeVar(vector<double> &primitive, vector<double> &conservative);
void Init();
void TimeMarching(int step);
void Output_FlowField();
void Output_PressureDistribution();
void RungeKutta();
void calcResidue();
void calcAerodynamics();
void RungeKutta_SubStep(int step);

ifstream mesh("../data/naca0012.grd");
ofstream conv("../result/convergence_history.dat");
ofstream flow("../result/flow_field.dat");
ofstream pd("../result/pressure_distribution.dat");

const double eps = 1e-9;
const double pi = 4 * atan(1);

int cnt_point = 0;
int cnt_edge = 0;
int cnt_cell = 0;

const double gama = 1.4;
double Ma = 0.8;
double angle_of_attack = 5;//degree
double p_inf = 101325.0;//Pa
double rho_inf = 1.225;//Kg/m^3
double c_inf = sqrt(gama*p_inf / rho_inf);//�������٣���ճ���õ��ع�ϵʽ
double u_inf = Ma*c_inf*cos(angle_of_attack*pi / 180);
double v_inf = Ma*c_inf*sin(angle_of_attack*pi / 180);
double s_inf = p_inf / pow(rho_inf, gama);
double ke_inf = 0.5*rho_inf*pow(Ma*c_inf, 2);

vector<double> w_inf//�������غ���
{ 
	rho_inf,
	rho_inf*u_inf,
	rho_inf*v_inf,
	p_inf / (gama - 1) + rho_inf*(pow(u_inf,2) + pow(v_inf,2))*0.5 
};
vector<double> primitives_inf
{
	rho_inf,
	u_inf,
	v_inf,
	p_inf
};

double k2 = 0.75;
double k4 = 0.015;
double CFL = 1.2;

const int RungeKutta_STEP = 4;
const double RungeKutta_alpha[RungeKutta_STEP + 1] = { 0,1.0 / 4,1.0 / 3,1.0 / 2,1 };

double err_rho = 1.0;
double err_vel = 1.0;
double err_p = 1.0;
double cl = 0;
double cd = 0;

int STEP = 2000;

class Point
{
	friend ostream &operator<<(ostream &fout, const Point &p);

private:
	//opertaor=�ĸ�������
	void exchange(Point &rhs)
	{
		swap(coordinate, rhs.coordinate);
	}

public:
	vector<double> coordinate;

	//���캯��
	Point() {}
	Point(double _x, double _y) :coordinate(vector<double>{_x, _y}) {}
	Point(double _x, double _y, double _z) :coordinate(vector<double>{_x, _y, _z}) {}
	Point(const vector<double> &_coord) :coordinate(_coord) {}
	Point(const Point &rhs) :coordinate(rhs.coordinate) {}
	
	//��������
	~Point() {}
	
	//���ظ�ֵ�����
	Point &operator=(Point rhs)
	{
		exchange(rhs);
		return *this;
	}

	//����һ��ļ��ξ���
	double DistanceTo(const Point &rhs) const
	{
		assert(coordinate.size() == rhs.coordinate.size());

		double len = 0;
		for (int i = 0; i < coordinate.size(); i++)
			len += pow(coordinate[i] - rhs.coordinate[i], 2);
		return sqrt(len);
	}

	//����һ���ʸ��
	vector<double> DeltaTo(const Point &rhs) const
	{
		assert(coordinate.size() == rhs.coordinate.size());

		vector<double> ans(coordinate.size(), 0);
		for (int i = 0; i < coordinate.size(); i++)
			ans[i] = rhs.coordinate[i] - coordinate[i];
		return ans;
	}
};

vector<Point> point;

class Cell
{
	friend void convertToConservativeVar(vector<double> &primitive, vector<double> &conservative);
	friend void convertToPhysicalVar(vector<double> &conservative, vector<double> &primitive);

private:
	//opertaor=�ĸ�������
	void exchange(Cell &rhs)
	{
		swap(point, rhs.point);
		swap(volum, rhs.volum);
		swap(w, rhs.w);
		swap(w_next, rhs.w_next);
		swap(physicalVar, rhs.physicalVar);
		swap(pre_physicalVar, rhs.pre_physicalVar);

		swap(localTimeStep, rhs.localTimeStep);
		swap(Q, rhs.Q);
		swap(D, rhs.D);
		swap(ddw, rhs.ddw);
	}

public:
	vector<int> point;
	double volum;
	vector<double> w;//rho,rho*u,rho*v,rho*E
	vector<double> w_next;
	vector<double> physicalVar;//rho,u,v,p
	vector<double> pre_physicalVar;

	double localTimeStep;
	vector<double> Q;//cell�Ķ���ͨ��
	vector<double> D;//cell�ĺ�ɢͨ��
	vector<double> ddw;//�غ���w_next��Laplace

	//���캯��
	Cell() {}
	Cell(const vector<int> &_pts):
		point(_pts),
		volum(0),
		w(w_inf),
		physicalVar(primitives_inf),
		w_next(vector<double>(w_inf.size(), 0)),
		pre_physicalVar(vector<double>(primitives_inf.size(), 0)),
		localTimeStep(0),
		Q(vector<double>(w_inf.size(), 0)),
		D(vector<double>(w_inf.size(), 0)),
		ddw(vector<double>(w_inf.size(), 0)) {}
	Cell(const Cell &rhs):
		point(rhs.point),
		volum(rhs.volum),
		w(rhs.w),
		physicalVar(rhs.physicalVar),
		w_next(rhs.w_next),
		pre_physicalVar(rhs.pre_physicalVar),
		localTimeStep(rhs.localTimeStep),
		Q(rhs.Q),
		D(rhs.D),
		ddw(rhs.ddw) {}

	//��������
	~Cell() {}

	//��ֵ����
	Cell &operator=(Cell rhs)
	{
		exchange(rhs);
		return *this;
	}

	void updatePrevRecord()
	{
		pre_physicalVar = physicalVar;
	}

	double getDensity() const
	{
		return physicalVar[0];
	}
	double getDensityDiff() const
	{
		return fabs(physicalVar[0] - pre_physicalVar[0]);
	}

	double getVelocity() const
	{
		return sqrt(pow(physicalVar[1], 2) + pow(physicalVar[2], 2));
	}
	double getVelocityDiff() const
	{
		return sqrt(pow(physicalVar[1] - pre_physicalVar[1], 2) + pow(physicalVar[2] - pre_physicalVar[2], 2));
	}

	double getPressure() const
	{
		return physicalVar[3];
	}
	double getPressureDiff() const
	{
		return fabs(physicalVar[3] - pre_physicalVar[3]);
	}
};

vector<Cell> cell;

class Edge
{
	friend void convertToConservativeVar(vector<double> &primitive, vector<double> &conservative);
	friend void convertToPhysicalVar(vector<double> &conservative, vector<double> &primitive);

private:
	//opertaor=�ĸ�������
	void exchange(Edge &rhs)
	{
		swap(start, rhs.start);
		swap(end, rhs.end);
		swap(len, rhs.len);
		swap(delta, rhs.delta);	
		swap(delta_n, rhs.delta_n);

		swap(w_av, rhs.w_av);
		swap(physicalVar, rhs.physicalVar);
		swap(c, rhs.c);
		swap(Z, rhs.Z);
		swap(convective_flux, rhs.convective_flux);

		swap(w_diff, rhs.w_diff);
		swap(ScalingFactor, rhs.ScalingFactor);
		swap(v, rhs.v);
		swap(eps2, rhs.eps2);
		swap(eps4, rhs.eps4);
		swap(d2, rhs.d2);
		swap(d4, rhs.d4);
	}

	//�����޷���߽���������Զ����
	//�õ�w_av,rho,u,v,p,c
	void calcFarBound()
	{
		double Vn_inf = u_inf*delta_n[0] + v_inf*delta_n[1];
		if (fabs(Vn_inf / c_inf) >= 1)
		{
			if (Vn_inf < 0)//����������,���ϵ�������������һ��
			{
				w_av = w_inf;
				physicalVar = primitives_inf;
				c = c_inf;
			}
			else//�����ٳ��������ϵ������������ĵ�Ԫһ��
			{
				w_av = cell[leftCell].w_next;
				convertToPhysicalVar(w_av, physicalVar);
				c = sqrt(gama*physicalVar[3] / physicalVar[0]);
			}
		}
		else
		{
			w_av = cell[leftCell].w_next;
			convertToPhysicalVar(w_av, physicalVar);
			c = sqrt(gama*physicalVar[3] / physicalVar[0]);//�ڲ൥Ԫ�����٣��б��������c
			double Vn_cell = physicalVar[1] * delta_n[0] + physicalVar[2] * delta_n[1];

			//Riemann Invariants
			double R_plus = Vn_cell + 2 * c / (gama - 1);
			double R_minus = Vn_inf - 2 * c_inf / (gama - 1);

			//������Riemann�������õ��߽��ϵķ����ٶ�������
			double Vn = 0.5*(R_plus + R_minus);
			c = 0.25*(gama - 1)*(R_plus - R_minus);

			if (Vn < 0)//����������
			{
				double s = s_inf;
				physicalVar[0] = pow(pow(c,2) / (gama*s), 1.0 / (gama - 1));
				physicalVar[1] = u_inf + (Vn - Vn_inf)*delta_n[0];
				physicalVar[2] = v_inf + (Vn - Vn_inf)*delta_n[1];
				physicalVar[3] = physicalVar[0] * pow(c, 2) / gama;

				convertToConservativeVar(physicalVar, w_av);
			}
			else//�����ٳ���
			{
				double s = physicalVar[3] / pow(physicalVar[0], gama);
				physicalVar[0] = pow(pow(c, 2) / (gama*s), 1.0 / (gama - 1));
				physicalVar[1] += (Vn - Vn_cell)*delta_n[0];
				physicalVar[2] += (Vn - Vn_cell)*delta_n[1];
				physicalVar[3] = physicalVar[0] * pow(c, 2) / gama;

				convertToConservativeVar(physicalVar, w_av);
			}
		}
	}

public:
	int start, end;
	int leftCell, rightCell;
	double len;
	vector<double> delta;
	vector<double> delta_n;//��λ�ⷨ����(����)
	
	vector<double> w_av;//���ߵ�Ԫ����������ƽ��
	vector<double> physicalVar;//���ϵ���������rho,u,v,p
	double c;//��������
	double Z;//���ڸ�������
	vector<double> convective_flux;//�ñ��ϵĶ���ͨ��

	vector<double> w_diff;//���ߵ�Ԫ������֮��,��-��û��������
	double ScalingFactor;//�װ뾶*�߳�+fabs(Z)
	double v;//����̽����
	double eps2, eps4;//����̽����
	vector<double> d2, d4;//components of the dissipation_flux
	
	//���캯��
	Edge() {}
	Edge(const vector<Point> &pts, int _start, int _end, int _lc, int _rc):
		start(_start),
		end(_end), 
		leftCell(_lc), 
		rightCell(_rc),
		delta(pts[_start].DeltaTo(pts[_end])),
		w_av(vector<double>(w_inf.size(),0)),
		physicalVar(vector<double>(primitives_inf.size(),0)),
		c(0),
		Z(0),
		convective_flux(vector<double>(w_inf.size(),0)),
		w_diff(vector<double>(w_inf.size(),0)),
		ScalingFactor(0),
		v(0),
		eps2(0),
		eps4(0),
		d2(vector<double>(w_inf.size(),0)),
		d4(vector<double>(w_inf.size(),0))
	{
		double length = 0;
		for (int i = 0; i < delta.size(); i++)
			length += pow(delta[i], 2);
		len = sqrt(length);

		delta_n = vector<double>{ delta[1] / len,-delta[0] / len };
	}
	Edge(const Edge &rhs):
		start(rhs.start),
		end(rhs.end),
		leftCell(rhs.leftCell),
		rightCell(rhs.rightCell),
		len(rhs.len),
		delta(rhs.delta),
		delta_n(rhs.delta_n),
		w_av(rhs.w_av),
		physicalVar(rhs.physicalVar),
		c(rhs.c),
		Z(rhs.Z),
		convective_flux(rhs.convective_flux),
		w_diff(rhs.w_diff),
		ScalingFactor(rhs.ScalingFactor),
		v(rhs.v),
		eps2(rhs.eps2),
		eps4(rhs.eps4),
		d2(rhs.d2),
		d4(rhs.d4) {}
	
	//��������
	~Edge() {}

	//���ظ�ֵ����
	Edge &operator=(Edge rhs)
	{
		exchange(rhs);
		return *this;
	}

	//ȡ���е�
	Point getMidPoint() const
	{
		assert(point[start].coordinate.size() == point[end].coordinate.size());

		vector<double> mid_point(point[start].coordinate.size());
		for (int i = 0; i < mid_point.size(); i++)
			mid_point[i] = 0.5*(point[start].coordinate[i] + point[end].coordinate[i]);

		return Point(mid_point);
	}

	//����ñ��ϵ�w_av�����ݲ�ͬ��������ͬ����
	//��һ���õ����ϵ�rho,u,v,p,c,Z,ScalingFactor
	//ע�⣺������ֻ����Runge-Kutta�еĵ��������Դ�ÿ��cell�е�w_next��ȡ����
	void calcBasicVariables()
	{
		if (rightCell >= 0)//�ڲ���
		{
			for (int i = 0; i < w_av.size(); i++)
				w_av[i] = 0.5*(cell[leftCell].w_next[i] + cell[rightCell].w_next[i]);

			convertToPhysicalVar(w_av, physicalVar);
			c = sqrt(gama*physicalVar[3] / physicalVar[0]);
			Z = physicalVar[1] * delta[1] - physicalVar[2] * delta[0];
		}
		else if (rightCell == -1)//����
		{
			for (int i = 0; i < w_av.size(); i++)
				w_av[i] = cell[leftCell].w_next[i];

			convertToPhysicalVar(w_av, physicalVar);
			c = sqrt(gama*physicalVar[3] / physicalVar[0]);
			Z = 0;
		}
		else//Զ��
		{
			calcFarBound();
			Z = physicalVar[1] * delta[1] - physicalVar[2] * delta[0];
		}
		ScalingFactor = fabs(Z) + c*len;
	}

	//����ñ��ϵĶ���ͨ��Q
	void calcConvection()
	{
		convective_flux[0] = Z*physicalVar[0];
		convective_flux[1] = convective_flux[0] * physicalVar[1] + physicalVar[3] * delta[1];
		convective_flux[2] = convective_flux[0] * physicalVar[2] - physicalVar[3] * delta[0];
		convective_flux[3] = Z*(physicalVar[3] * (gama / (gama - 1)) + 0.5*physicalVar[0] * (pow(physicalVar[1], 2) + pow(physicalVar[2], 2)));
	}

	//���ñ��ϵĶ���ͨ��Q�ۼӵ��������cell
	void addConvectionToAdjcentCell()
	{
		for (int i = 0; i < convective_flux.size(); i++)
			cell[leftCell].Q[i] += convective_flux[i];

		if (rightCell >= 0)
		{
			for (int i = 0; i < convective_flux.size(); i++)
				cell[rightCell].Q[i] -= convective_flux[i];
		}
	}

	//�������˹���ɢ��ص�����v��eps2,eps4,w_diff(��-��),d2
	//�ڹ����н�w_diff�ۼӵ�������ĵ�Ԫ
	//ע�⣺��ɢ��ص�����Ҫ�ߵ����඼��cell,���˴���δ��ʾҪ��Ӧ�ڵ��ù���������
	//����ֻ����Runge-Kutta�еĵ�����������cell��w_next������w
	void calcDissipationRelatedVariables()
	{
		//�˹���ɢ��ص���
		v = fabs((cell[rightCell].getPressure() - cell[leftCell].getPressure()) / (cell[rightCell].getPressure() + cell[leftCell].getPressure()));
		eps2 = k2*v;
		eps4 = max(0.0, k4 - eps2);
		for (int i = 0; i < w_diff.size(); i++)
			w_diff[i] = cell[rightCell].w_next[i] - cell[leftCell].w_next[i];
		for (int i = 0; i < d2.size(); i++)
			d2[i] = ScalingFactor*eps2*w_diff[i];

		//�ۼ�w_diff
		for (int i = 0; i < w_diff.size(); i++)
		{
			cell[leftCell].ddw[i] += w_diff[i];
			cell[rightCell].ddw[i] -= w_diff[i];
		}
	}

	//����d4
	//ע�⣺��ɢ��ص�����Ҫ�ߵ����඼��cell,���˴���δ��ʾҪ��Ӧ�ڵ��ù���������
	void calc_d4()
	{
		for (int i = 0; i < d4.size(); i++)
			d4[i] = -ScalingFactor*eps4*(cell[rightCell].ddw[i] - cell[leftCell].ddw[i]);
	}

	//���ñ��ϵĺ�ɢͨ��D=d2+d4�ۼӵ��������cell,ע������
	//ע�⣺��ɢ��ص�����Ҫ�ߵ����඼��cell,���˴���δ��ʾҪ��Ӧ�ڵ��ù���������
	void addDissipationToAdjcentCell()
	{
		assert(d2.size() == d4.size());

		for (int i = 0; i < d2.size(); i++)
		{
			cell[leftCell].D[i] += (d2[i] + d4[i]);
			cell[rightCell].D[i] -= (d2[i] + d4[i]);
		}
	}
};

vector<Edge> edge;

int main(int argc, char **argv)
{
	/*
	cout << "����������������" << endl;
	cout << "Ma(e.g. 0.8):"; cin >> Ma;
	cout << "ӭ�ǣ�e.g. 3����"; cin >> angle_of_attack;
	*/

	//�����������ݲ���ʼ��
	Init();

	//����
	for (int step = 0; step < STEP; step++)
		TimeMarching(step);

	//�����̬���
	Output_FlowField();
	Output_PressureDistribution();

	mesh.close();
	conv.close();
	flow.close();
	pd.close();
	return 0;
}

void convertToPhysicalVar(vector<double> &conservative, vector<double> &primitive)
{
	primitive[0] = conservative[0];
	primitive[1] = conservative[1] / conservative[0];
	primitive[2] = conservative[2] / conservative[0];
	primitive[3] = (gama - 1)*(conservative[3] - 0.5*conservative[0] * (pow(primitive[1], 2) + pow(primitive[2], 2)));
}

void convertToConservativeVar(vector<double> &primitive, vector<double> &conservative)
{
	conservative[0] = primitive[0];
	conservative[1] = primitive[0] * primitive[1];
	conservative[2] = primitive[0] * primitive[2];
	conservative[3] = primitive[3] / (gama - 1) + 0.5*primitive[0] * (pow(primitive[1], 2) + pow(primitive[2], 2));
}

void Init()
{
	//���������ڹ��캯�������ó�ֵ
	mesh >> cnt_point >> cnt_edge >> cnt_cell;

	double x, y;
	for (int i = 0; i < cnt_point; i++)
	{
		mesh >> x >> y;
		point.push_back(Point(x, y));
	}

	int a, b, lc, rc;
	for (int i = 0; i < cnt_edge; i++)
	{
		mesh >> a >> b >> lc >> rc;
		edge.push_back(Edge(point, a - 1, b - 1, lc - 1, rc > 0 ? rc - 1 : rc));
	}

	vector<int> pts(3, -1);
	for (int i = 0; i < cnt_cell; i++)
	{
		for (int k = 0; k < 3; k++)
		{
			mesh >> pts[k];
			--pts[k];
		}
		cell.push_back(Cell(pts));
	}

	for (int i = 0; i < cnt_cell; i++)
		mesh >> cell[i].volum;
}

void TimeMarching(int step)
{
	//cerr << "In Step " << step << endl;

	for (int i = 0; i < cnt_cell; i++)
		cell[i].w_next = cell[i].w;

	RungeKutta();

	for (int i = 0; i < cnt_cell; i++)
	{
		cell[i].w = cell[i].w_next;
		cell[i].updatePrevRecord();
		convertToPhysicalVar(cell[i].w, cell[i].physicalVar);
	}

	calcResidue();
	calcAerodynamics();

	cerr << setw(8) << step
		<< setw(16) << err_rho
		<< setw(16) << err_vel
		<< setw(16) << err_p
		<< setw(16) << cl
		<< setw(16) << cd
		<< endl;

	conv << setw(8) << step
		<< setw(16) << err_rho
		<< setw(16) << err_vel
		<< setw(16) << err_p
		<< setw(16) << cl
		<< setw(16) << cd
		<< endl;
}

void Output_FlowField()
{
	flow << "TITLE = \"Flow Field for NACA0012\"" << endl;
	flow << "VARIABLES = \"X\", \"Y\", \"DENSITY\", \"U\", \"V\", \"P\"" << endl;
	flow << "ZONE  N = " << cnt_point << ", E = " << cnt_cell << ", ZONETYPE = FETRIANGLE" << endl;
	flow << "DATAPACKING = BLOCK" << endl;
	flow << "VARLOCATION = ([1-2] = NODAL, [3-6] = CELLCENTERED)" << endl;

	//�ڵ�X,Y����
	for (int k = 0; k < 2; k++)
		for (int i = 0; i < cnt_point; i++)
			flow << point[i].coordinate[k] << endl;
	
	//ÿ����Ԫ��DENSITY,U,V,P
	for (int k = 0; k < 4; k++)
		for (int i = 0; i < cnt_cell; i++)
			flow << cell[i].physicalVar[k] << endl;

	//��Ԫ��ڵ�Ķ�Ӧ��ϵ
	for (int i = 0; i < cnt_cell; i++)
	{
		for (int k = 0; k < 3; k++)
			flow << cell[i].point[k] + 1 << ' ';
		flow << endl;
	}

}

void Output_PressureDistribution()
{
	vector<pair<Point,double>> up, down;

	for (auto e : edge)
	{
		if (e.rightCell != -1)
			continue;

		if (e.delta_n[1] < 0)//������
			up.push_back(make_pair(e.getMidPoint(), cell[e.leftCell].getPressure()));
		else//������
			down.push_back(make_pair(e.getMidPoint(), cell[e.leftCell].getPressure()));
	}

	for (auto elem : up)
		pd << elem.first << setw(16) << elem.second << endl;

	pd << endl;

	for (auto elem : down)
		pd << elem.first << setw(16) << elem.second << endl;
}

void RungeKutta()
{
	//4��Runge-Kutta����
	for (int i = 1; i <= RungeKutta_STEP; i++)
		RungeKutta_SubStep(i);
}

void calcResidue()
{
	double a = 0, b = 0;

	for (int i = 0; i < cnt_cell; i++)
	{
		a += cell[i].getDensityDiff();
		b += cell[i].getDensity();
	}
	err_rho = a / b;

	a = b = 0;
	for (int i = 0; i < cnt_cell; i++)
	{
		a += cell[i].getVelocityDiff();
		b += cell[i].getVelocity();
	}
	err_vel = a / b;

	a = b = 0;
	for (int i = 0; i < cnt_cell; i++)
	{
		a += cell[i].getPressureDiff();
		b += cell[i].getPressure();
	}
	err_p = a / b;
}

void calcAerodynamics()
{
	double Fx = 0, Fy = 0;
	double Fp = 0;

	for (auto e : edge)
	{
		if (e.rightCell != -1)
			continue;

		Fp = cell[e.leftCell].getPressure() * e.len;

		Fx += Fp*e.delta_n[0];
		Fy += Fp*e.delta_n[1];
	}

	cd = Fx / ke_inf;
	cl = Fy / ke_inf;
}

void RungeKutta_SubStep(int step)
{
	//����ÿ����Ԫ�е�Q,D,ddw
	for (int i = 0; i < cnt_cell; i++)
	{
		fill(cell[i].Q.begin(), cell[i].Q.end(), 0);
		fill(cell[i].D.begin(), cell[i].D.end(), 0);
		fill(cell[i].ddw.begin(), cell[i].ddw.end(), 0);
	}

	//ͨ������ÿ����Ԫ�е�Q
	for (int i = 0; i < cnt_edge; i++)
	{
		//�õ�ÿ�����ϵ�w,rho,u,v,p,c,Z,ScalingFactor
		edge[i].calcBasicVariables();
		
		//�õ�ÿ�����ϵ�Q
		edge[i].calcConvection();

		//ÿ�����ϵ�Q�ۼӵ�������cell
		edge[i].addConvectionToAdjcentCell();
	}

	//ͨ������ÿ����Ԫ�е�D
	for (int i = 0; i < cnt_edge; i++)
	{
		if (edge[i].rightCell < 0)
			continue;

		//����ÿ���������˹���ɢ��ص�����v��eps2, eps4, w_diff,d2
		//���ڹ����н�w_diff�ۼӵ�������ĵ�Ԫ
		edge[i].calcDissipationRelatedVariables();
	}

	//ÿ����Ԫ��ddw�������ˣ�������ÿ�����ϵ�d4��Ȼ���ۼӵ�������cell
	for (int i = 0; i < cnt_edge; i++)
	{
		if (edge[i].rightCell < 0)
			continue;
		
		//����d4
		edge[i].calc_d4();

		//ÿ�����ϵ�D=d2+d4�ۼӵ�������cell,ע������
		edge[i].addDissipationToAdjcentCell();
	}

	//�ڵ�һ���������ڼ���ÿ����Ԫ�ĵ���ʱ�䲽��
	if (step == 1)
	{
		for (int i = 0; i < cnt_cell; i++)
			cell[i].localTimeStep = 0;

		for (int i = 0; i < cnt_edge; i++)
		{
			cell[edge[i].leftCell].localTimeStep += edge[i].ScalingFactor;
			if (edge[i].rightCell >= 0)
				cell[edge[i].rightCell].localTimeStep += edge[i].ScalingFactor;
		}
		for (int i = 0; i < cnt_cell; i++)
			cell[i].localTimeStep = CFL*cell[i].volum / cell[i].localTimeStep;
	}

	//Runge-Kutta����w_next
	for (int k = 0; k < cnt_cell; k++)
		for (int i = 0; i < cell[k].w_next.size(); i++)
			cell[k].w_next[i] = cell[k].w[i] + RungeKutta_alpha[step] * cell[k].localTimeStep*(-1.0*(cell[k].Q[i] - cell[k].D[i]) / cell[k].volum);
}

//Point���<<�����
ostream &operator<<(ostream &fout, const Point &p)
{
	for (auto c : p.coordinate)
		fout << setw(16) << c;

	return fout;
}