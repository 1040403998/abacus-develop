#ifndef TOWannier90_H
#define TOWannier90_H

#include <iostream>
using namespace std;
#include <vector>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include "src_pw/tools.h"
#include "src_global/lapack_connector.h"
#include "src_pw/global.h"


class toWannier90
{
public:
	const int k_supercell = 5;                                                              // default the k-space supercell
	const int k_cells = (2 * k_supercell + 1)*(2 * k_supercell + 1)*(2 * k_supercell + 1);  // the primitive cell number in k-space supercell
	const int k_shells = 12;                                                                // default the shell numbers
	const double large_number = 99999999.0;
	const double small_number = 0.000001;
	int num_kpts;                                                                           // k�����Ŀ
	Matrix3 recip_lattice;
	vector<Vector3<double>> lmn;                                                            //ÿ��k��ԭ�����
	vector<double> dist_shell;                                                              //ÿһ��shell�Ľ���k�����
	vector<int> multi;                                                                      //ÿһ��shell�Ľ���k����Ŀ
	int num_shell_real;                                                                     //����������B1������shell��Ŀ�����ս����(ע��1��ʼ����)
	int *shell_list_real;                                                                   //1��12��shell�в�ƽ�в��ȼ۵�shell��ǩ������Ϊnum_shell_real
	double *bweight;                                                                        //ÿ��shell��bweight������Ϊnum_shell_real
	vector<vector<int>> nnlist;                                                             //ÿ��k��Ľ���k�����
	vector<vector<Vector3<double>>> nncell;                                                 //ÿ��k��Ľ���k�����ڵ�ԭ�����
	int nntot = 0;                                                                          //ÿ��k��Ľ���k����Ŀ   
	int num_wannier;																		//��Ҫ����wannier�����ĸ���
	int *L;																					//��̽����Ľ�������ָ��,����Ϊnum_wannier
	int *m;																					//��̽����Ĵ�������ָ��,����Ϊnum_wannier
	int *rvalue;																			//��̽����ľ��򲿷ֺ�����ʽ,ֻ��������ʽ,����Ϊnum_wannier
	double *alfa;																			//��̽����ľ��򲿷ֺ����еĵ��ڲ���,����Ϊnum_wannier
	Vector3<double> *R_centre;																//��̽�����������,����Ϊnum_wannier,cartesian����






	toWannier90(int num_kpts,Matrix3 recip_lattice);
	~toWannier90();

	void kmesh_supercell_sort(); //������ԭ��ľ����С��������lmn
	void get_nnkpt_first();      //������12��shell�Ľ���k��ľ���͸���
	void kmesh_get_bvectors(int multi, int reference_kpt, double dist_shell, vector<Vector3<double>>& bvector);  //��ȡָ��shell�㣬ָ���ο�k��Ľ���k���bvector
	void get_nnkpt_last(); //��ȡ���յ�shell��Ŀ��bweight
    void get_nnlistAndnncell();

	void init_wannier();
	void cal_Amn();
	void cal_Mmn();
	void produce_trial_in_pw(const int &ik, ComplexMatrix &trial_orbitals_k);
	void integral(const int meshr, const double *psir, const double *r, const double *rab, const int &l, double* table);
	void writeUNK();
	void ToRealSpace(const int &ik, const int &ib, const ComplexMatrix *evc, complex<double> *psir, const Vector3<double> G);
	//void ToReciSpace(const complex<double> *psir, complex<double> *psik, const int ib);
	complex<double> unkdotb(const complex<double> *psir, const int ikb, const int bandindex);

};

#endif