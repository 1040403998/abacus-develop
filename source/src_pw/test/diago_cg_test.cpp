#include "../../module_base/inverse_matrix.h"
#include "../../module_base/lapack_connector.h"
#include "../diago_cg.h"
#include "diago_mock.h"
#include "mpi.h"

#include "gtest/gtest.h"
#include <random>

/************************************************
 *  unit test of functions in Diago_CG
 ***********************************************/

/**
 * Class Diago_CG is an approach for eigenvalue problems
 * This unittest test the function Diago_CG::diag()
 * with different examples.
 *  - the Hermite matrices (npw=500,1000) produced using random numbers and with sparsity of 0%, 60%, 80%
 *  - the Hamiltonian matrix read from "data-H", produced by using out_hs in INPUT of a LCAO calculation
 *  - a 2x2 Hermite matrix for learning and checking
 *
 * Note:
 * The test is passed when the eignvalues are closed to these calculated by LAPACK.
 * It is used together with a header file diago_mock.h.
 * The default Hermite matrix generated here is real symmetric, one can add an imaginary part
 * by changing two commented out lines in diago_mock.h.
 *
 */

// call lapack in order to compare to cg
void lapackEigen(int &npw, ModuleBase::ComplexMatrix &hm, double *e, bool outtime = false)
{
    clock_t start, end;
    start = clock();
    int lwork = 2 * npw;
    std::complex<double> *work2 = new std::complex<double>[lwork];
    double *rwork = new double[3 * npw - 2];
    int info = 0;
    LapackConnector::zheev('V', 'U', npw, hm, npw, e, work2, lwork, rwork, &info);
    end = clock();
    if (outtime)
        std::cout << "Lapack Run time: " << (double)(end - start) / CLOCKS_PER_SEC << " S" << std::endl;
    delete[] rwork;
    delete[] work2;
}

class DiagoCGPrepare
{
  public:
    DiagoCGPrepare(int nband, int npw, bool sub, int sparsity, bool reorder, double eps, int maxiter, double threshold)
        : nband(nband), npw(npw), sub(sub), sparsity(sparsity), reorder(reorder), eps(eps), maxiter(maxiter),
          threshold(threshold)
    {
    }

    int nband, npw, sparsity, maxiter, notconv;
    // eps is the convergence threshold within cg_diago
    double eps, avg_iter;
    bool sub; // do subspace diagonalization if true
    bool reorder;
    double threshold;
    // threshold is the comparison standard between cg and lapack

    void CompareEigen(double *precondition)
    {
        // calculate eigenvalues by LAPACK;
        double *e_lapack = new double[npw];
        ModuleBase::ComplexMatrix ev = DIAGOTEST::hmatrix;
        lapackEigen(npw, ev, e_lapack, false);
        // initial guess of psi by perturbing lapack psi
        ModuleBase::ComplexMatrix psiguess(nband, npw);
        std::default_random_engine p(1);
        std::uniform_int_distribution<unsigned> u(1, 10);
        for (int i = 0; i < nband; i++)
        {
            for (int j = 0; j < npw; j++)
            {
		double rand = static_cast<double>(u(p))/10.;
                // psiguess(i,j) = ev(j,i)*(1+rand);
                psiguess(i, j) = ev(j, i) * rand;
            }
        }
        // run cg
        clock_t start, end;
        start = clock();
        avg_iter = 0.0;
        notconv = 0;
        double *en = new double[npw];
        int ik = 1;
        int ntry = 0;
        Hamilt_PW hpw;
        Diago_CG cg(&hpw);
        do
        {
            if (sub && ntry > 0)
            {
                hpw.diagH_subspace(ik, nband, nband, psiguess, psiguess, en);
            }
            cg.diag(psiguess, en, npw, npw, nband, precondition, eps, maxiter, reorder, notconv, avg_iter);
            ++ntry;
        } while ((ntry <= 5) && (notconv > 0));
        end = clock();
        // std::cout<<"diag Run time: "<<(double)(end - start) / CLOCKS_PER_SEC<<" S, notconv="
        //				   << notconv <<", avg_iter=" << avg_iter << std::endl;
        // std::cout << " ntry " << ntry << std::endl;
        for (int i = 0; i < nband; i++)
        {
            // std::cout << " en " << en[i] << " e_lapack " << e_lapack[i] << std::endl;
            EXPECT_NEAR(en[i], e_lapack[i], threshold);
        }

        delete[] en;
        delete[] e_lapack;
    }
};

class DiagoCGTest : public ::testing::TestWithParam<DiagoCGPrepare>
{
};

