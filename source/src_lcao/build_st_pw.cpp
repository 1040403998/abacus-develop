#include "build_st_pw.h"
#include "../src_pw/global.h"

#include "global_fp.h" // mohan add 2021-01-30

Build_ST_pw::Build_ST_pw()
{

}

Build_ST_pw::~Build_ST_pw()
{

}

// be called in LCAO_Hamilt::calculate_STNR_k()
// FUNCTION: calculate the overlap and kinetic matrix
// in localized basis (expanded in plane wave basis).
void Build_ST_pw::set_ST(const int &ik, const char& dtype)
{
	switch (dtype)
	{
		case 'S':
		{
			for(int i=0; i<GlobalV::NLOCAL; i++)
			{
				const int mu = ParaO.trace_loc_row[i];
				if(mu < 0)continue;
				for(int j=0; j<GlobalV::NLOCAL; j++)
				{
					const int nu = ParaO.trace_loc_col[j];
					if(nu < 0)continue;
					
					if(GlobalV::NSPIN!=4)
					{
						complex<double> v = ZERO;
						for (int ig = 0; ig < GlobalC::kv.ngk[ik]; ig++) 
						{
							v += conj(GlobalC::wf.wanf2[ik](mu, ig)) * GlobalC::wf.wanf2[ik](nu, ig);
						}
						
		//				cout << "i=" << i << " j=" << j << " v=" << v << endl;
						//-----------------------------------
						// The results are saved in Sloc2.
						// 2 stands for k points.
						//-----------------------------------
						LM.Sloc2[ mu * ParaO.ncol + nu ] = v;
					}
					else//added by zhengdy-soc
					{
/*						complex<double> v0 = ZERO, v1 = ZERO, v2 = ZERO, v3 = ZERO;
						for (int ig = 0; ig < GlobalC::kv.ngk[ik]; ig++)
						{
							v0 += conj(GlobalC::wf.wanf2[ik](mu, ig)) * GlobalC::wf.wanf2[ik](nu, ig);
							v1 += conj(GlobalC::wf.wanf2[ik](mu, ig)) * GlobalC::wf.wanf2[ik](nu, ig + GlobalC::wf.npwx);
							v2 += conj(GlobalC::wf.wanf2[ik](mu, ig + GlobalC::wf.npwx)) * GlobalC::wf.wanf2[ik](nu, ig);
							v3 += conj(GlobalC::wf.wanf2[ik](mu, ig + GlobalC::wf.npwx)) * GlobalC::wf.wanf2[ik](nu, ig + GlobalC::wf.npwx);
						}
						LM.Sloc2_soc(0, mu * ParaO.ncol + nu) = v0;
						LM.Sloc2_soc(1, mu * ParaO.ncol + nu) = v1;
						LM.Sloc2_soc(2, mu * ParaO.ncol + nu) = v2;
						LM.Sloc2_soc(3, mu * ParaO.ncol + nu) = v3;*/
						complex<double> v0 = ZERO;
						for (int ig = 0; ig < GlobalC::wf.npwx*GlobalV::NPOL; ig++)
							v0 += conj(GlobalC::wf.wanf2[ik](mu, ig)) * GlobalC::wf.wanf2[ik](nu, ig);
						LM.Sloc2[ mu * ParaO.ncol + nu ] = v0;

					}
				}
			}
			break;
		}
		case 'T':
		{
			//------------------------------------
			//calculate the kinetic energy of ik.
			//------------------------------------
			GlobalC::wf.ekin(ik);

			for(int i=0; i<GlobalV::NLOCAL; i++)
			{
				const int mu = ParaO.trace_loc_row[i];
				if(mu < 0)continue;
				for(int j=0; j<GlobalV::NLOCAL; j++)
				{
					const int nu = ParaO.trace_loc_col[j];
					if(nu < 0)continue;
					
					complex<double> v = ZERO;
					for (int ig = 0; ig < GlobalC::kv.ngk[ik]; ig++) 
					{
						v += conj(GlobalC::wf.wanf2[ik](mu, ig)) * GlobalC::wf.wanf2[ik](nu, ig) * GlobalC::wf.g2kin[ig];
					}
					if(GlobalV::NSPIN==4)
					for (int ig = 0; ig < GlobalC::kv.ngk[ik]; ig++)
					{
						v += conj(GlobalC::wf.wanf2[ik](mu, ig + GlobalC::wf.npwx)) * GlobalC::wf.wanf2[ik](nu, ig + GlobalC::wf.npwx) * GlobalC::wf.g2kin[ig];
					}
					
	//				cout << "i=" << i << " j=" << j << " v=" << v << endl;
					//-----------------------------------------
					// The results are saved in Hloc_fixed2.
					//-----------------------------------------
					LM.Hloc_fixed2[ mu * ParaO.ncol + nu ] = v;
				}
			}
			break;
		}
	}

	return;
}