TEST_P(DiagoCGTest, RandomHamilt)
{
    DiagoCGPrepare dcp = GetParam();
    // std::cout << "npw=" << dcp.npw << ", nband=" << dcp.nband << ", sparsity="
    //		  << dcp.sparsity << ", eps=" << dcp.eps << std::endl;

    HPsi hpsi(dcp.nband, dcp.npw, dcp.sparsity);
    DIAGOTEST::hmatrix = hpsi.hamilt();
    DIAGOTEST::npw = dcp.npw;
    // ModuleBase::ComplexMatrix psi = hpsi.psi();
    dcp.CompareEigen(hpsi.precond());
}

INSTANTIATE_TEST_SUITE_P(VerifyCG,
                         DiagoCGTest,
                         ::testing::Values(
                             // nband, npw, sub, sparsity, reorder, eps, maxiter, threshold
                             DiagoCGPrepare(10, 500, true, 0, true, 1e-5, 50, 1e-3),
                             DiagoCGPrepare(20, 500, true, 6, true, 1e-5, 50, 1e-3),
                             DiagoCGPrepare(20, 1000, true, 8, true, 1e-5, 50, 1e-3),
                             DiagoCGPrepare(40, 1000, true, 8, true, 1e-5, 50, 1e-3)));

// check that the mock class HPsi work well
// in generating a Hermite matrix
TEST(DiagoCGTest, Hamilt)
{
    int dim = 2;
    int nbnd = 2;
    HPsi hpsi(nbnd, dim);
    ModuleBase::ComplexMatrix hm = hpsi.hamilt();
    EXPECT_EQ(hm.nr, 2);
    EXPECT_EQ(hm.nc, 2);
    EXPECT_EQ(hm(0, 0).imag(), 0.0);
    EXPECT_EQ(hm(1, 1).imag(), 0.0);
    EXPECT_EQ(conj(hm(1, 0)).real(), hm(0, 1).real());
    EXPECT_EQ(conj(hm(1, 0)).imag(), hm(0, 1).imag());
}

// check that lapack work well
// for an eigenvalue problem
TEST(DiagoCGTest, ZHEEV)
{
    int dim = 100;
    int nbnd = 2;
    HPsi hpsi(nbnd, dim);
    ModuleBase::ComplexMatrix hm = hpsi.hamilt();
    ModuleBase::ComplexMatrix hm_backup = hm;
    ModuleBase::ComplexMatrix eig(dim, dim);
    double e[dim];
    // using zheev to do a direct test
    lapackEigen(dim, hm, e);
    eig = transpose(hm, true) * hm_backup * hm;
    // for (int i=0;i<dim;i++) std::cout<< " e[i] "<<e[i]<<std::endl;
    for (int i = 0; i < dim; i++)
    {
        EXPECT_NEAR(e[i], eig(i, i).real(), 1e-10);
    }
}

// cg for a 2x2 matrix
TEST(DiagoCGTest, TwoByTwo)
{
    int dim = 2;
    int nband = 2;
    ModuleBase::ComplexMatrix hm(2, 2);
    hm(0, 0) = std::complex<double>{4.0, 0.0};
    hm(0, 1) = std::complex<double>{1.0, 0.0};
    hm(1, 0) = std::complex<double>{1.0, 0.0};
    hm(1, 1) = std::complex<double>{3.0, 0.0};
    // nband, npw, sub, sparsity, reorder, eps, maxiter, threshold
    DiagoCGPrepare dcp(nband, dim, false, 0, true, 1e-4, 50, 1e-10);
    HPsi hpsi;
    hpsi.create(nband, dim);
    DIAGOTEST::hmatrix = hm;
    DIAGOTEST::npw = dim;
    dcp.CompareEigen(hpsi.precond());
}

TEST(DiagoCGTest, readH)
{
    // read Hamilt matrix from file data-H
    ModuleBase::ComplexMatrix hm;
    std::ifstream ifs;
    ifs.open("data-H");
    DIAGOTEST::readh(ifs, hm);
    ifs.close();
    int dim = hm.nr;
    int nband = 10; // not nband < dim, here dim = 26 in data-H
    // nband, npw, sub, sparsity, reorder, eps, maxiter, threshold
    DiagoCGPrepare dcp(nband, dim, true, 0, true, 1e-4, 50, 1e-3);
    HPsi hpsi;
    hpsi.create(nband, dim);
    DIAGOTEST::hmatrix = hpsi.hamilt();
    DIAGOTEST::npw = dim;
    dcp.CompareEigen(hpsi.precond());
}

int main(int argc, char **argv)
{

    MPI_Init(&argc, &argv);

    testing::InitGoogleTest(&argc, argv);
    int result = RUN_ALL_TESTS();

    MPI_Finalize();

    return result;
}