void Build_ST_pw::set_local(const int &ik)
{
	TITLE("Build_ST_pw","set_local");
	timer::tick("Build_ST_pw","set_local");
	assert(GlobalV::NLOCAL>0);
	assert(!GlobalV::GAMMA_ONLY_LOCAL);

    const int npw = GlobalC::kv.ngk[ik];
    complex<double> *psi_one = new complex<double>[npw];
    complex<double> *hpsi = new complex<double>[npw];
	complex<double> *psic = new complex<double>[GlobalC::pw.nrxx];
	int *fft_index = new int[npw];
	for(int ig=0; ig<npw; ig++)
	{
		fft_index[ig] = GlobalC::pw.ig2fftw[ GlobalC::wf.igk(ik, ig) ];
	}

//	ComplexMatrix vij(GlobalV::NLOCAL, GlobalV::NLOCAL);

	for(int i=0; i<GlobalV::NLOCAL; i++)
	{
		if(GlobalV::NSPIN!=4)
		{
			for(int ig=0; ig<npw; ig++)
			{
				psi_one[ig] = GlobalC::wf.wanf2[ik](i, ig);
			}

			ZEROS( psic, GlobalC::pw.nrxx);
			// (1) set value
			for (int ig=0; ig< npw; ig++)
			{
				psic[ fft_index[ig]  ] = psi_one[ig];
			}

			// (2) fft to real space and doing things.
			GlobalC::pw.FFT_wfc.FFT3D( psic, 1);
			for (int ir=0; ir< GlobalC::pw.nrxx; ir++)
			{
				psic[ir] *= GlobalC::pot.vr_eff1[ir];
			}

			// (3) fft back to G space.
			GlobalC::pw.FFT_wfc.FFT3D( psic, -1);

			for(int ig=0; ig<npw; ig++)
			{
				hpsi[ig] = psic[ fft_index[ig] ];
			}

			for(int j=i; j<GlobalV::NLOCAL; j++)
			{
				complex<double> v = ZERO;
				for(int ig=0; ig<npw; ig++)
				{
					v += conj( GlobalC::wf.wanf2[ik](j,ig) ) * hpsi[ig];
				}
	//			vij(j, i) = v;
				LM.set_HSk(j,i,v,'L');
				if(i!=j)
				{
					LM.set_HSk(i,j,conj(v),'L');
				}
			}
		}
		else//noncolinear case
		{
			complex<double>* psi_down = new complex<double> [npw];
			complex<double> *psic1 = new complex<double>[GlobalC::pw.nrxx];
			delete[] hpsi;
			hpsi = new complex<double> [GlobalC::wf.npwx*GlobalV::NPOL];
			ZEROS(hpsi, GlobalC::wf.npwx*GlobalV::NPOL);
			
			for(int ig=0; ig<npw; ig++)
			{
				psi_one[ig] = GlobalC::wf.wanf2[ik](i, ig);
				psi_down[ig] = GlobalC::wf.wanf2[ik](i, ig+ GlobalC::wf.npwx);
			}

			ZEROS( psic, GlobalC::pw.nrxx);
			ZEROS( psic1, GlobalC::pw.nrxx);
			// (1) set value
			for (int ig=0; ig< npw; ig++)
			{
				psic[ fft_index[ig]  ] = psi_one[ig];
				psic1[ fft_index[ig]  ] = psi_down[ig];
			}

			// (2) fft to real space and doing things.
			GlobalC::pw.FFT_wfc.FFT3D( psic, 1);
			GlobalC::pw.FFT_wfc.FFT3D( psic1, 1);
			complex<double> sup,sdown;
			for (int ir=0; ir< GlobalC::pw.nrxx; ir++)
			{
				sup = psic[ir] * (GlobalC::pot.vr_eff(0,ir) + GlobalC::pot.vr_eff(3,ir)) +
					psic1[ir] * (GlobalC::pot.vr_eff(1,ir) - complex<double>(0.0,1.0) * GlobalC::pot.vr_eff(2,ir));
				sdown = psic1[ir] * (GlobalC::pot.vr_eff(0,ir) - GlobalC::pot.vr_eff(3,ir)) +
					psic[ir] * (GlobalC::pot.vr_eff(1,ir) + complex<double>(0.0,1.0) * GlobalC::pot.vr_eff(2,ir));
				
				psic[ir] = sup;
				psic1[ir] = sdown;
			}
	
			// (3) fft back to G space.
			GlobalC::pw.FFT_wfc.FFT3D( psic, -1);
			GlobalC::pw.FFT_wfc.FFT3D( psic1, -1);
	
			for(int ig=0; ig<npw; ig++)
			{
				hpsi[ig] = psic[ fft_index[ig] ];
				hpsi[ig+GlobalC::wf.npwx] = psic1[ fft_index[ig] ];
			}

			for(int j=i; j<GlobalV::NLOCAL; j++)
			{
				complex<double> v = ZERO;
				for(int ig=0; ig<npw; ig++)
				{
					v += conj( GlobalC::wf.wanf2[ik](j,ig) ) * hpsi[ig];
					v += conj( GlobalC::wf.wanf2[ik](j,ig + GlobalC::wf.npwx) ) * hpsi[ig + GlobalC::wf.npwx];
				}
//			vij(j, i) = v;
				LM.set_HSk(j,i,v,'L');
				if(i!=j)
				{
					LM.set_HSk(i,j,conj(v),'L');
				}
			}
			delete[] psi_down;
			delete[] psic1;
		}
	}

//	out.printcm_norm("vij",vij,1.0e-5);

	delete[] fft_index;			
    delete[] psi_one;
    delete[] hpsi;
	delete[] psic;
	timer::tick("Build_ST_pw","set_local");
	return;
}
